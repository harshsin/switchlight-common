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

/**************************************************************************//**
 *
 * collecta Internal Header
 *
 *****************************************************************************/
#ifndef __COLLECTA_INT_H__
#define __COLLECTA_INT_H__

#include <collecta/collecta_config.h>
#include <loci/loci.h>
#include <slshared/slshared.h>
#include <SocketManager/socketmanager.h>
#include <debug_counter/debug_counter.h>
#include <AIM/aim_list.h>

typedef struct collect_collector_entry_key_s { /* collect_collector_entry_key */
    uint32_t collector_ip;
} collect_collector_entry_key_t;

typedef struct collect_collector_entry_value_s { /* collect_collector_entry_value */
    uint16_t collector_udp_dport;
} collect_collector_entry_value_t;

typedef struct collect_collector_entry_stats_s { /* collect_collector_entry_stats */
    uint64_t tx_packets;
    uint64_t tx_bytes;
} collect_collector_entry_stats_t;

typedef struct collect_collector_entry_s { /* collect_collector_entry */
    collect_collector_entry_key_t key;
    collect_collector_entry_value_t value;
    collect_collector_entry_stats_t stats;
    int sd;
    list_links_t  links;
} collect_collector_entry_t;

typedef struct collect_debug_counters_s { /* collect_debug_counters */
    debug_counter_t packet_in;
    debug_counter_t packet_out;
} collect_debug_counters_t;

extern uint64_t datapath_id;
extern struct ind_cfg_ops collecta_cfg_ops;
extern collect_debug_counters_t collect_counters;

list_head_t *collect_collectors_list(void);

#endif /* __COLLECTA_INT_H__ */
