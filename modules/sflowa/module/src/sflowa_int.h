/****************************************************************
 *
 *        Copyright 2014, Big Switch Networks, Inc.
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

/**************************************************************************//**
 *
 * sflowa Internal Header
 *
 *****************************************************************************/
#ifndef __SFLOWA_INT_H__
#define __SFLOWA_INT_H__

#include <sflowa/sflowa_config.h>
#include <sflowa/sflowa.h>
#include <loci/loci.h>
#include <indigo/of_state_manager.h>
#include <SocketManager/socketmanager.h>
#include <indigo/port_manager.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared.h>
#include <AIM/aim_list.h>
#include <host_sflow/host_sflow.h>

/*
 * if-status, admin and oper state
 */
#define IF_ADMIN_UP 0x01
#define IF_OPER_UP  0x02

/*
 * SFLOW Packet Structure
 ------------------------------------------------------
| 6  | 6  |    4   | 2  |  20  |   8   |               |
|DMAC|SMAC|TAG|VLAN|TYPE|IP HDR|UDP HDR| SFLOW DATAGRAM|
 ------------------------------------------------------
*/

#define SFLOW_PKT_HEADER_SIZE (SLSHARED_CONFIG_DOT1Q_HEADER_SIZE + \
    SLSHARED_CONFIG_IPV4_HEADER_SIZE + SLSHARED_CONFIG_UDP_HEADER_SIZE) //18 + 20 + 8

#define VLAN_VID(tci) ((tci) & 0xfff)
#define VLAN_PCP(tci) ((tci) >> 13)

typedef struct sflow_collector_entry_key_s { /* sflow_collector_entry_key */
    uint32_t collector_ip;
} sflow_collector_entry_key_t;

typedef struct sflow_collector_entry_value_s { /* sflow_collector_entry_value */
    uint16_t vlan_id;
    uint8_t vlan_pcp;
    of_mac_addr_t agent_mac;
    of_ipv4_t agent_ip;
    uint16_t agent_udp_sport;
    of_mac_addr_t collector_mac;
    uint16_t collector_udp_dport;
    uint32_t sub_agent_id;
} sflow_collector_entry_value_t;

typedef struct sflow_collector_entry_stats_s { /* sflow_collector_entry_stats */
    uint64_t tx_packets;
    uint64_t tx_bytes;
} sflow_collector_entry_stats_t;

typedef struct sflow_collector_entry_s { /* sflow_collector_entry */
    sflow_collector_entry_key_t key;
    sflow_collector_entry_value_t value;
    sflow_collector_entry_stats_t stats;
    int sd;
    list_links_t  links;
} sflow_collector_entry_t;

typedef struct sflow_sampler_entry_key_s { /* sflow_sampler_entry_key */
    of_port_no_t port_no;
} sflow_sampler_entry_key_t;

typedef struct sflow_sampler_entry_value_s { /* sflow_sampler_entry_value */
    uint32_t sampling_rate;
    uint32_t header_size;
    uint32_t polling_interval;
} sflow_sampler_entry_value_t;

typedef struct sflow_sampler_entry_stats_s { /* sflow_sampler_entry_stats */
    uint64_t rx_packets;
} sflow_sampler_entry_stats_t;

typedef struct sflow_sampler_entry_s { /* sflow_sampler_entry */
    sflow_sampler_entry_key_t key;
    sflow_sampler_entry_value_t value;
    sflow_sampler_entry_stats_t stats;
} sflow_sampler_entry_t;

typedef enum sflow_send_mode_e { /* sflow_send_mode */
    SFLOW_SEND_MODE_MGMT,
    SFLOW_SEND_MODE_DATAPLANE,
} sflow_send_mode_t;

typedef struct sflow_port_features_s { /* sflow_port_features */
    uint64_t speed;             /* Interface's current bandwidth in
                                   bits per second */
    uint32_t direction;         /* Derived from MAU MIB (RFC 2668)
                                   0 = unknown, 1 = full-duplex,
                                   2 = half-duplex, 3 = in, 4 = out */
    uint32_t status;            /* bit field with the following bits assigned:
                                   bit 0 = ifAdminStatus (0 = down, 1 = up)
                                   bit 1 = ifOperStatus (0 = down, 1 = up) */
} sflow_port_features_t;

typedef struct sflow_debug_counters_s { /* sflow_debug_counters */
    debug_counter_t packet_in;
    debug_counter_t packet_out;
    debug_counter_t counter_request;
    debug_counter_t port_status_notification;
    debug_counter_t port_features_update;
} sflow_debug_counters_t;

extern sflow_debug_counters_t sflow_counters;
extern sflow_sampler_entry_t sampler_entries[SFLOWA_CONFIG_OF_PORTS_MAX+1];
extern sflow_port_features_t port_features[SFLOWA_CONFIG_OF_PORTS_MAX+1];

/* Internal functions used by utest module */
list_head_t *sflow_collectors_list(void);

void sflow_timer(void *cookie);

indigo_core_listener_result_t
sflowa_packet_in_handler(of_packet_in_t *packet_in);

indigo_core_listener_result_t
sflowa_port_status_handler(of_port_status_t *port_status);

#endif /* __SFLOWA_INT_H__ */
