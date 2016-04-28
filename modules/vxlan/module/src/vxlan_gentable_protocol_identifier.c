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
#include <vxlan/vxlan.h>
#include "vxlan_int.h"
#include "vxlan_log.h"

static indigo_core_gentable_t *vxlan_protocol_identifier;
static const indigo_core_gentable_ops_t vxlan_protocol_identifier_ops;
static int vxlan_udp_dst_port = VXLAN_UDP_DST_PORT_UNDEFINED;

indigo_error_t
vxlan_gentable_protocol_identifier_init(void)
{
    indigo_core_gentable_register("vxlan_protocol_identifier",
                                  &vxlan_protocol_identifier_ops,
                                  NULL, 1, 4, &vxlan_protocol_identifier);

    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_gentable_protocol_identifier_deinit(void)
{
    indigo_core_gentable_unregister(vxlan_protocol_identifier);

    return INDIGO_ERROR_NONE;
}

int
vxlan_protocol_identifier_udp_dst_port_get(void)
{
    return vxlan_udp_dst_port;
}

static indigo_error_t
vxlan_protocol_identifier_parse_key(of_list_bsn_tlv_t *tlvs)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected empty key TLV list, instead got %s",
                      __FUNCTION__, of_object_id_str[tlv.object_id]);
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_protocol_identifier_parse_value(of_list_bsn_tlv_t *tlvs,
                                      uint16_t *udp_dst_port)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected udp_dst value TLV, "
                      "instead got end of list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_UDP_DST) {
        of_bsn_tlv_udp_dst_value_get(&tlv, udp_dst_port);
    } else {
        AIM_LOG_ERROR("%s: expected udp_dst value TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }


    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of value TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_protocol_identifier_add(void *table_priv, of_list_bsn_tlv_t *key,
                              of_list_bsn_tlv_t *value, void **entry_priv)
{
    indigo_error_t rv;
    uint16_t udp_dst_port;

    if ((rv = vxlan_protocol_identifier_parse_key(key)) < 0) {
        return rv;
    }

    if ((rv = vxlan_protocol_identifier_parse_value(value, &udp_dst_port)) < 0) {
        return rv;
    }

    /* Sanity check: vxlan_protocol_identifier gentable can have only one entry */
    if (vxlan_udp_dst_port != VXLAN_UDP_DST_PORT_UNDEFINED) {
        AIM_LOG_ERROR("%s: Trying to install more than one entry, "
                      "udp_dst_port %d", __FUNCTION__, vxlan_udp_dst_port);
        return INDIGO_ERROR_TABLE_FULL;
    }

    vxlan_udp_dst_port = udp_dst_port;

    AIM_LOG_TRACE("%s: udp_dst_port %d", __FUNCTION__, vxlan_udp_dst_port);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_protocol_identifier_modify(void *table_priv, void *entry_priv,
                                of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *value)
{
    indigo_error_t rv;
    uint16_t udp_dst_port;

    if ((rv = vxlan_protocol_identifier_parse_value(value, &udp_dst_port)) < 0) {
        return rv;
    }

    AIM_LOG_TRACE("%s: udp_dst_port old %d, new %d",
                  __FUNCTION__, vxlan_udp_dst_port, udp_dst_port);

    vxlan_udp_dst_port = udp_dst_port;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
vxlan_protocol_identifier_delete(void *table_priv, void *entry_priv,
                                 of_list_bsn_tlv_t *key)
{
    AIM_LOG_TRACE("%s: udp_dst_port %d", __FUNCTION__, vxlan_udp_dst_port);
    vxlan_udp_dst_port = VXLAN_UDP_DST_PORT_UNDEFINED;
    return INDIGO_ERROR_NONE;
}

static void
vxlan_protocol_identifier_get_stats(void *table_priv, void *entry_priv,
                                    of_list_bsn_tlv_t *key,
                                    of_list_bsn_tlv_t *stats)
{
    /* Nothing to do here */
}

static const indigo_core_gentable_ops_t vxlan_protocol_identifier_ops = {
    .add = vxlan_protocol_identifier_add,
    .modify = vxlan_protocol_identifier_modify,
    .del = vxlan_protocol_identifier_delete,
    .get_stats = vxlan_protocol_identifier_get_stats,
};
