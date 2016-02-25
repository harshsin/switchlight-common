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

#include "macblaster_int.h"

static bool macblaster_initialized = false;
static uint8_t macblaster_pkt[60];

struct macblaster_state {
    indigo_cxn_id_t cxn_id;
    of_port_no_t of_port;
    of_list_bsn_tlv_t *cmd_tlvs;
    of_object_t cmd_tlv;
};

/* Debug counters */
static debug_counter_t parse_failure_counter;
static debug_counter_t pktout_failure_counter;

static void
macblaster_send_packet(of_port_no_t of_port,
                       uint16_t vlan_vid,
                       of_mac_addr_t eth_src)
{
    indigo_error_t rv;
    ppe_packet_t ppep;
    of_mac_addr_t eth_dst = {{0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC}};
    of_octets_t octets = { macblaster_pkt, sizeof(macblaster_pkt) };

    memset(macblaster_pkt, 0, sizeof(macblaster_pkt));
    ppe_packet_init(&ppep, macblaster_pkt, sizeof(macblaster_pkt));

    /* Set ethertypes before parsing */
    if (vlan_vid) {
        macblaster_pkt[12] = 0x81;
        macblaster_pkt[13] = 0x00;
        macblaster_pkt[16] = 0x88;
        macblaster_pkt[17] = 0xb5;
    } else {
        macblaster_pkt[12] = 0x88;
        macblaster_pkt[13] = 0xb5;
    }

    if (ppe_parse(&ppep) < 0) {
        AIM_DIE("macblaster_send_packet parsing failed");
    }

    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_DST_MAC,
                           eth_dst.addr) < 0) {
        AIM_DIE("Failed to set PPE_FIELD_ETHERNET_DST_MAC");
    }

    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_SRC_MAC,
                           eth_src.addr) < 0) {
        AIM_DIE("Failed to set PPE_FIELD_ETHERNET_SRC_MAC");
    }

    if (vlan_vid) {
        if (ppe_field_set(&ppep, PPE_FIELD_8021Q_VLAN, vlan_vid) < 0) {
            AIM_DIE("Failed to set PPE_FIELD_8021Q_VLAN");
        }
    }

    rv = slshared_fwd_packet_out(&octets, 0, of_port,
                                 SLSHARED_CONFIG_PDU_QUEUE_PRIORITY);
    if (rv < 0) {
        debug_counter_inc(&pktout_failure_counter);
        AIM_LOG_ERROR("Failed to send packet: %s", indigo_strerror(rv));
    }
}

static ind_soc_task_status_t
macblaster_task(void *cookie)
{
    struct macblaster_state *state = cookie;
    of_mac_addr_t mac;
    uint16_t vlan_vid;
    of_object_t bucket_tlv;
    of_list_bsn_tlv_t bucket_tlvs;

    while (!of_list_bsn_tlv_next(state->cmd_tlvs, &state->cmd_tlv)) {
        if (state->cmd_tlv.object_id != OF_BSN_TLV_BUCKET) {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected bucket TLV, instead got %s",
                          __FUNCTION__, of_class_name(&state->cmd_tlv));
            goto done;
        }

        of_bsn_tlv_bucket_value_bind(&state->cmd_tlv, &bucket_tlvs);

        if (of_list_bsn_tlv_first(&bucket_tlvs, &bucket_tlv) < 0) {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got end of list",
                          __FUNCTION__);
            goto done;
        }

        if (bucket_tlv.object_id == OF_BSN_TLV_VLAN_VID) {
            of_bsn_tlv_vlan_vid_value_get(&bucket_tlv, &vlan_vid);
        } else {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got %s",
                          __FUNCTION__, of_class_name(&bucket_tlv));
            goto done;
        }

        if (of_list_bsn_tlv_next(&bucket_tlvs, &bucket_tlv) < 0) {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected mac TLV, instead got end of list",
                          __FUNCTION__);
            goto done;
        }

        if (bucket_tlv.object_id == OF_BSN_TLV_MAC) {
            of_bsn_tlv_mac_value_get(&bucket_tlv, &mac);
        } else {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected mac TLV, instead got %s",
                          __FUNCTION__, of_class_name(&bucket_tlv));
            goto done;
        }

        if (of_list_bsn_tlv_next(&bucket_tlvs, &bucket_tlv) == 0) {
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected end of bucket TLV list, instead got %s",
                          __FUNCTION__, of_class_name(&bucket_tlv));
            goto done;
        }

        macblaster_send_packet(state->of_port, vlan_vid, mac);

        if (ind_soc_should_yield()) {
            return IND_SOC_TASK_CONTINUE;
        }
    }

done:
    indigo_cxn_resume(state->cxn_id);

    aim_free(state->cmd_tlvs);
    aim_free(state);

    return IND_SOC_TASK_FINISHED;
}

static indigo_error_t
macblaster_handle_flexlink_commmad(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    of_str64_t command_name;
    struct macblaster_state *state;

    of_bsn_generic_command_name_get(msg, &command_name);

    if (strcmp(command_name, "flexlink")) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    state = aim_zmalloc(sizeof(*state));
    state->cxn_id = cxn_id;

    /* Get command TLVs and parse port number */
    state->cmd_tlvs = of_bsn_generic_command_tlvs_get(msg);
    if (state->cmd_tlvs == NULL) {
        debug_counter_inc(&parse_failure_counter);
        AIM_LOG_ERROR("%s: getting command tlvs", __FUNCTION__);
        goto error;
    }

    if (of_list_bsn_tlv_first(state->cmd_tlvs, &state->cmd_tlv) < 0) {
        debug_counter_inc(&parse_failure_counter);
        AIM_LOG_ERROR("%s: expected port TLV, instead got end of list",
                      __FUNCTION__);
        goto error;
    }

    if (state->cmd_tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&state->cmd_tlv, &state->of_port);
    } else {
        debug_counter_inc(&parse_failure_counter);
        AIM_LOG_ERROR("%s: expected port TLV, instead got %s",
                      __FUNCTION__, of_class_name(&state->cmd_tlv));
        goto error;
    }

    /* Start long running task which parses VLAN, MAC buckets
     * and sends packet-out messages */
    if (ind_soc_task_register(macblaster_task, state, IND_SOC_NORMAL_PRIORITY) < 0) {
        AIM_DIE("Failed to create macblaster long running task");
    }

    indigo_cxn_pause(cxn_id);

    return INDIGO_CORE_LISTENER_RESULT_DROP;

error:
    if (state) {
        if (state->cmd_tlvs) aim_free(state->cmd_tlvs);
        aim_free(state);
    }

    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

static indigo_core_listener_result_t
macblaster_message_listener(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    switch (msg->object_id) {
        case OF_BSN_GENERIC_COMMAND:
            return macblaster_handle_flexlink_commmad(cxn_id, msg);

        default:
            return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}

indigo_error_t
macblaster_init(void)
{
    if (macblaster_initialized) return INDIGO_ERROR_NONE;

    indigo_core_message_listener_register(macblaster_message_listener);

    debug_counter_register(
        &parse_failure_counter, "macblaster.parse_failure",
        "Failed to parse message from controller");

    debug_counter_register(
        &pktout_failure_counter, "macblaster.pktout_failure",
        "Failed to sent packet-outs");

    macblaster_initialized = true;

    return INDIGO_ERROR_NONE;
}

indigo_error_t
macblaster_finish(void)
{
    if (!macblaster_initialized) return INDIGO_ERROR_NONE;

    indigo_core_message_listener_unregister(macblaster_message_listener);

    macblaster_initialized = false;

    return INDIGO_ERROR_NONE;
}

