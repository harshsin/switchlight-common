/****************************************************************
 *
 *        Copyright 2016, Big Switch Networks, Inc.
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

#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <AIM/aim_list.h>
#include "vxlan_int.h"
#include "vxlan_log.h"

#define MAX_VLAN 4095

/* NOTE: IP address can be added to key in future */
typedef struct vxlan_vni_vlan_mapping_key_s {
    uint32_t vni;
} vxlan_vni_vlan_mapping_key_t;

typedef struct vxlan_vni_vlan_mapping_s {
    bighash_entry_t hash_entry;
    vxlan_vni_vlan_mapping_key_t key;
    uint16_t vlan_vid;
} vxlan_vni_vlan_mapping_t;

#define TEMPLATE_NAME vni_vlan_mapping_hashtable
#define TEMPLATE_OBJ_TYPE vxlan_vni_vlan_mapping_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

static indigo_core_gentable_t *vxlan_vni_vlan_mapping;
static const indigo_core_gentable_ops_t vxlan_vni_vlan_mapping_ops;

static bighash_table_t *vni_vlan_mapping;

indigo_error_t
vxlan_gentable_vni_vlan_mapping_init(void)
{
    indigo_core_gentable_register("vxlan_vni_vlan_mapping",
                                  &vxlan_vni_vlan_mapping_ops,
                                  NULL, MAX_VLAN+1, 256, &vxlan_vni_vlan_mapping);
    vni_vlan_mapping = bighash_table_create(MAX_VLAN+1);
    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_gentable_vni_vlan_mapping_deinit(void)
{
    indigo_core_gentable_unregister(vxlan_vni_vlan_mapping);
    bighash_table_destroy(vni_vlan_mapping, NULL);

    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_vni_vlan_mapping_get(uint32_t vni, uint16_t *vlan_vid)
{
    vxlan_vni_vlan_mapping_key_t key = {.vni = vni};
    vxlan_vni_vlan_mapping_t *entry = vni_vlan_mapping_hashtable_first(
                                            vni_vlan_mapping, &key);

    if (entry == NULL) {
        return INDIGO_ERROR_NOT_FOUND;
    }

    *vlan_vid = entry->vlan_vid;
    return INDIGO_ERROR_NONE;
}

#if VXLAN_CONFIG_INCLUDE_UCLI == 1

void
vxlan_gentable_vni_vlan_mapping_print(ucli_context_t *uc)
{
    bighash_iter_t iter;
    vxlan_vni_vlan_mapping_t *map;

    if (vni_vlan_mapping == NULL) return;

    for (map = (vxlan_vni_vlan_mapping_t *)bighash_iter_start(vni_vlan_mapping, &iter);
         map != NULL;
         map = (vxlan_vni_vlan_mapping_t *)bighash_iter_next(&iter)) {
         ucli_printf(uc, "vni %u  vlan %u\n", map->key.vni, map->vlan_vid);
    }
}

#endif /* VXLAN_CONFIG_INCLUDE_UCLI == 1 */

static indigo_error_t
vxlan_vni_vlan_mapping_parse_key(of_list_bsn_tlv_t *tlvs,
                                 vxlan_vni_vlan_mapping_key_t *key)
{
    of_object_t tlv;
    uint32_t ipv4_src;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected vni key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VNI) {
        of_bsn_tlv_vni_value_get(&tlv, &key->vni);
    } else {
        AIM_LOG_ERROR("%s: expected vni key TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        return INDIGO_ERROR_NONE;
    }

    /* Optional IPv4 address */
    if (tlv.object_id == OF_BSN_TLV_IPV4) {
        of_bsn_tlv_vni_value_get(&tlv, &ipv4_src);
    } else {
        AIM_LOG_ERROR("%s: expected ipv4 key TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of key TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_parse_value(of_list_bsn_tlv_t *tlvs,
                                   uint16_t *vlan_vid)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected vlan_vid value TLV, "
                      "instead got end of list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, vlan_vid);
    } else {
        AIM_LOG_ERROR("%s: expected vlan_vid value TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    *vlan_vid = STRIP_OFPVID_PRESENT(*vlan_vid);

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of value TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_add(void *table_priv, of_list_bsn_tlv_t *key,
                           of_list_bsn_tlv_t *value, void **entry_priv)
{
    indigo_error_t rv;
    vxlan_vni_vlan_mapping_key_t vni_vlan_map_key;
    uint16_t vlan_vid;

    if ((rv = vxlan_vni_vlan_mapping_parse_key(key, &vni_vlan_map_key)) < 0) {
        return rv;
    }

    if ((rv = vxlan_vni_vlan_mapping_parse_value(value, &vlan_vid)) < 0) {
        return rv;
    }

    vxlan_vni_vlan_mapping_t *entry = aim_zmalloc(sizeof(*entry));
    entry->key = vni_vlan_map_key;
    entry->vlan_vid = vlan_vid;
    *entry_priv = entry;
    vni_vlan_mapping_hashtable_insert(vni_vlan_mapping, entry);

    AIM_LOG_TRACE("%s: vni %u vlan_vid %u",
                  __FUNCTION__, vni_vlan_map_key.vni, vlan_vid);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_modify(void *table_priv, void *entry_priv,
                              of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value)
{
    indigo_error_t rv;
    vxlan_vni_vlan_mapping_t *entry = entry_priv;
    uint16_t vlan_vid;

    if ((rv = vxlan_vni_vlan_mapping_parse_value(value, &vlan_vid)) < 0) {
        return rv;
    }

    AIM_LOG_TRACE("%s: vni %u old vlan_vid %u new vlan_vid %u",
                  __FUNCTION__, entry->key.vni, entry->vlan_vid, vlan_vid);
    entry->vlan_vid = vlan_vid;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_delete(void *table_priv, void *entry_priv,
                              of_list_bsn_tlv_t *key)
{
    vxlan_vni_vlan_mapping_t *entry = entry_priv;

    bighash_remove(vni_vlan_mapping, &entry->hash_entry);

    AIM_LOG_TRACE("%s: vni %u vlan_vid %u",
                  __FUNCTION__, entry->key.vni, entry->vlan_vid);

    aim_free(entry);

    return INDIGO_ERROR_NONE;
}

static void
vxlan_vni_vlan_mapping_get_stats(void *table_priv, void *entry_priv,
                                    of_list_bsn_tlv_t *key,
                                    of_list_bsn_tlv_t *stats)
{
    /* Nothing to do here */
}

static const indigo_core_gentable_ops_t vxlan_vni_vlan_mapping_ops = {
    .add = vxlan_vni_vlan_mapping_add,
    .modify = vxlan_vni_vlan_mapping_modify,
    .del = vxlan_vni_vlan_mapping_delete,
    .get_stats = vxlan_vni_vlan_mapping_get_stats,
};
