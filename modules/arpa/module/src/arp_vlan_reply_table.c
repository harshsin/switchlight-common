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

#include <arpa/arpa.h>
#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <AIM/aim_list.h>

#include "arpa_log.h"

struct arp_vlan_reply_entry_key {
    uint16_t vlan_vid;
};

struct arp_vlan_reply_entry_value {
    of_mac_addr_t mac;
};

struct arp_vlan_reply_entry {
    bighash_entry_t hash_entry;
    struct arp_vlan_reply_entry_key key;
    struct arp_vlan_reply_entry_value value;
};

#define TEMPLATE_NAME arp_vlan_reply_hashtable
#define TEMPLATE_OBJ_TYPE struct arp_vlan_reply_entry
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

static indigo_core_gentable_t *arp_vlan_reply_table;
static const indigo_core_gentable_ops_t arp_vlan_reply_ops;
static bighash_table_t *arp_vlan_reply_hashtable;

void
arpa_vlan_reply_table_init(void)
{
    arp_vlan_reply_hashtable = bighash_table_create(1024);
    indigo_core_gentable_register("arp_vlan_reply", &arp_vlan_reply_ops, NULL, 16384, 1024,
                                  &arp_vlan_reply_table);
}

void
arpa_vlan_reply_table_finish(void)
{
    indigo_core_gentable_unregister(arp_vlan_reply_table);
    bighash_table_destroy(arp_vlan_reply_hashtable, NULL);
}

/* arp_vlan_reply table operations */

static indigo_error_t
arp_vlan_reply_parse_key(of_list_bsn_tlv_t *tlvs, struct arp_vlan_reply_entry_key *key)
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
arp_vlan_reply_parse_value(of_list_bsn_tlv_t *tlvs, struct arp_vlan_reply_entry_value *value)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_MAC) {
        of_bsn_tlv_mac_value_get(&tlv, &value->mac);
    } else {
        AIM_LOG_ERROR("expected mac value TLV, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of value list, instead got %s", of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_vlan_reply_add(void *table_priv, of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    struct arp_vlan_reply_entry_key key;
    struct arp_vlan_reply_entry_value value;
    struct arp_vlan_reply_entry *entry;

    rv = arp_vlan_reply_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = arp_vlan_reply_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    arp_vlan_reply_hashtable_insert(arp_vlan_reply_hashtable, entry);

    *entry_priv = entry;
    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_vlan_reply_modify(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    struct arp_vlan_reply_entry_value value;
    struct arp_vlan_reply_entry *entry = entry_priv;

    rv = arp_vlan_reply_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry->value = value;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
arp_vlan_reply_delete(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs)
{
    struct arp_vlan_reply_entry *entry = entry_priv;
    bighash_remove(arp_vlan_reply_hashtable, &entry->hash_entry);
    aim_free(entry);
    return INDIGO_ERROR_NONE;
}

static void
arp_vlan_reply_get_stats(void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    /* Nothing to do */
}

static const indigo_core_gentable_ops_t arp_vlan_reply_ops = {
    .add = arp_vlan_reply_add,
    .modify = arp_vlan_reply_modify,
    .del = arp_vlan_reply_delete,
    .get_stats = arp_vlan_reply_get_stats,
};


/* Hashtable lookup */

indigo_error_t
arpa_vlan_reply_table_lookup(uint16_t vlan_vid, of_mac_addr_t *mac)
{
    struct arp_vlan_reply_entry_key key;
    memset(&key, 0, sizeof(key));
    key.vlan_vid = vlan_vid;
    struct arp_vlan_reply_entry *entry = arp_vlan_reply_hashtable_first(arp_vlan_reply_hashtable, &key);
    if (entry == NULL) {
        return INDIGO_ERROR_NOT_FOUND;
    }

    *mac = entry->value.mac;
    return INDIGO_ERROR_NONE;
}
