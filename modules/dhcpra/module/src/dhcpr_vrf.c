/****************************************************************
 *
 *        Copyright 2015, Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

#include "dhcpra_int.h"
#include "dhcpr_vrf.h"
#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <murmur/murmur.h>

typedef struct {
    bighash_entry_t hash_entry;
    uint32_t      internal_vlan;
    uint32_t      vrf;
    of_mac_addr_t vrouter_mac;
} dhcp_vrf_t;

/*
 * Define key for dhcp_vrf table
 * 1st value is vlan
 * 2nd value is vrouter_mac
 *
 * Use attribute 'packed', so that we can use murmur / memcmp directly
 * Otherwise, must initialize padding
 */
typedef struct {
    uint32_t      vlan;
    of_mac_addr_t mac;
} __attribute__((packed)) dhcp_vrf_key_t;

static void
dhcpr_vrf_get_stats(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats);
static indigo_error_t
dhcpr_vrf_delete(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key);
static indigo_error_t
dhcpr_vrf_add(void *table_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value, void **entry_priv);
static indigo_error_t
dhcpr_vrf_modify(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value);

static indigo_core_gentable_t *dhcpr_vrf;
static const indigo_core_gentable_ops_t dhcpr_vrf_ops;

/* dhcpr_vrf table: InternalVlan+Mac to vrf */
BIGHASH_DEFINE_STATIC(dhcpr_vrf_table, 256);

/*
 * Iterate on entries of dhcp_vrf_table.
 * bighash_iter_t _iter - Big Hash iterator,
 * dhcp_vrf_t   *_entry - Pointer to dhcp_vrf entry.
 */
#define DHCP_VRF_TABLE_ITERATE(_iter, _entry)        \
    for (_entry = (dhcp_vrf_t *)bighash_iter_start(&dhcpr_vrf_table, &_iter); \
         _entry != NULL;                         \
         _entry = (dhcp_vrf_t *)bighash_iter_next(&_iter))

static const indigo_core_gentable_ops_t dhcpr_vrf_ops = {
    .add = dhcpr_vrf_add,
    .modify = dhcpr_vrf_modify,
    .del = dhcpr_vrf_delete,
    .get_stats = dhcpr_vrf_get_stats,
};

/* Register dhcpr_vrf table */
indigo_error_t
dhcpr_vrf_init()
{
    /*
     * Caller provides the dhcpr_vrf: UNUSED in our case
     * Callee provides dhcpr_vrf_ops
     */
    indigo_core_gentable_register("dhcp_vrf", &dhcpr_vrf_ops, NULL, VLAN_MAX+1, 256, &dhcpr_vrf);

    return INDIGO_ERROR_NONE;
}

/* Unregister dhcpr_vrf table */
void
dhcpr_vrf_finish()
{
    indigo_core_gentable_unregister(dhcpr_vrf);
}


/*************************************
 * dhcpr_vrf_table hash table support
 *************************************/
/*
 * vlan_key_t must be packed
 * if not, murmur might encode uninitialized padding
 */
static uint32_t
hash_key(dhcp_vrf_key_t *k)
{
    return murmur_hash(k, sizeof(*k), 0);
}

/*
 * vlan_key_t must be packed
 * if not, memcmp might compare unitialized padding
 */
static bool
is_key_equal(dhcp_vrf_key_t *key1, dhcp_vrf_key_t *key2)
{
    return (memcmp(key1, key2, sizeof(*key2)) == 0);
}

static dhcp_vrf_t *
find_hash_entry_by_dhcp_vrf_key(bighash_table_t *table,
                                dhcp_vrf_key_t *key)
{
    bighash_entry_t *e;
    dhcp_vrf_key_t entry_key;

    for (e = bighash_first(table, hash_key(key)); e; e = bighash_next(e)) {
        dhcp_vrf_t *te = container_of(e, hash_entry, dhcp_vrf_t);
        entry_key.vlan = te->internal_vlan;
        entry_key.mac = te->vrouter_mac;
        if (is_key_equal(&entry_key, key)) {
            return te;
        }
    }
    return NULL;
}

/*************************
 * dhcpr_vrf table operation supports
 *************************/
static indigo_error_t
dhcpr_vrf_parse_key(of_list_bsn_tlv_t *tlvs, uint16_t *vlan, of_mac_addr_t *mac)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty key list");
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, vlan);
    } else {
        AIM_LOG_ERROR("expected vlan key TLV, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of key list");
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_MAC) {
        of_bsn_tlv_mac_value_get(&tlv, mac);
    } else {
        AIM_LOG_ERROR("expected mac key TLV, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of key list, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
dhcpr_vrf_parse_value(of_list_bsn_tlv_t *tlvs, uint32_t *vrf)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VRF) {
        of_bsn_tlv_vrf_value_get(&tlv, vrf);
    } else {
        AIM_LOG_ERROR("expected VRF value TLV, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of value list, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * Allocate 1 hash entry
 * If return error, caller doesn't need to free any memory
 */
static indigo_error_t
dhcpr_vrf_add_entry(dhcp_vrf_t *entry)
{
    dhcp_vrf_key_t key;

    /*
     * Vlan and virtualRtouerIP is a mapping 1:1
     * New vlan entry: new virtual_router_ip
     */
    key.vlan = entry->internal_vlan;
    key.mac  = entry->vrouter_mac;
    if (find_hash_entry_by_dhcp_vrf_key(&dhcpr_vrf_table, &key)) {
        AIM_LOG_ERROR("Virtual Router entry exists for VLAN=%u MAC=%02x:%02x:%02x:%02x:%02x:%02x",
                      entry->internal_vlan,
                      entry->vrouter_mac.addr[0], entry->vrouter_mac.addr[1], entry->vrouter_mac.addr[2],
                      entry->vrouter_mac.addr[3], entry->vrouter_mac.addr[4], entry->vrouter_mac.addr[5]);
        return INDIGO_ERROR_EXISTS;
    }

    bighash_insert(&dhcpr_vrf_table, &entry->hash_entry, hash_key(&key));

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
dhcpr_vrf_add(void *table_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value, void **entry_priv)
{
    indigo_error_t rv;
    uint16_t       vlan;
    uint32_t       vrf;
    of_mac_addr_t  mac;
    dhcp_vrf_t *entry = NULL;

    rv = dhcpr_vrf_parse_key(key, &vlan, &mac);
    if (rv < 0) {
        return rv;
    }

    rv = dhcpr_vrf_parse_value(value, &vrf);
    if (rv < 0) {
        return rv;
    }

    entry = aim_zmalloc(sizeof(dhcp_vrf_t));

    /* Set key */
    entry->internal_vlan = vlan;
    entry->vrouter_mac = mac;

    /* Set value */
    entry->vrf = vrf;

    rv = dhcpr_vrf_add_entry(entry);
    if (rv == INDIGO_ERROR_NONE) {
        *entry_priv = entry;
    } else {
        /* Free entry and all internal data */
        aim_free(entry);
    }
    return rv;
}

static indigo_error_t
dhcpr_vrf_modify(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value)
{
    indigo_error_t rv;
    uint32_t vrf;
    dhcp_vrf_t *entry = entry_priv;

    rv = dhcpr_vrf_parse_value(value, &vrf);
    if (rv < 0) {
        return rv;
    }

    /* Update */
    entry->vrf = vrf;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
dhcpr_vrf_delete(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key)
{
    dhcp_vrf_t *entry = entry_priv;
    bighash_remove(&dhcpr_vrf_table, &entry->hash_entry);
    aim_free(entry);
    return INDIGO_ERROR_NONE;
}

static void
dhcpr_vrf_get_stats(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    /* Empty */
}

/**
 * Utility function
 *
 *
 * Get vrf from vlan and mac address values
 *
 * ret: 0; if successful
 */
int
dhcpr_vrf_find(uint32_t *vrf, uint32_t vlan, uint8_t *mac_addr)
{
    dhcp_vrf_key_t key;

    key.vlan = vlan;
    memcpy(key.mac.addr, mac_addr, OF_MAC_ADDR_BYTES);
    dhcp_vrf_t *entry = find_hash_entry_by_dhcp_vrf_key(&dhcpr_vrf_table, &key);

    if (entry) {
        *vrf = entry->vrf;
        return 0;
    }

    return -1;
}

/* Print dhcp vrf table */
void
dhcpr_vrf_table_print(aim_pvs_t *apvs)
{
    dhcp_vrf_t *entry;
    bighash_iter_t iter;

    aim_printf(apvs, "VLAN\tMAC\tVRF\n");
    DHCP_VRF_TABLE_ITERATE(iter, entry) {
        aim_printf(apvs, "%u\t%02x:%02x:%02x:%02x:%02x:%02x\t%u\n",
                   entry->internal_vlan,
                   entry->vrouter_mac.addr[0],entry->vrouter_mac.addr[1],entry->vrouter_mac.addr[2],
                   entry->vrouter_mac.addr[3],entry->vrouter_mac.addr[4],entry->vrouter_mac.addr[5],
                   entry->vrf);
    }
}
