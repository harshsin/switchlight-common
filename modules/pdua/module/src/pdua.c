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

#define PDU_SLOT_NUM  2

#define PDUA_DEBUG(fmt, ...)                       \
            AIM_LOG_TRACE(fmt, ##__VA_ARGS__)
static indigo_error_t  pdua_pkt_data_set(pdua_pkt_t *lpkt, of_octets_t *data);
static void pdua_pkt_data_free (pdua_pkt_t *lpkt);
static indigo_error_t pdua_port_disable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt, pdua_port_t *port);
static indigo_error_t pdua_port_enable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt, pdua_port_t *port,
                                       of_octets_t *data, uint32_t interval_ms);
static void  pdua_disable_tx_rx(pdua_port_t *pdua);
static void pdu_timeout_rx(void *cookie);
static void pdu_periodic_tx(void *cookie);
static void rx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *rx_req);
static void tx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *rx_req);

static void pdu_periodic_tx(void *cookie);
static void pdu_timeout_rx(void *cookie);


pdua_system_t pdua_port_sys;
int           pdua_dump_data = PDUA_DUMP_DISABLE_ALL_PORTS;

pdua_port_t*
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
 * Ret 0 for success
 * */
static indigo_error_t
pdua_pkt_data_set(pdua_pkt_t *lpkt, of_octets_t *data)
{
    if(!lpkt || !data) {
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
pdua_pkt_data_free (pdua_pkt_t *lpkt)
{
    if (lpkt) {
        if (lpkt->data.data) {
            PDUA_FREE(lpkt->data.data);
            lpkt->data.data  = NULL;
            lpkt->data.bytes = 0;
        }
    }
}

static indigo_error_t
pdua_port_disable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt, pdua_port_t *port)
{
    indigo_error_t rv;

    if ((rv = ind_soc_timer_event_unregister(cb, port)) == INDIGO_ERROR_NONE) {
        pkt->interval_ms = 0;
        pdua_pkt_data_free(pkt);
    }
    return rv;
}

static indigo_error_t
pdua_port_enable(ind_soc_timer_callback_f cb, pdua_pkt_t *pkt, pdua_port_t *port,
                 of_octets_t *data, uint32_t interval_ms)
{
    indigo_error_t rv;

    if ((rv = pdua_pkt_data_set(pkt, data)) == INDIGO_ERROR_NONE) {
        if ((rv = ind_soc_timer_event_register_with_priority(cb, port, interval_ms,
                                                             IND_SOC_HIGH_PRIORITY))
                == INDIGO_ERROR_NONE) {
            pkt->interval_ms = interval_ms;
        } else {
            pdua_pkt_data_free(pkt);
        }
    }
    return rv;
}

/* Unregister timer and free data */
static void
pdua_disable_tx_rx(pdua_port_t *port)
{
    indigo_error_t rv;

    if (!port)
        return;

    if (port->tx_pkt.interval_ms) {
        if((rv = pdua_port_disable(pdu_periodic_tx, &port->tx_pkt, port))
               != INDIGO_ERROR_NONE) {
            AIM_LOG_ERROR("Port tx %u failed to disable %s\n", port->port_no, indigo_strerror(rv));
        }
    }

    if (port->rx_pkt.interval_ms) {
        if((rv = pdua_port_disable(pdu_timeout_rx, &port->rx_pkt, port))
               != INDIGO_ERROR_NONE) {
            AIM_LOG_ERROR("Port rx %u failed to disable %s\n", port->port_no, indigo_strerror(rv));
        }
    }

}

static void
pdu_timeout_rx(void *cookie)
{
    uint32_t version;
    pdua_port_t *port = (pdua_port_t*) cookie;
    of_bsn_pdu_rx_timeout_t *timeout_msg = NULL;

    if (!port)
        return;

    if (indigo_cxn_get_async_version(&version) != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("No controller connected");
        return;
    }

    timeout_msg = of_bsn_pdu_rx_timeout_new(version);
    if(!timeout_msg){
        AIM_LOG_INTERNAL("Failed to allocate timeout msg");
        return;
    }

    /* Set port number */
    of_bsn_pdu_rx_timeout_port_no_set (timeout_msg, port->port_no);

    /* Set slot number */
    of_bsn_pdu_rx_timeout_slot_num_set (timeout_msg, PDU_SLOT_NUM);

    PDUA_DEBUG("Send rx timeout async msg");
    /* Send to controller, don't delete when send to controller */
    indigo_cxn_send_async_message(timeout_msg);

    port->timeout_pkt_cnt++;
}

static void
pdu_periodic_tx(void *cookie)
{
    pdua_port_t *port = (pdua_port_t*) cookie;
    indigo_error_t rv;

    if(!port) {
       return;
    }

    PDUA_DEBUG("Port %u: Fwd tx pkt out", port->port_no);

    rv = slshared_fwd_packet_out(&port->tx_pkt.data, 0, port->port_no,
                                 SLSHARED_CONFIG_PDU_QUEUE_PRIORITY);
    if (rv < 0) {
        AIM_LOG_INTERNAL("Fwd pkt out failed %s", indigo_strerror(rv));
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
    uint32_t     xid;
    of_port_no_t port_no;
    uint32_t     timeout_ms;
    of_octets_t  data;

    of_bsn_pdu_rx_reply_t *rx_reply = NULL;
    uint32_t              status_failed = 0;

    /* Get rx req info */
    of_bsn_pdu_rx_request_xid_get       (rx_req, &xid);
    of_bsn_pdu_rx_request_timeout_ms_get(rx_req, &timeout_ms);
    of_bsn_pdu_rx_request_data_get      (rx_req, &data);
    of_bsn_pdu_rx_request_port_no_get   (rx_req, &port_no);

    if (timeout_ms && !data.data) {
        status_failed = 1;
        AIM_LOG_ERROR("Req_Rx Port %u, inconsistent info", port_no);
        goto rx_reply_to_ctrl;
    }

    if (!(port = pdua_find_port(port_no))) {
        status_failed = 1;
        AIM_LOG_ERROR("Port %u doesn't exist", port_no);
        goto rx_reply_to_ctrl;
    }

    port->rx_req_cnt++;

    /* 1. Unreg timer, delete the current rx_pkt */
    if(port->rx_pkt.interval_ms) {
        if ((rv = pdua_port_disable(pdu_timeout_rx, &port->rx_pkt, port)) != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("Port rx %u failed to disable %s", port->port_no, indigo_strerror(rv));
            goto rx_reply_to_ctrl;
        }
    }

    AIM_TRUE_OR_DIE(!port->rx_pkt.interval_ms && !port->rx_pkt.data.data);

    /* 2. Set up new rx_pkt, timer */
    if(timeout_ms) {
        if ((rv = pdua_port_enable(pdu_timeout_rx, &port->rx_pkt, port,
                                   &data, timeout_ms)) != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("Port rx %u failed to enable %s", port->port_no, indigo_strerror(rv));
        }
    }

rx_reply_to_ctrl:
    /* 3. Setup reply */
    rx_reply = of_bsn_pdu_rx_reply_new(rx_req->version);
    if(!rx_reply){
        AIM_LOG_INTERNAL("Failed to allocate rx_reply");
        return;
    }
    of_bsn_pdu_rx_reply_xid_set     (rx_reply, xid);
    of_bsn_pdu_rx_reply_port_no_set (rx_reply, port_no);
    of_bsn_pdu_rx_reply_status_set  (rx_reply, status_failed);
    of_bsn_pdu_rx_reply_slot_num_set(rx_reply, PDU_SLOT_NUM);

    PDUA_DEBUG("Port %u: sends a RX_reply to ctrl, status %s, version %u",
                port_no, status_failed? "Failed" : "Success", rx_req->version);
    /* 4. Send to controller, don't delete obj */
    indigo_cxn_send_controller_message(cxn_id, rx_reply);

}


static void
tx_request_handle(indigo_cxn_id_t cxn_id, of_object_t *tx_req)
{
    pdua_port_t *port = NULL;

    indigo_error_t rv;

    /* tx_req info */
    uint32_t     xid;
    of_port_no_t port_no;
    uint32_t     tx_interval_ms;
    of_octets_t  data;

    /* tx_reply info */
    of_bsn_pdu_tx_reply_t *tx_reply = NULL;
    uint32_t              status_failed = 0;

    /* Get tx req info */
    of_bsn_pdu_tx_request_xid_get           (tx_req, &xid);
    of_bsn_pdu_tx_request_tx_interval_ms_get(tx_req, &tx_interval_ms);
    of_bsn_pdu_tx_request_data_get          (tx_req, &data);
    of_bsn_pdu_tx_request_port_no_get       (tx_req, &port_no);

    if (tx_interval_ms && !data.data) {
        status_failed = 1;
        AIM_LOG_ERROR("Req_Tx Port %u, Inconsistent info", port_no);
        goto tx_reply_to_ctrl;
    }

    if (!(port = pdua_find_port(port_no))) {
        status_failed = 1;
        AIM_LOG_ERROR("Port %u doesn't exist", port_no);
        goto tx_reply_to_ctrl;
    }

    port->tx_req_cnt++;

    /* 1. unreg old timer, delete old data */
    if (port->tx_pkt.interval_ms) {
        if ((rv = pdua_port_disable(pdu_periodic_tx, &port->tx_pkt, port)) != INDIGO_ERROR_NONE) {
            status_failed = 1;
            AIM_LOG_ERROR("Port tx %u failed to disable %s", port->port_no, indigo_strerror(rv));
            goto tx_reply_to_ctrl;
        }
    }

    AIM_TRUE_OR_DIE(!port->tx_pkt.interval_ms && !port->tx_pkt.data.data);

    /* 2. Set up new tx_pkt, alarm */
    if(tx_interval_ms) {
        if ((rv = pdua_port_enable(pdu_periodic_tx, &port->tx_pkt, port,
                                   &data, tx_interval_ms)) == INDIGO_ERROR_NONE) {
            /* Successfully enable, send one out immediately */
            pdu_periodic_tx(port);
        } else {
            status_failed = 1;
            AIM_LOG_ERROR("Port tx %u failed to enable %s", port->port_no, indigo_strerror(rv));
        }
    }

tx_reply_to_ctrl:
    /* 3. Setup reply  */
    tx_reply = of_bsn_pdu_tx_reply_new(tx_req->version);
    if(!tx_reply){
        AIM_LOG_INTERNAL("Failed to allocate tx reply");
        return;
    }

    of_bsn_pdu_tx_reply_xid_set     (tx_reply, xid);
    of_bsn_pdu_tx_reply_port_no_set (tx_reply, port_no);
    of_bsn_pdu_tx_reply_status_set  (tx_reply, status_failed);
    of_bsn_pdu_tx_reply_slot_num_set(tx_reply, PDU_SLOT_NUM);

    PDUA_DEBUG("Port %u: sends  a TX_reply to ctrl, status %s, version %u",
                port_no, status_failed? "Failed" : "Success", tx_req->version);
    /* 4. Send to controller, don't delete obj */
    indigo_cxn_send_controller_message(cxn_id, tx_reply);

}

/* Register to listen to CTRL msg */
indigo_core_listener_result_t
pdua_handle_msg (indigo_cxn_id_t cxn_id, of_object_t *msg)
{
    indigo_core_listener_result_t ret = INDIGO_CORE_LISTENER_RESULT_PASS;
    uint8_t slot_num;

    if(!msg)
        return ret;

    switch (msg->object_id) {
    case OF_BSN_PDU_RX_REQUEST:
        of_bsn_pdu_rx_request_slot_num_get(msg, &slot_num);
        if (slot_num != PDU_SLOT_NUM) {
            PDUA_DEBUG("Received rx request with slot_num: %u", slot_num);
            return ret;
        }

        /* Count msg in */
        pdua_port_sys.total_msg_in_cnt++;
        rx_request_handle(cxn_id, msg);
        ret = INDIGO_CORE_LISTENER_RESULT_DROP;
        break;

    case OF_BSN_PDU_TX_REQUEST:
        of_bsn_pdu_tx_request_slot_num_get(msg, &slot_num);
        if (slot_num != PDU_SLOT_NUM) {
            PDUA_DEBUG("Received tx request with slot_num: %u", slot_num);
            return ret;
        }

        /* Count msg in */
        pdua_port_sys.total_msg_in_cnt++;
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
 * */
static inline int
pdua_rx_pkt_is_expected(pdua_port_t *port, of_octets_t *data)
{
    int ret = 0;

    if (!port->rx_pkt.data.data) {
        PDUA_DEBUG("Port %u: MISMATCHED RX no data", port->port_no);
        port->rx_pkt_mismatched_no_data++;
        return ret;
    }

    if (port->rx_pkt.data.bytes != data->bytes) {
        PDUA_DEBUG("Port %u: MISMATCHED len exp=%u, rcv=%u",
                    port->port_no, port->rx_pkt.data.bytes, data->bytes);
        port->rx_pkt_mismatched_len++;
        return ret;
    }

    if (memcmp(port->rx_pkt.data.data, data->data, data->bytes) == 0) {
        PDUA_DEBUG("Port %u: MATCHED\n", port->port_no);
        ret = 1;
        port->rx_pkt_matched++;
    } else {
        PDUA_DEBUG("Port %u: MISMATCHED data\n", port->port_no);
        port->rx_pkt_mismatched_data++;
    }

    return ret;
}

/*
 * Caller must ensure pdua != NULL
 * Reset timeout
 * */
static inline void
pdua_update_rx_timeout(pdua_port_t *port)
{
    indigo_error_t rv;
    PDUA_DEBUG("Using reset timer");
    if ((rv = ind_soc_timer_event_register_with_priority(pdu_timeout_rx, port, port->rx_pkt.interval_ms,
                                                         IND_SOC_HIGH_PRIORITY)) !=
            INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("Port %u failed to register %s",port->port_no, indigo_strerror(rv));
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
        AIM_LOG_INTERNAL("PDUA port out of range %u", port_no);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    port->rx_pkt_in_cnt++;
    if (pdua_dump_data == PDUA_DUMP_ENABLE_ALL_PORTS ||
        pdua_dump_data == port_no) {
        PDUA_DEBUG("PDUA_DATA_HEXDUMP:\n%{data}\n", data->data, data->bytes);
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

    pdua_update_rx_timeout(port);
    pdua_port_sys.total_pkt_exp_cnt++;
    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

/* Register to listen to PACKETIN msg */
indigo_core_listener_result_t
pdua_handle_pkt (of_packet_in_t *packet_in)
{
    of_octets_t                data;
    of_port_no_t               port_no;
    of_match_t                 match;

    if (!packet_in) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* Data is the ether pkt */
    of_packet_in_data_get(packet_in, &data);

    if (!data.data) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* Only count pkt_in with valid data */
    pdua_port_sys.total_pkt_in_cnt++;

    if (packet_in->version <= OF_VERSION_1_1) {
        of_packet_in_in_port_get(packet_in, &port_no);
        PDUA_DEBUG("port %u pkt in version %d", port_no, packet_in->version);
    } else {
        if (of_packet_in_match_get(packet_in, &match) < 0) {
            AIM_LOG_INTERNAL("match get failed");
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }
        port_no = match.fields.in_port;
        PDUA_DEBUG("Port %u", port_no);
    }

    return pdua_receive_packet(&data, port_no);
}


/************************
 * PDUA INIT and FINISH
 ************************/

/* Return 0: success */
int
pdua_system_init()
{
    int i;
    pdua_port_t *port;

    AIM_LOG_VERBOSE("init");

    pdua_port_sys.pdua_total_of_ports = sizeof(pdua_port_sys.pdua_ports) /
                                        sizeof(pdua_port_sys.pdua_ports[0]);
    for (i=0; i < pdua_port_sys.pdua_total_of_ports; i++) {
        port = pdua_find_port(i);
        if (port)
            port->port_no = i;
        else
            AIM_LOG_INTERNAL("Port %d not existing", i);
    }

    indigo_core_message_listener_register(pdua_handle_msg);
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_register(pdua_handle_pkt);
#endif

    return 0;
}

void
pdua_system_finish()
{
    int i;
    pdua_port_t *port;

    indigo_core_message_listener_unregister(pdua_handle_msg);
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_unregister(pdua_handle_pkt);
#endif

    for (i=0; i < pdua_port_sys.pdua_total_of_ports; i++) {
        port = pdua_find_port(i);
        if (port)
            pdua_disable_tx_rx(port);
        else
            AIM_LOG_INTERNAL("Port %d not existing", i);
    }
}
