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

/*************************************************************//**
 *
 * pktina Internal Header
 *
 ****************************************************************/
#ifndef __PKTINA_INT_H__
#define __PKTINA_INT_H__

#include <PPE/ppe.h>
#include <pktina/pktina_config.h>
#include <debug_counter/debug_counter.h>
#include "pktina_log.h"

#define PKTINA_PORT_PKTIN_STATS             \
    PKTINA_PORT_PKTIN_STAT(station_move)    \
    PKTINA_PORT_PKTIN_STAT(new_host)        \
    PKTINA_PORT_PKTIN_STAT(arp_cache)       \
    PKTINA_PORT_PKTIN_STAT(debug_acl)       \
    PKTINA_PORT_PKTIN_STAT(cdp)             \
    PKTINA_PORT_PKTIN_STAT(lldp)            \
    PKTINA_PORT_PKTIN_STAT(lacp)            \
    PKTINA_PORT_PKTIN_STAT(dhcp)            \
    PKTINA_PORT_PKTIN_STAT(igmp)            \
    PKTINA_PORT_PKTIN_STAT(arp)             \
    PKTINA_PORT_PKTIN_STAT(l3_dst_miss)     \
    PKTINA_PORT_PKTIN_STAT(icmp)            \
    PKTINA_PORT_PKTIN_STAT(traceroute)      \
    PKTINA_PORT_PKTIN_STAT(sflow)           \
    PKTINA_PORT_PKTIN_STAT(bad_ttl)         \
    PKTINA_PORT_PKTIN_STAT(total)

typedef struct pktina_port_debug_s {
#define PKTINA_PORT_PKTIN_STAT(name)   \
    debug_counter_t name;              \
    char name##_counter_name_buf[DEBUG_COUNTER_NAME_SIZE];

    PKTINA_PORT_PKTIN_STATS
#undef PKTINA_PORT_PKTIN_STAT
} pktina_port_debug_t;

#endif /* __PKTINA_INT_H__ */
