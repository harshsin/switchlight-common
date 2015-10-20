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

/*************************************************************//**
 *
 * pdua Internal Header
 *
 ****************************************************************/
#ifndef __PDUA_INT_H__
#define __PDUA_INT_H__

#include <pdua/pdua.h>
#include <pdua/pdua_config.h>

#include <loci/loci_base.h>
#include <OFStateManager/ofstatemanager.h>
#include <SocketManager/socketmanager.h>
#include <debug_counter/debug_counter.h>

typedef struct pdua_pkt_s {
    /* interval_ms == 0: disable */
    uint32_t        interval_ms;
    of_octets_t     data;
} pdua_pkt_t;

typedef struct pdua_port_s {
    of_port_no_t            port_no;
    pdua_pkt_t              rx_pkt;
    pdua_pkt_t              tx_pkt;

    /* Internal Port Statistics */
    uint64_t                rx_pkt_in_cnt;
    uint64_t                rx_pkt_mismatched_no_data;
    uint64_t                rx_pkt_mismatched_len;
    uint64_t                rx_pkt_mismatched_data;
    uint64_t                rx_pkt_matched;
    uint64_t                tx_pkt_out_cnt;
    uint64_t                timeout_pkt_cnt;
    uint64_t                tx_req_cnt;
    uint64_t                rx_req_cnt;

    /* Slot_Num Statistics if supported */

    bool                    dump_enabled;
    pdua_packet_state_t     pkt_state;
} pdua_port_t;

typedef struct pdua_debug_s {
    debug_counter_t     total_pkt_in_cnt;
    debug_counter_t     total_msg_in_cnt;
    debug_counter_t     total_pkt_exp_cnt;
} pdua_debug_t;

typedef struct pdua_system_s {
    uint32_t        pdua_total_of_ports;

    /* Internal statistic for listener interfaces*/
    pdua_debug_t    debug_info;
    pdua_port_t     pdua_ports[PDUA_CONFIG_OF_PORTS_MAX+1];
} pdua_system_t;

indigo_core_listener_result_t pdua_handle_msg(indigo_cxn_id_t cxn_id,
                                              of_object_t *msg);

indigo_core_listener_result_t pdua_handle_pkt(of_packet_in_t *packet_in);

pdua_port_t *pdua_find_port(of_port_no_t port_no);

extern pdua_system_t pdua_port_sys;

#endif /* __PDUA_INT_H__ */
