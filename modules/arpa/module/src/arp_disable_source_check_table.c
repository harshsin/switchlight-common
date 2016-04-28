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

/*
 * This table allows the controller to disable the ARP source miss check for
 * specific VLANs. It will be used to ignore duplicate IPs on the inband segment
 * in the case of VM controllers.
 *
 * The VM controller uses Linux bond mode balance-xor with ARP monitoring. Each
 * slave interface periodically sends ARP requests to a magic IP. If a slave does
 * not receive an ARP reply for some time it will be ineligible for TX.
 *
 * The problem is that all slaves on all controllers use the same source IP. This
 * means that the leaf switches see ARPs from multiple MACs with the same IP, and
 * many ARP packets are sent to the controller (without sending replies back).
 *
 * The new table lets the controller turn off the ARP source check for the inband
 * segment so the ARP agent doesn't see the duplicate source IPs.
 */

#include <arpa/arpa.h>
#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <AIM/aim_list.h>

#include "arpa_log.h"

struct arp_disable_source_check_entry_key {
    uint16_t vlan_vid;
};

struct arp_disable_source_check_entry_value {
    int unused;
};

struct arp_disable_source_check_entry {
    bighash_entry_t hash_entry;
    struct arp_disable_source_check_entry_key key;
    struct arp_disable_source_check_entry_value value;
};

#define TEMPLATE_NAME arp_disable_source_check_hashtable
#define TEMPLATE_OBJ_TYPE struct arp_disable_source_check_entry
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

static indigo_core_gentable_t *arp_disable_source_check_table;
static const indigo_core_gentable_ops_t arp_disable_source_check_ops;
static bighash_table_t *arp_disable_source_check_hashtable;

void
arpa_disable_source_check_table_init(void)
{
    arp_disable_source_check_hashtable = bighash_table_create(1024);
    indigo_core_gentable_register("arp_disable_source_check", &arp_disable_source_check_ops, NULL, 16384, 1024,
                                  &arp_disable_source_check_table);
}

void
arpa_disable_source_check_table_finish(void)
{
    indigo_core_gentable_unregister(arp_disable_source_check_table);
    bighash_table_destroy(arp_disable_source_check_hashtable, NULL);
}

/* arp_disable_source_check table operations */

static indigo_error_t
arp_disable_source_check_parse_key(of_list_bsn_tlv_t *tlvs, struct arp_disable_source_check_entry_key *key)
{
    of_object_t tlv;

    memset(key, 0, sizeof(*key));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty key list");
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &key->vlan_vid);
    } else {
        AIM_LOG_ERROR("expected vlan key TLV, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of key list, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_disable_source_check_parse_value(of_list_bsn_tlv_t *tlvs, struct arp_disable_source_check_entry_value *value)
{
    of_object_t tlv;

    memset(value, 0, sizeof(*value));

    if (of_list_bsn_tlv_first(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("Expected empty value list, found %s", of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_disable_source_check_add(void *table_priv, of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    struct arp_disable_source_check_entry_key key;
    struct arp_disable_source_check_entry_value value;
    struct arp_disable_source_check_entry *entry;

    rv = arp_disable_source_check_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = arp_disable_source_check_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    arp_disable_source_check_hashtable_insert(arp_disable_source_check_hashtable, entry);

    *entry_priv = entry;
    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_disable_source_check_modify(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    struct arp_disable_source_check_entry_value value;
    struct arp_disable_source_check_entry *entry = entry_priv;

    rv = arp_disable_source_check_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry->value = value;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_disable_source_check_delete(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs)
{
    struct arp_disable_source_check_entry *entry = entry_priv;
    bighash_remove(arp_disable_source_check_hashtable, &entry->hash_entry);
    aim_free(entry);
    return INDIGO_ERROR_NONE;
}

static void
arp_disable_source_check_get_stats(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    /* Nothing to do */
}

static const indigo_core_gentable_ops_t arp_disable_source_check_ops = {
    .add = arp_disable_source_check_add,
    .modify = arp_disable_source_check_modify,
    .del = arp_disable_source_check_delete,
    .get_stats = arp_disable_source_check_get_stats,
};


/* Hashtable lookup */

bool
arpa_disable_source_check_table_lookup(uint16_t vlan_vid)
{
    struct arp_disable_source_check_entry_key key;
    memset(&key, 0, sizeof(key));
    key.vlan_vid = vlan_vid;
    struct arp_disable_source_check_entry *entry = arp_disable_source_check_hashtable_first(arp_disable_source_check_hashtable, &key);
    if (entry == NULL) {
        return false;
    }

    return true;
}
