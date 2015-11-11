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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <slshared/slshared.h>

#define AIM_LOG_MODULE_NAME pdua
#include <AIM/aim_log.h>

#include <pdua/pdua_config.h>
#include <pdua/pdua_porting.h>
#include <pdua/pdua.h>
#include "pdua_int.h"

#include <BigList/biglist.h>

#define PDU_SLOT_NUM  2

#define PDUA_DEBUG(fmt, ...)                       \
            AIM_LOG_TRACE(fmt, ##__VA_ARGS__)

static indigo_error_t pdua_pkt_data_set(pdua_pkt_t *lpkt, of_octets_t *data);
static void pdua_pkt_data_free(pdua_pkt_t *lpkt);

static indigo_error_t pdua_port_disable(ind_soc_timer_callback_f cb,
                                        pdua_pkt_t *pkt, pdua_port_t *port);

static indigo_error_t pdua_port_enable(ind_soc_timer_callback_f cb,
                                       pdua_pkt_t *pkt, pdua_port_t *port,
                                       of_octets_t *data, uint32_t interval_ms,
                                       ind_soc_priority_t prio);

static void pdua_disable_tx_rx(pdua_port_t *pdua);
static void pdu_timeout_rx(void *cookie);
static void pdu_periodic_tx(void *cookie);
static void rx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *rx_req);
static void tx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *rx_req);

static void pdu_periodic_tx(void *cookie);
static void pdu_timeout_rx(void *cookie);

static biglist_t *pkt_event_listener_list;

pdua_system_t pdua_port_sys;

static void
pkt_state_change_handle(pdua_port_t *port, pdua_packet_state_t new_pkt_state)
{
    biglist_t *ble;
    pdua_pkt_event_listener_callback_f fn;

    if (port->pkt_state != new_pkt_state) {
        PDUA_DEBUG("%s: port %u pkt state change: "
                   "%{pdua_packet_state} -> %{pdua_packet_state}",
                   __FUNCTION__, port->port_no, port->pkt_state, new_pkt_state);
        BIGLIST_FOREACH_DATA(ble, pkt_event_listener_list,
                             pdua_pkt_event_listener_callback_f,
                             fn) {
            fn(port->port_no, new_pkt_state);
        }
    }
    port->pkt_state = new_pkt_state;
}

pdua_port_t *
pdua_find_port(of_port_no_t port_no)
{
    pdua_port_t *ret = NULL;
    if (port_no < pdua_port_sys.pdua_total_of_ports) {
        ret = &pdua_port_sys.pdua_ports[port_no];
    }

    return ret;
}

/*
 * data.data must be NULL
 */
static indigo_error_t
pdua_pkt_data_set(pdua_pkt_t *lpkt, of_octets_t *data)
{
    if (!lpkt || !data) {
        return INDIGO_ERROR_PARAM;
    }

    if (lpkt->data.data) {
        return INDIGO_ERROR_PARAM;
    }

    lpkt->data.data = PDUA_MALLOC(data->bytes);
    if (!lpkt->data.data) {
        return INDIGO_ERROR_RESOURCE;
    }

    lpkt->data.bytes = data->bytes;
    PDUA_MEMCPY(lpkt->data.data, data->data, data->bytes);

    return INDIGO_ERROR_NONE;
}

/* free data and reset bytes */
static void
pdua_pkt_data_free(pdua_pkt_t *lpkt)
{
    if (lpkt) {
        if (lpkt->data.data) {
            PDUA_FREE(lpkt->data.data);
            lpkt->data.data = NULL;
            lpkt->data.bytes = 0;
        }
    }
}

static indigo_error_t
pdua_port_disable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt,
                  pdua_port_t *port)
{
    indigo_error_t rv;

    rv = ind_soc_timer_event_unregister(cb, port);
    if (rv == INDIGO_ERROR_NONE) {
        pkt->interval_ms = 0;
        pdua_pkt_data_free(pkt);
    } else {
        AIM_LOG_ERROR("%s: failed to unregister timer: %s",
                      __FUNCTION__, indigo_strerror(rv));
    }
    return rv;
}

static indigo_error_t
pdua_port_enable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt,
                 pdua_port_t *port, of_octets_t *data, uint32_t interval_ms,
                 ind_soc_priority_t prio)
{
    indigo_error_t rv;

    rv = pdua_pkt_data_set(pkt, data);
    if (rv == INDIGO_ERROR_NONE) {
        rv = ind_soc_timer_event_register_with_priority(cb, port, interval_ms,
                                                        prio);
        if (rv == INDIGO_ERROR_NONE) {
            pkt->interval_ms = interval_ms;
        } else {
            AIM_LOG_ERROR("%s: failed to register timer: %s",
                          __FUNCTION__, indigo_strerror(rv));
            pdua_pkt_data_free(pkt);
        }
    } else {
        AIM_LOG_ERROR("%s: failed to set pkt data: %s",
                      __FUNCTION__, indigo_strerror(rv));
    }
    return rv;
}

/* Unregister timer and free data */
static void
pdua_disable_tx_rx(pdua_port_t *port)
{
    indigo_error_t rv;

    if (!port) {
        return;
    }

    if (port->tx_pkt.interval_ms) {
        rv = pdua_port_disable(pdu_periodic_tx, &port->tx_pkt, port);
        if (rv != INDIGO_ERROR_NONE) {
            AIM_LOG_ERROR("%s: failed to disable port %u tx: %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
        }
    }

    if (port->rx_pkt.interval_ms) {
        rv = pdua_port_disable(pdu_timeout_rx, &port->rx_pkt, port);
        if (rv != INDIGO_ERROR_NONE) {
            AIM_LOG_ERROR("%s: failed to disable port %u rx: %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
        }
    }
}

static void
pdu_timeout_rx(void *cookie)
{
    uint32_t version;
    pdua_port_t *port = (pdua_port_t *)cookie;
    of_bsn_pdu_rx_timeout_t *timeout_msg = NULL;

    if (!port) {
        return;
    }

    /* It is a packet MISS if we reach this point */
    pkt_state_change_handle(port, PDUA_PACKET_STATE_MISS);

    if (indigo_cxn_get_async_version(&version) != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: no controller connected", __FUNCTION__);
        return;
    }

    timeout_msg = of_bsn_pdu_rx_timeout_new(version);
    if (!timeout_msg){
        AIM_LOG_INTERNAL("%s: failed to allocate timeout msg", __FUNCTION__);
        return;
    }

    /* Set port number */
    of_bsn_pdu_rx_timeout_port_no_set(timeout_msg, port->port_no);

    /* Set slot number */
    of_bsn_pdu_rx_timeout_slot_num_set(timeout_msg, PDU_SLOT_NUM);

    PDUA_DEBUG("%s: sending rx timeout async msg", __FUNCTION__);
    /* Send to controller, don't delete when send to controller */
    indigo_cxn_send_async_message(timeout_msg);

    port->timeout_pkt_cnt++;
}

static void
pdu_periodic_tx(void *cookie)
{
    pdua_port_t *port = (pdua_port_t *)cookie;
    indigo_error_t rv;

    if (!port) {
       return;
    }

    PDUA_DEBUG("%s: fwd tx pkt out on port %u", __FUNCTION__, port->port_no);

    rv = slshared_fwd_packet_out(&port->tx_pkt.data, 0, port->port_no,
                                 SLSHARED_CONFIG_PDU_QUEUE_PRIORITY);
    if (rv < 0) {
        AIM_LOG_INTERNAL("%s: failed to fwd tx pkt out on port %u: %s",
                         __FUNCTION__, port->port_no, indigo_strerror(rv));
    } else {
        port->tx_pkt_out_cnt++;
    }
}

static void
rx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *rx_req)
{
    pdua_port_t *port = NULL;
    indigo_error_t rv;

    /* rx_req info */
    uint32_t xid;
    of_port_no_t port_no;
    uint32_t timeout_ms;
    of_octets_t data;
    of_bsn_pdu_rx_reply_t *rx_reply = NULL;
    uint32_t status_failed = 0;

    /* Get rx req info */
    of_bsn_pdu_rx_request_xid_get(rx_req, &xid);
    of_bsn_pdu_rx_request_timeout_ms_get(rx_req, &timeout_ms);
    of_bsn_pdu_rx_request_data_get(rx_req, &data);
    of_bsn_pdu_rx_request_port_no_get(rx_req, &port_no);

    if (timeout_ms && !data.data) {
        status_failed = 1;
        AIM_LOG_ERROR("%s: Req_Rx Port %u, timeout_ms is set with no pkt data",
                      __FUNCTION__, port_no);
        goto rx_reply_to_ctrl;
    }

    port = pdua_find_port(port_no);
    if (port == NULL) {
        status_failed = 1;
        AIM_LOG_ERROR("%s: Port %u doesn't exist", __FUNCTION__, port_no);
        goto rx_reply_to_ctrl;
    }

    port->rx_req_cnt++;

    /* 1. Clean up old rx_pkt configuration if it exists */
    if (port->rx_pkt.interval_ms) {
        rv = pdua_port_disable(pdu_timeout_rx, &port->rx_pkt, port);
        if (rv != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("%s: Port rx %u failed to disable %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
            goto rx_reply_to_ctrl;
        }

        /* Reset packet state */
        pkt_state_change_handle(port, PDUA_PACKET_STATE_UNKNOWN);
    }

    AIM_TRUE_OR_DIE(!port->rx_pkt.interval_ms && !port->rx_pkt.data.data);

    /* 2. Set up new rx_pkt, timer */
    if (timeout_ms) {
        rv = pdua_port_enable(pdu_timeout_rx, &port->rx_pkt, port, &data,
                              timeout_ms, PDUA_CONFIG_RX_TIMEOUT_PRIO);
        if (rv != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("%s: Port rx %u failed to enable %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
        }
    }

rx_reply_to_ctrl:
    /* 3. Setup reply */
    rx_reply = of_bsn_pdu_rx_reply_new(rx_req->version);
    if (!rx_reply) {
        AIM_LOG_INTERNAL("%s: failed to allocate rx_reply", __FUNCTION__);
        return;
    }
    of_bsn_pdu_rx_reply_xid_set(rx_reply, xid);
    of_bsn_pdu_rx_reply_port_no_set(rx_reply, port_no);
    of_bsn_pdu_rx_reply_status_set(rx_reply, status_failed);
    of_bsn_pdu_rx_reply_slot_num_set(rx_reply, PDU_SLOT_NUM);

    PDUA_DEBUG("%s: Port %u: sending a RX_reply to ctrl, status %s, version %u",
               __FUNCTION__, port_no, status_failed? "Failed" : "Success",
               rx_req->version);
    /* 4. Send to controller, don't delete obj */
    indigo_cxn_send_controller_message(cxn_id, rx_reply);
}


static void
tx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *tx_req)
{
    pdua_port_t *port = NULL;

    indigo_error_t rv;

    /* tx_req info */
    uint32_t xid;
    of_port_no_t port_no;
    uint32_t tx_interval_ms;
    of_octets_t data;

    /* tx_reply info */
    of_bsn_pdu_tx_reply_t *tx_reply = NULL;
    uint32_t status_failed = 0;

    /* Get tx req info */
    of_bsn_pdu_tx_request_xid_get(tx_req, &xid);
    of_bsn_pdu_tx_request_tx_interval_ms_get(tx_req, &tx_interval_ms);
    of_bsn_pdu_tx_request_data_get(tx_req, &data);
    of_bsn_pdu_tx_request_port_no_get(tx_req, &port_no);

    if (tx_interval_ms && !data.data) {
        status_failed = 1;
        AIM_LOG_ERROR("%s: Req_Tx Port %u, "
                      "tx_interval_ms is set with no pkt data",
                      __FUNCTION__, port_no);
        goto tx_reply_to_ctrl;
    }

    port = pdua_find_port(port_no);
    if (port == NULL) {
        status_failed = 1;
        AIM_LOG_ERROR("%s: Port %u doesn't exist", __FUNCTION__, port_no);
        goto tx_reply_to_ctrl;
    }

    port->tx_req_cnt++;

    /* 1. Clean up old tx_pkt configuration if it exists */
    if (port->tx_pkt.interval_ms) {
        rv = pdua_port_disable(pdu_periodic_tx, &port->tx_pkt, port);
        if (rv != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("%s: Port tx %u failed to disable %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
            goto tx_reply_to_ctrl;
        }
    }

    AIM_TRUE_OR_DIE(!port->tx_pkt.interval_ms && !port->tx_pkt.data.data);

    /* 2. Set up new tx_pkt, alarm */
    if (tx_interval_ms) {
        rv = pdua_port_enable(pdu_periodic_tx, &port->tx_pkt, port, &data,
                              tx_interval_ms, PDUA_CONFIG_TX_PRIO);
        if (rv == INDIGO_ERROR_NONE) {
            /* Successfully enable, send one out immediately */
            pdu_periodic_tx(port);
        } else {
            status_failed = 1;
            AIM_LOG_ERROR("%s: Port tx %u failed to enable %s",
                          __FUNCTION__, port->port_no, indigo_strerror(rv));
        }
    }

tx_reply_to_ctrl:
    /* 3. Setup reply  */
    tx_reply = of_bsn_pdu_tx_reply_new(tx_req->version);
    if (!tx_reply) {
        AIM_LOG_INTERNAL("%s: failed to allocate tx reply", __FUNCTION__);
        return;
    }

    of_bsn_pdu_tx_reply_xid_set(tx_reply, xid);
    of_bsn_pdu_tx_reply_port_no_set(tx_reply, port_no);
    of_bsn_pdu_tx_reply_status_set(tx_reply, status_failed);
    of_bsn_pdu_tx_reply_slot_num_set(tx_reply, PDU_SLOT_NUM);

    PDUA_DEBUG("%s: Port %u: sending a TX_reply to ctrl, status %s, version %u",
               __FUNCTION__, port_no, status_failed? "Failed" : "Success",
               tx_req->version);
    /* 4. Send to controller, don't delete obj */
    indigo_cxn_send_controller_message(cxn_id, tx_reply);
}

/* Register to listen to CTRL msg */
indigo_core_listener_result_t
pdua_handle_msg(indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    indigo_core_listener_result_t ret = INDIGO_CORE_LISTENER_RESULT_PASS;
    uint8_t slot_num;

    if(!msg) {
        return ret;
    }

    switch (msg->object_id) {

    case OF_BSN_PDU_RX_REQUEST:
        of_bsn_pdu_rx_request_slot_num_get(msg, &slot_num);
        if (slot_num != PDU_SLOT_NUM) {
            PDUA_DEBUG("%s: Received rx request with slot_num: %u, ignoring",
                       __FUNCTION__, slot_num);
            return ret;
        }

        /* Count msg in */
        debug_counter_inc(&pdua_port_sys.debug_info.total_msg_in_cnt);
        rx_request_handle(cxn_id, msg);
        ret = INDIGO_CORE_LISTENER_RESULT_DROP;
        break;

    case OF_BSN_PDU_TX_REQUEST:
        of_bsn_pdu_tx_request_slot_num_get(msg, &slot_num);
        if (slot_num != PDU_SLOT_NUM) {
            PDUA_DEBUG("%s: Received tx request with slot_num: %u, ignoring",
                       __FUNCTION__, slot_num);
            return ret;
        }

        /* Count msg in */
        debug_counter_inc(&pdua_port_sys.debug_info.total_msg_in_cnt);
        tx_request_handle(cxn_id, msg);
        ret = INDIGO_CORE_LISTENER_RESULT_DROP;
        break;

    default:
        break;
    }

    return ret;
}


/*****************
 * HANDLE PKT IN *
 *****************/

/*
 * Caller must ensure port != NULL, data != NULL
 * return 1 if pkt is expected
 */
static int
pdua_rx_pkt_is_expected(pdua_port_t *port, of_octets_t *data)
{
    int ret = 0;

    if (!port->rx_pkt.data.data) {
        PDUA_DEBUG("%s: Port %u: MISMATCHED RX no data",
                   __FUNCTION__, port->port_no);
        port->rx_pkt_mismatched_no_data++;
        return ret;
    }

    if (port->rx_pkt.data.bytes != data->bytes) {
        PDUA_DEBUG("%s: Port %u: MISMATCHED len exp=%u, rcv=%u",
                   __FUNCTION__, port->port_no, port->rx_pkt.data.bytes,
                   data->bytes);
        port->rx_pkt_mismatched_len++;
        return ret;
    }

    if (memcmp(port->rx_pkt.data.data, data->data, data->bytes) == 0) {
        PDUA_DEBUG("%s: Port %u: MATCHED", __FUNCTION__, port->port_no);
        ret = 1;
        port->rx_pkt_matched++;
    } else {
        PDUA_DEBUG("%s: Port %u: MISMATCHED data", __FUNCTION__, port->port_no);
        port->rx_pkt_mismatched_data++;
    }

    return ret;
}

/*
 * Caller must ensure pdua != NULL
 * Reset timeout
 */
static void
pdua_update_rx_timeout(pdua_port_t *port)
{
    indigo_error_t rv;
    PDUA_DEBUG("%s: Using reset timer", __FUNCTION__);

    rv = ind_soc_timer_event_register_with_priority(pdu_timeout_rx, port,
        port->rx_pkt.interval_ms, PDUA_CONFIG_RX_TIMEOUT_PRIO);

    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: Port %u failed to register %s",
                      __FUNCTION__, port->port_no, indigo_strerror(rv));
    }
}

/*
 * This api can be used to send a pdu packet directly to the agent
 */
indigo_core_listener_result_t
pdua_receive_packet(of_octets_t *data, of_port_no_t port_no)
{
    if (port_no == OF_PORT_DEST_CONTROLLER) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    pdua_port_t *port = pdua_find_port(port_no);
    if (!port) {
        AIM_LOG_INTERNAL("%s: PDUA port out of range %u",
                         __FUNCTION__, port_no);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    port->rx_pkt_in_cnt++;
    if (port->dump_enabled) {
        PDUA_DEBUG("%s: PDUA_DATA_HEXDUMP:\n%{data}\n",
                   __FUNCTION__, data->data, data->bytes);
    }

    /* At this step we will process the PDU packet
     * 0. Port doesn't have data, won't expect any packet
     * 1. If expected, reset the timeout
     * 2. If not, it's automatically PASSED to the controller
     *    as a packet-in
     */
    if (!pdua_rx_pkt_is_expected(port, data)) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* It is a packet HIT if we reach this point */
    pkt_state_change_handle(port, PDUA_PACKET_STATE_HIT);

    pdua_update_rx_timeout(port);
    debug_counter_inc(&pdua_port_sys.debug_info.total_pkt_exp_cnt);
    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

/* Register to listen to PACKETIN msg */
indigo_core_listener_result_t
pdua_handle_pkt(of_packet_in_t *packet_in)
{
    of_octets_t data;
    of_port_no_t port_no;
    of_match_t match;

    if (!packet_in) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* Data is the ether pkt */
    of_packet_in_data_get(packet_in, &data);

    if (!data.data) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* Only count pkt_in with valid data */
    debug_counter_inc(&pdua_port_sys.debug_info.total_pkt_in_cnt);

    if (packet_in->version <= OF_VERSION_1_1) {
        of_packet_in_in_port_get(packet_in, &port_no);
        PDUA_DEBUG("%s: port %u pkt in version %d",
                   __FUNCTION__, port_no, packet_in->version);
    } else {
        if (of_packet_in_match_get(packet_in, &match) < 0) {
            AIM_LOG_INTERNAL("%s: match get failed", __FUNCTION__);
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }
        port_no = match.fields.in_port;
        PDUA_DEBUG("%s: Port %u", __FUNCTION__, port_no);
    }

    return pdua_receive_packet(&data, port_no);
}


/************************
 * PDUA INIT and FINISH
 ************************/

static void
pdua_register_system_counters(void)
{
    debug_counter_register(&pdua_port_sys.debug_info.total_pkt_in_cnt,
                           "pdua.total_pkt_in_cnt",
                           "Packet-ins recv'd by pdua");
    debug_counter_register(&pdua_port_sys.debug_info.total_msg_in_cnt,
                           "pdua.total_msg_in_cnt",
                           "OF messages recv'd by pdua");
    debug_counter_register(&pdua_port_sys.debug_info.total_pkt_exp_cnt,
                           "pdua.total_pkt_exp_cnt",
                           "Expected packets recv'd by pdua");
}

static void
pdua_unregister_system_counters(void)
{
    debug_counter_unregister(&pdua_port_sys.debug_info.total_pkt_in_cnt);
    debug_counter_unregister(&pdua_port_sys.debug_info.total_msg_in_cnt);
    debug_counter_unregister(&pdua_port_sys.debug_info.total_pkt_exp_cnt);
}

/* Return 0: success */
int
pdua_system_init(void)
{
    int i;
    pdua_port_t *port;

    AIM_LOG_VERBOSE("init");

    pdua_port_sys.pdua_total_of_ports = AIM_ARRAYSIZE(pdua_port_sys.pdua_ports);
    for (i = 0; i < pdua_port_sys.pdua_total_of_ports; i++) {
        port = pdua_find_port(i);
        if (port) {
            port->port_no = i;
            port->dump_enabled = false;
            port->pkt_state = PDUA_PACKET_STATE_UNKNOWN;
        } else {
            AIM_LOG_INTERNAL("%s: Port %d not existing", __FUNCTION__, i);
        }
    }

    indigo_core_message_listener_register(pdua_handle_msg);
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_register(pdua_handle_pkt);
#endif

    pdua_register_system_counters();

    return 0;
}

void
pdua_system_finish(void)
{
    int i;
    pdua_port_t *port;

    indigo_core_message_listener_unregister(pdua_handle_msg);
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_unregister(pdua_handle_pkt);
#endif

    for (i = 0; i < pdua_port_sys.pdua_total_of_ports; i++) {
        port = pdua_find_port(i);
        if (port) {
            pdua_disable_tx_rx(port);
        } else {
            AIM_LOG_INTERNAL("%s: Port %d not existing", __FUNCTION__, i);
        }
    }

    pdua_unregister_system_counters();
}

indigo_error_t
pdua_port_dump_enable_set(of_port_no_t port_no, bool enabled)
{
    pdua_port_t *port;

    port = pdua_find_port(port_no);
    if (port != NULL) {
        port->dump_enabled = enabled;
    } else {
        AIM_LOG_ERROR("%s: Port %u not found", __FUNCTION__, port_no);
        return INDIGO_ERROR_NOT_FOUND;
    }

    return INDIGO_ERROR_NONE;
}

indigo_error_t
pdua_port_dump_enable_get(of_port_no_t port_no, bool *enabled)
{
    pdua_port_t *port;

    port = pdua_find_port(port_no);
    if (port != NULL) {
        *enabled = port->dump_enabled;
    } else {
        AIM_LOG_ERROR("%s: Port %u not found", __FUNCTION__, port_no);
        return INDIGO_ERROR_NOT_FOUND;
    }

    return INDIGO_ERROR_NONE;
}

indigo_error_t
pdua_pkt_event_listener_register(pdua_pkt_event_listener_callback_f fn)
{
    if (biglist_find(pkt_event_listener_list, fn)) {
        AIM_LOG_ERROR("%s: listener is already registered", __FUNCTION__);
        return INDIGO_ERROR_EXISTS;
    }

    pkt_event_listener_list = biglist_append(pkt_event_listener_list, fn);
    return INDIGO_ERROR_NONE;
}

void
pdua_pkt_event_listener_unregister(pdua_pkt_event_listener_callback_f fn)
{
    pkt_event_listener_list = biglist_remove(pkt_event_listener_list, fn);
}
