/*
 * Copyright 2016, Big Switch Networks, Inc.
 *
 * general query tx generic_command handling.
 */

/* Implements generic_command handler that sends general queries
 * on the specified output port for all specified vlans, using
 * the given source mac. */


#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <SocketManager/socketmanager.h>
#include <PPE/ppe.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared.h>
#include <slshared/slshared_config.h>

#include "igmpa_int.h"
#include "gq_tx_gencmd.h"


DEBUG_COUNTER(gq_tx_gencmd, "igmpa.gq_tx_gencmd",
              "gq_tx_gencmd received");
DEBUG_COUNTER(gq_tx_gencmd_parse_failure, "igmpa.gq_tx_gencmd_parse_failure",
              "gq_tx_gencmd parsing failed");
DEBUG_COUNTER(gq_tx_gencmd_tx, "igmpa.gq_tx_gencmd_tx",
              "gq_tx_gencmd tx count");
DEBUG_COUNTER(gq_tx_gencmd_tx_failure, "igmpa.gq_tx_gencmd_tx_failure",
              "gq_tx_gencmd tx failure");


static void
send_gq(of_port_no_t outport, uint8_t src_mac[], uint16_t vlan_vid)
{
    uint8_t dst_mac[OF_MAC_ADDR_BYTES] =  /* L2 multicast MAC for 224.0.0.1 */
        { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };

    igmpa_pkt_params_t params = {
        .eth_src = src_mac,
        .eth_dst = dst_mac,
        .vlan_vid = vlan_vid,
        .ipv4_src = 0, /* 0.0.0.0 */
        .ipv4_dst = 0xe0000001, /* 224.0.0.1 */
        .igmp_type = PPE_IGMP_TYPE_QUERY,
        .igmp_max_resp_time = 100,  /* 10s */
        .igmp_group_addr = 0,  /* field is cleared */
        .output_port_no = outport,
    };

    if (igmpa_send_igmp_packet(&params)) {
        debug_counter_inc(&gq_tx_gencmd_tx_failure);
    } else {
        debug_counter_inc(&gq_tx_gencmd_tx);
    }
}


/* state for long running task */
struct gq_tx_gencmd_state {
    indigo_cxn_id_t cxn_id;     /* cxn to resume / send error message */
    of_port_no_t outport;       /* ofport for all general queries */
    of_mac_addr_t eth_src;      /* src mac for all general queries */
    of_object_t *cmd;           /* original generic command message */
    of_list_bsn_tlv_t cmd_tlvs; /* all gencmd tlvs */
    of_object_t cmd_tlv;        /* current tlv in cmd_tlvs */
};

/* long running task to send general queries */
static ind_soc_task_status_t
gq_tx_gencmd_task(void *cookie)
{
    struct gq_tx_gencmd_state *state = cookie;
    uint16_t vlan_vid;
    bool parse_error = false;

    while (of_list_bsn_tlv_next(&state->cmd_tlvs,
                                &state->cmd_tlv) == OF_ERROR_NONE) {
        if (state->cmd_tlv.object_id != OF_BSN_TLV_VLAN_VID) {
            parse_error = true;
            debug_counter_inc(&gq_tx_gencmd_parse_failure);
            AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got %s",
                          __FUNCTION__, of_class_name(&state->cmd_tlv));
            goto done;
        }

        of_bsn_tlv_vlan_vid_value_get(&state->cmd_tlv, &vlan_vid);

        send_gq(state->outport, state->eth_src.addr, vlan_vid);

        if (ind_soc_should_yield()) {
            return IND_SOC_TASK_CONTINUE;
        }
    }

done:
    if (parse_error) {
        indigo_cxn_send_bsn_error(state->cxn_id, state->cmd,
                                  "Error parsing command");
    }

    indigo_cxn_resume(state->cxn_id);

    of_object_delete(state->cmd);
    aim_free(state);

    return IND_SOC_TASK_FINISHED;
}

/* parses generic_command and launches long-running task to send gq */
static indigo_core_listener_result_t
handle_gq_tx_gencmd(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    of_str64_t command_name;
    struct gq_tx_gencmd_state *state;

    of_bsn_generic_command_name_get(msg, &command_name);

    if (strcmp(command_name, "igmp_general_query_tx")) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    debug_counter_inc(&gq_tx_gencmd);

    state = aim_zmalloc(sizeof(*state));
    state->cxn_id = cxn_id;
    state->cmd = of_object_dup(msg);
    if (state->cmd == NULL) {
        AIM_LOG_ERROR("%s: failed to copy gencmd", __FUNCTION__);
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error handling command");
        goto error;
    }

    of_bsn_generic_command_tlvs_bind(state->cmd, &state->cmd_tlvs);

    if (of_list_bsn_tlv_first(&state->cmd_tlvs, &state->cmd_tlv) < 0) {
        debug_counter_inc(&gq_tx_gencmd_parse_failure);
        AIM_LOG_ERROR("%s: expected port TLV, instead got end of list",
                      __FUNCTION__);
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        goto error;
    }
    if (state->cmd_tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&state->cmd_tlv, &state->outport);
        if (state->outport > SLSHARED_CONFIG_OF_PORT_MAX) {
            debug_counter_inc(&gq_tx_gencmd_parse_failure);
            AIM_LOG_ERROR("%s: port %u exceeds maximum %u",
                          __FUNCTION__, state->outport,
                          SLSHARED_CONFIG_OF_PORT_MAX);
            indigo_cxn_send_bsn_error(cxn_id, msg, "Invalid openflow port");
            goto error;
        }
    } else {
        debug_counter_inc(&gq_tx_gencmd_parse_failure);
        AIM_LOG_ERROR("%s: expected port TLV, instead got %s",
                      __FUNCTION__, of_class_name(&state->cmd_tlv));
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        goto error;
    }
    if (of_list_bsn_tlv_next(&state->cmd_tlvs, &state->cmd_tlv) < 0) {
        debug_counter_inc(&gq_tx_gencmd_parse_failure);
        AIM_LOG_ERROR("%s: expected eth_src TLV, instead got end of list",
                      __FUNCTION__);
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        goto error;
    }
    if (state->cmd_tlv.object_id == OF_BSN_TLV_ETH_SRC) {
        of_bsn_tlv_eth_src_value_get(&state->cmd_tlv, &state->eth_src);
    } else {
        debug_counter_inc(&gq_tx_gencmd_parse_failure);
        AIM_LOG_ERROR("%s: expected eth_src TLV, instead got %s",
                      __FUNCTION__, of_class_name(&state->cmd_tlv));
        indigo_cxn_send_bsn_error(cxn_id, msg, "Error parsing command");
        goto error;
    }

    /* Start long running task that walks rest of cmd
     * and sends general queries */
    if (ind_soc_task_register(gq_tx_gencmd_task, state,
                              IND_SOC_LOW_PRIORITY) < 0) {
        AIM_DIE("Failed to create gq_tx_gencmd long running task");
    }
    indigo_cxn_pause(cxn_id);

    return INDIGO_CORE_LISTENER_RESULT_DROP;

 error:
    if (state) {
        if (state->cmd) {
            of_object_delete(state->cmd);
        }
        aim_free(state);
    }
    return INDIGO_CORE_LISTENER_RESULT_DROP;
}


static indigo_core_listener_result_t
gencmd_listener(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    switch (msg->object_id) {
        case OF_BSN_GENERIC_COMMAND:
            return handle_gq_tx_gencmd(cxn_id, msg);

        default:
            return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}


void
igmpa_gq_tx_gencmd_stats_clear(void)
{
    debug_counter_reset(&gq_tx_gencmd);
    debug_counter_reset(&gq_tx_gencmd_parse_failure);
    debug_counter_reset(&gq_tx_gencmd_tx);
    debug_counter_reset(&gq_tx_gencmd_tx_failure);
}

void
igmpa_gq_tx_gencmd_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "gq_tx_gencmd  %"PRIu64"\n",
               debug_counter_get(&gq_tx_gencmd));
    aim_printf(pvs, "gq_tx_gencmd_parse_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_gencmd_parse_failure));
    aim_printf(pvs, "gq_tx_gencmd_tx  %"PRIu64"\n",
               debug_counter_get(&gq_tx_gencmd_tx));
    aim_printf(pvs, "gq_tx_gencmd_tx_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_gencmd_tx_failure));
}

void
igmpa_gq_tx_gencmd_init(void)
{
    indigo_core_message_listener_register(gencmd_listener);
}

void
igmpa_gq_tx_gencmd_finish(void)
{
    indigo_core_message_listener_unregister(gencmd_listener);
}

