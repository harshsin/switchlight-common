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
#include "vxlan_int.h"
#include "vxlan_log.h"

#define MAX_VLAN 4095

static indigo_core_gentable_t *vxlan_vni_vlan_mapping;
static const indigo_core_gentable_ops_t vxlan_vni_vlan_mapping_ops;

indigo_error_t
vxlan_gentable_vni_vlan_mapping_init(void)
{
    indigo_core_gentable_register("vxlan_vni_vlan_mapping",
                                  &vxlan_vni_vlan_mapping_ops,
                                  NULL, MAX_VLAN+1, 256, &vxlan_vni_vlan_mapping);

    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_gentable_vni_vlan_mapping_deinit(void)
{
    indigo_core_gentable_unregister(vxlan_vni_vlan_mapping);

    return INDIGO_ERROR_NONE;
}

int
vxlan_gentable_vni_vlan_mapping_get(void)
{
}

static indigo_error_t
vxlan_vni_vlan_mapping_parse_key(of_list_bsn_tlv_t *tlvs,
                                 vxlan_vni_vlan_mapping_key_t *key)
{
    of_object_t tlv;

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
        of_bsn_tlv_vni_value_get(&tlv, &key->ipv4_src);
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
    vxlan_vni_vlan_mapping_t *entry;

    if ((rv = vxlan_vni_vlan_mapping_parse_key(key, &entry->key)) < 0) {
        return rv;
    }

    if ((rv = vxlan_vni_vlan_mapping_parse_value(value, &entry->value)) < 0) {
        return rv;
    }

    *entry_priv = entry;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_modify(void *table_priv, void *entry_priv,
                                of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value)
{
    indigo_error_t rv;
    vxlan_vni_vlan_mapping_t *entry = entry_priv;
    vxlan_vni_vlan_mapping_t new_entry;

    if ((rv = vxlan_vni_vlan_mapping_parse_value(value, &new_entry.value)) < 0) {
        return rv;
    }

    entry->value = new_entry->value;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_vni_vlan_mapping_delete(void *table_priv, void *entry_priv,
                                 of_list_bsn_tlv_t *key)
{
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
