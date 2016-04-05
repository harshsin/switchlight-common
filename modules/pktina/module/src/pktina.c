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

/****************************************************************
 * pktina takes packet-ins and distributes the PPE to relevant
 * switchlight-common agents based on packet-in reason and
 * packet type.
 *
 * NOTE: packet-of-death message is not handled here.
 * BRCMDriver3 uses indigo_core_packet_in_listener_register
 * to get packet-ins.
 ****************************************************************/

#include "pktina_int.h"
#include <cdpa/cdpa.h>
#include <lldpa/lldpa.h>
#include <lacpa/lacpa.h>
#include <arpa/arpa.h>
#include <icmpa/icmpa.h>
#include <dhcpra/dhcpra.h>
#include <router_ip_table/router_ip_table.h>
#include <indigo/port_manager.h>
#include <igmpa/igmpa.h>
#include <sflowa/sflowa.h>

DEBUG_COUNTER(pktin, "pktina.pktin",
              "Received packet-in messages");
DEBUG_COUNTER(ctrl_pktin, "pktina.pktin.controller",
              "Pktin's passed to the controller");
DEBUG_COUNTER(sflow_pktin, "pktina.pktin.sflow",
              "Sflow sampled pktin's recv'd");
DEBUG_COUNTER(pktin_parse_error, "pktina.pktin.parse_error",
              "Error while parsing packet-in");
DEBUG_COUNTER(pktin_invalid_port, "pktina.pktin.invalid_port",
              "Pktin's received on invalid port");

static pktina_port_debug_t *pktina_port_stats;
static pktina_port_debug_t pktina_cpu_port_stats;

static const of_mac_addr_t cdp_mac = { { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc } };

/*
 * Returns true if a given port is ephemeral, else returns false
 */
static bool
is_ephemeral(uint32_t port)
{
    return (port >= 32768 && port <= 61000);
}

static pktina_port_debug_t *
pktina_port_stats_get(of_port_no_t of_port)
{
    if (of_port <= PKTINA_CONFIG_OF_PORTS_MAX) {
        /* Front panel port stats */
        return &pktina_port_stats[of_port];
    } else if (of_port == OF_PORT_DEST_CONTROLLER) {
        /* CPU port stats */
        return &pktina_cpu_port_stats;
    } else {
        /* Not expecting packet-ins from any other port */
        AIM_LOG_ERROR("Packet-in on invalid port %u", of_port);
    }

    return NULL;
}

static indigo_core_listener_result_t
pktina_distribute_packet(of_octets_t octets,
                         uint32_t in_port,
                         uint64_t metadata,
                         pktina_port_debug_t *port_stats)
{
    indigo_core_listener_result_t result = INDIGO_CORE_LISTENER_RESULT_PASS;
    ppe_packet_t ppep;

    ppe_packet_init(&ppep, octets.data, octets.bytes);
    if (ppe_parse(&ppep) < 0) {
        debug_counter_inc(&pktin_parse_error);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    /* See if the packet-in is a SFLOW packet-in */
    if (metadata & OFP_BSN_PKTIN_FLAG_SFLOW) {
        result = sflowa_receive_packet(&ppep, in_port);
        debug_counter_inc(&port_stats->sflow);
        debug_counter_inc(&sflow_pktin);
    }

    /* Identify if the packet-in needs to go to the controller before parsing */
    if (metadata & OFP_BSN_PKTIN_FLAG_STATION_MOVE) {
        debug_counter_inc(&port_stats->station_move);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    if (metadata & OFP_BSN_PKTIN_FLAG_NEW_HOST) {
        debug_counter_inc(&port_stats->new_host);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    if (metadata & OFP_BSN_PKTIN_FLAG_ARP_CACHE) {
        debug_counter_inc(&port_stats->arp_cache);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    if (metadata & OFP_BSN_PKTIN_FLAG_VXLAN_SIP_MISS) {
        debug_counter_inc(&port_stats->vxlan_sip_miss);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Identify the packet-in based on header type
     *
     * Echo requests/traceroute destined to VRouter will be
     * consumed by the ICMP agent on the switch.
     * But L3 destination miss needs to be processed
     * before ICMP Echo requests.
     *
     * If these pktin's also have ttl expired, we dont need to respond
     * with icmp ttl expired msg to the original source,
     * since echo/traceroute response will take precedence.
     */
    if (!memcmp(octets.data, cdp_mac.addr, OF_MAC_ADDR_BYTES)) {
        result = cdpa_receive_packet(&octets, in_port);
        debug_counter_inc(&port_stats->cdp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_LLDP)) {
        result = lldpa_receive_packet(&octets, in_port);
        debug_counter_inc(&port_stats->lldp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_LACP)) {
        result = lacpa_receive_packet (&ppep, in_port);
        debug_counter_inc(&port_stats->lacp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_DHCP)) {
        result = dhcpra_receive_packet(&ppep, in_port);
        debug_counter_inc(&port_stats->dhcp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_IGMP)) {
        result = igmpa_receive_pkt(&ppep, in_port);
        debug_counter_inc(&port_stats->igmp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_ARP)) {
        bool check_source = (metadata & OFP_BSN_PKTIN_FLAG_ARP) != 0;
        result = arpa_receive_packet(&ppep, in_port, check_source);
        debug_counter_inc(&port_stats->arp);
    } else if (metadata & OFP_BSN_PKTIN_FLAG_L3_MISS) {
        result = icmpa_send(&ppep, in_port, 3, 0);
        debug_counter_inc(&port_stats->l3_dst_miss);
    } else if (ppe_header_get(&ppep, PPE_HEADER_ICMP)) {
        result = icmpa_reply(&ppep, in_port);
        debug_counter_inc(&port_stats->icmp);
    } else if (ppe_header_get(&ppep, PPE_HEADER_UDP) &&
        ppe_header_get(&ppep, PPE_HEADER_IP4)) {

        /*
         * To handle traceroute, we need to check for
         * a) UDP Packet
         * b) dest IP is Vrouter IP
         * c) UDP src and dest ports are ephemeral
         */
        uint32_t dest_ip, src_port, dest_port;
        ppe_field_get(&ppep, PPE_FIELD_IP4_DST_ADDR, &dest_ip);
        ppe_field_get(&ppep, PPE_FIELD_UDP_SRC_PORT, &src_port);
        ppe_field_get(&ppep, PPE_FIELD_UDP_DST_PORT, &dest_port);

        if (router_ip_check(dest_ip) && is_ephemeral(src_port) &&
            is_ephemeral(dest_port)) {
            result = icmpa_send(&ppep, in_port, 3, 3);
            debug_counter_inc(&port_stats->traceroute);
        }
    }

    /*
     * Identify if the packet-in has debug/acl flag set
     * Debug/ACL packet-in's should always go the controller
     */
    bool debug_acl_flag = metadata & (OFP_BSN_PKTIN_FLAG_INGRESS_ACL|OFP_BSN_PKTIN_FLAG_DEBUG);

    if (result == INDIGO_CORE_LISTENER_RESULT_DROP) {
        if (debug_acl_flag) {
            debug_counter_inc(&port_stats->debug_acl);
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        } else {
            return INDIGO_CORE_LISTENER_RESULT_DROP;
        }
    }

    /*
     * Packet-in's passed by ICMP agent should later be
     * checked for ttl expired reason
     */
    if (metadata & OFP_BSN_PKTIN_FLAG_TTL_EXPIRED) {
        result = icmpa_send(&ppep, in_port, 11, 0);
        debug_counter_inc(&port_stats->bad_ttl);
    }

    if (result == INDIGO_CORE_LISTENER_RESULT_DROP && !debug_acl_flag) {
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    return INDIGO_CORE_LISTENER_RESULT_PASS;
}

static indigo_core_listener_result_t
pktina_of_packet_in_listener(of_packet_in_t *packet_in)
{
    of_match_t match;
    of_octets_t octets;
    indigo_core_listener_result_t result;
    pktina_port_debug_t *port_stats;

    debug_counter_inc(&pktin);

    AIM_TRUE_OR_DIE(of_packet_in_match_get(packet_in, &match) == 0);
    of_packet_in_data_get(packet_in, &octets);

    port_stats = pktina_port_stats_get(match.fields.in_port);
    if (port_stats == NULL) {
        debug_counter_inc(&pktin_invalid_port);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    debug_counter_inc(&port_stats->total);

    result = pktina_distribute_packet(octets, match.fields.in_port,
                                      match.fields.metadata, port_stats);

    if (result == INDIGO_CORE_LISTENER_RESULT_PASS) {
        debug_counter_inc(&ctrl_pktin);
    }

    return result;
}

static void
pktina_port_debug_counter_register(of_port_no_t of_port,
                                   pktina_port_debug_t *port_stat)
{
#define PKTINA_PORT_PKTIN_STAT(name)                            \
    snprintf(port_stat->name##_counter_name_buf,                \
        DEBUG_COUNTER_NAME_SIZE,                                \
        "pktina.port:%u.%s", of_port, #name);                   \
    debug_counter_register(&port_stat->name,                    \
        port_stat->name##_counter_name_buf,                     \
        #name" pktins received on this port");

    PKTINA_PORT_PKTIN_STATS
#undef PKTINA_PORT_PKTIN_STAT
}

static void
pktina_port_debug_counter_unregister(pktina_port_debug_t *port_stat)
{
#define PKTINA_PORT_PKTIN_STAT(name)                            \
    debug_counter_unregister(&port_stat->name);

    PKTINA_PORT_PKTIN_STATS
#undef PKTINA_PORT_PKTIN_STAT
}

static void
pktina_debug_counter_register(void)
{
    of_port_no_t of_port;

    /* Front panel port stats */
    for (of_port = 0; of_port <= PKTINA_CONFIG_OF_PORTS_MAX; of_port++) {
        pktina_port_debug_counter_register(of_port, &pktina_port_stats[of_port]);
    }

    /* CPU port stats */
    pktina_port_debug_counter_register(OF_PORT_DEST_CONTROLLER, &pktina_cpu_port_stats);
}

static void
pktina_debug_counter_unregister(void)
{
    of_port_no_t of_port;

    /* Front panel port stats */
    for (of_port = 0; of_port <= PKTINA_CONFIG_OF_PORTS_MAX; of_port++) {
        pktina_port_debug_counter_unregister(&pktina_port_stats[of_port]);
    }

    pktina_port_debug_counter_unregister(&pktina_cpu_port_stats);
}

#if PKTINA_CONFIG_INCLUDE_UCLI == 1

void
pktina_debug_counters_print(ucli_context_t* uc)
{
#define GLOBAL_DEBUG_COUNTER_PRINT(_counter)   \
    ucli_printf(uc, "%s   %"PRIu64"\n", _counter.name, debug_counter_get(&_counter))

    GLOBAL_DEBUG_COUNTER_PRINT(pktin);
    GLOBAL_DEBUG_COUNTER_PRINT(ctrl_pktin);
    GLOBAL_DEBUG_COUNTER_PRINT(sflow_pktin);
    GLOBAL_DEBUG_COUNTER_PRINT(pktin_parse_error);
    GLOBAL_DEBUG_COUNTER_PRINT(pktin_invalid_port);
}

void
pktina_debug_counters_clear(void)
{
    debug_counter_reset(&pktin);
    debug_counter_reset(&ctrl_pktin);
    debug_counter_reset(&sflow_pktin);
    debug_counter_reset(&pktin_parse_error);
    debug_counter_reset(&pktin_invalid_port);
}

void
pktina_port_debug_counters_print(ucli_context_t *uc, of_port_no_t of_port)
{
    pktina_port_debug_t *port_stat = pktina_port_stats_get(of_port);

    if (!port_stat)
        return;

#define PKTINA_PORT_PKTIN_STAT(name)                            \
    ucli_printf(uc, "%s   %"PRIu64"\n",                         \
                port_stat->name##_counter_name_buf,             \
                debug_counter_get(&port_stat->name));

    PKTINA_PORT_PKTIN_STATS
#undef PKTINA_PORT_PKTIN_STAT
}

void
pktina_port_debug_counters_clear(of_port_no_t of_port)
{
    pktina_port_debug_t *port_stat = pktina_port_stats_get(of_port);

    if (!port_stat)
        return;

#define PKTINA_PORT_PKTIN_STAT(name)                            \
    debug_counter_reset(&port_stat->name);

    PKTINA_PORT_PKTIN_STATS
#undef PKTINA_PORT_PKTIN_STAT
}

#endif /* PKTINA_CONFIG_INCLUDE_UCLI == 1 */

/**
 * pktina will always register/unregister packet-in listener
 * independent of SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER define.
 */
indigo_error_t
pktina_init(void)
{
    pktina_port_stats = aim_zmalloc(sizeof(pktina_port_debug_t) * (PKTINA_CONFIG_OF_PORTS_MAX+1));
    if (pktina_port_stats == NULL) {
        AIM_LOG_ERROR("Failed to allocate resources for port stats");
        return INDIGO_ERROR_RESOURCE;
    }

    indigo_core_packet_in_listener_register(pktina_of_packet_in_listener);
    pktina_debug_counter_register();

    return INDIGO_ERROR_NONE;
}

indigo_error_t
pktina_finish(void)
{
    indigo_core_packet_in_listener_unregister(pktina_of_packet_in_listener);
    pktina_debug_counter_unregister();
    aim_free(pktina_port_stats);
    return INDIGO_ERROR_NONE;
}
