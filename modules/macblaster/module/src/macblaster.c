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
#define MACBLASTER_PKTLEN     68

static bool macblaster_initialized = false;
static uint8_t macblaster_tagged_pkt[MACBLASTER_PKTLEN];
static uint8_t macblaster_untagged_pkt[MACBLASTER_PKTLEN];
static ppe_packet_t ppe_tagged_pkt;
static ppe_packet_t ppe_untagged_pkt;

struct macblaster_state {
    indigo_cxn_id_t cxn_id;
    of_port_no_t of_port;
    of_object_t *cmd;
    of_list_bsn_tlv_t cmd_tlvs;
    of_object_t cmd_tlv;
};

/* Debug counters */
DEBUG_COUNTER(parse_failure_counter, "macblaster.parse_failure",
              "Failed to parse message from controller");
DEBUG_COUNTER(pktout_failure_counter, "macblaster.pktout_failure",
              "Failed to sent packet-outs");

static void
macblaster_send_packet(of_port_no_t of_port,
                       uint16_t vlan_vid,
                       of_mac_addr_t eth_src)
{
    indigo_error_t rv;
    ppe_packet_t *ppep;
    of_octets_t octets;

    octets.bytes = MACBLASTER_PKTLEN;
    octets.data = (vlan_vid) ? macblaster_tagged_pkt : macblaster_untagged_pkt;
    ppep = (vlan_vid) ? &ppe_tagged_pkt : &ppe_untagged_pkt;

    if (ppe_wide_field_set(ppep, PPE_FIELD_ETHERNET_SRC_MAC,
                           eth_src.addr) < 0) {
        AIM_DIE("Failed to set PPE_FIELD_ETHERNET_SRC_MAC");
    }

    if (vlan_vid) {
        if (ppe_field_set(ppep, PPE_FIELD_8021Q_VLAN, vlan_vid) < 0) {
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
    bool parse_error = false;

    while (!of_list_bsn_tlv_next(&state->cmd_tlvs, &state->cmd_tlv)) {
        if (state->cmd_tlv.object_id != OF_BSN_TLV_BUCKET) {
            parse_error = true;
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected bucket TLV, instead got %s",
                          __FUNCTION__, of_class_name(&state->cmd_tlv));
            goto done;
        }

        of_bsn_tlv_bucket_value_bind(&state->cmd_tlv, &bucket_tlvs);

        if (of_list_bsn_tlv_first(&bucket_tlvs, &bucket_tlv) < 0) {
            parse_error = true;
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got end of list",
                          __FUNCTION__);
            goto done;
        }

        if (bucket_tlv.object_id == OF_BSN_TLV_VLAN_VID) {
            of_bsn_tlv_vlan_vid_value_get(&bucket_tlv, &vlan_vid);
        } else {
            parse_error = true;
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got %s",
                          __FUNCTION__, of_class_name(&bucket_tlv));
            goto done;
        }

        if (of_list_bsn_tlv_next(&bucket_tlvs, &bucket_tlv) < 0) {
            parse_error = true;
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected mac TLV, instead got end of list",
                          __FUNCTION__);
            goto done;
        }

        if (bucket_tlv.object_id == OF_BSN_TLV_MAC) {
            of_bsn_tlv_mac_value_get(&bucket_tlv, &mac);
        } else {
            parse_error = true;
            debug_counter_inc(&parse_failure_counter);
            AIM_LOG_ERROR("%s: expected mac TLV, instead got %s",
                          __FUNCTION__, of_class_name(&bucket_tlv));
            goto done;
        }

        if (of_list_bsn_tlv_next(&bucket_tlvs, &bucket_tlv) == 0) {
            parse_error = true;
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
    if (parse_error) {
        indigo_cxn_send_bsn_error(state->cxn_id, state->cmd, "Error parsing command");
    }

    indigo_cxn_resume(state->cxn_id);

    of_object_delete(state->cmd);
    aim_free(state);

    return IND_SOC_TASK_FINISHED;
}

static indigo_core_listener_result_t
macblaster_handle_flexlink_command(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    of_str64_t command_name;
    struct macblaster_state *state;

    of_bsn_generic_command_name_get(msg, &command_name);

    if (strcmp(command_name, "flexlink")) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    state = aim_zmalloc(sizeof(*state));
    state->cxn_id = cxn_id;
    state->cmd = of_object_dup(msg);
    if (state->cmd == NULL) {
        AIM_LOG_ERROR("%s: Failed to allocate the command", __FUNCTION__);
        goto error;
    }

    /* Get command TLVs and parse port number */
    of_bsn_generic_command_tlvs_bind(state->cmd, &state->cmd_tlvs);

    if (of_list_bsn_tlv_first(&state->cmd_tlvs, &state->cmd_tlv) < 0) {
        debug_counter_inc(&parse_failure_counter);
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        AIM_LOG_ERROR("%s: expected port TLV, instead got end of list",
                      __FUNCTION__);
        goto error;
    }

    if (state->cmd_tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&state->cmd_tlv, &state->of_port);
    } else {
        debug_counter_inc(&parse_failure_counter);
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        AIM_LOG_ERROR("%s: expected port TLV, instead got %s",
                      __FUNCTION__, of_class_name(&state->cmd_tlv));
        goto error;
    }

    /* Start long running task which parses VLAN, MAC buckets
     * and sends packet-out messages */
    if (ind_soc_task_register(macblaster_task, state, IND_SOC_LOW_PRIORITY) < 0) {
        AIM_DIE("Failed to create macblaster long running task");
    }

    indigo_cxn_pause(cxn_id);

    return INDIGO_CORE_LISTENER_RESULT_DROP;

error:

    if (state) {
        if (state->cmd) of_object_delete(state->cmd);
        aim_free(state);
    }

    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

static indigo_core_listener_result_t
macblaster_message_listener(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    switch (msg->object_id) {
        case OF_BSN_GENERIC_COMMAND:
            return macblaster_handle_flexlink_command(cxn_id, msg);

        default:
            return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}

static void
macblaster_pkt_init(void)
{
    of_mac_addr_t eth_dst = {{0x01, 0x00, 0x0D, 0xCC, 0xCC, 0xCC}};

    MACBLASTER_MEMCPY(macblaster_tagged_pkt, &eth_dst, sizeof(eth_dst));
    macblaster_tagged_pkt[12] = 0x81;
    macblaster_tagged_pkt[13] = 0x00;
    macblaster_tagged_pkt[16] = 0x88;  /* Experimental type */
    macblaster_tagged_pkt[17] = 0xb5;
    ppe_packet_init(&ppe_tagged_pkt,
                    macblaster_tagged_pkt,
                    sizeof(macblaster_tagged_pkt));

    if (ppe_parse(&ppe_tagged_pkt) < 0) {
        AIM_DIE("macblaster_pkt_init parsing tagged_pkt failed");
    }

    MACBLASTER_MEMCPY(macblaster_untagged_pkt, &eth_dst, sizeof(eth_dst));
    macblaster_untagged_pkt[12] = 0x88;  /* Experimental type */
    macblaster_untagged_pkt[13] = 0xb5;
    ppe_packet_init(&ppe_untagged_pkt,
                    macblaster_untagged_pkt,
                    sizeof(macblaster_untagged_pkt));

    if (ppe_parse(&ppe_untagged_pkt) < 0) {
        AIM_DIE("macblaster_pkt_init parsing untagged_pkt failed");
    }

}

indigo_error_t
macblaster_init(void)
{
    if (macblaster_initialized) return INDIGO_ERROR_NONE;

    indigo_core_message_listener_register(macblaster_message_listener);

    macblaster_pkt_init();

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

