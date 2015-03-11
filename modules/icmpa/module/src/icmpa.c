/****************************************************************
 *
 *        Copyright 2013, Big Switch Networks, Inc.
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

/*
 * Implementation of Icmp Agent.
 *
 * This file contains code for icmp error msg generation and handling
 * icmp request msg's (ECHO, etc.)
 */

#include "icmpa_int.h"

/*
 * isBroadcastAddress
 *
 * Returns true if a given IPv4 address is a Broadcast IP;
 * else returns false
 */
bool
isBroadcastAddress (uint32_t ip)
{
    return (ip == 0xffffffff);
}

/*
 * isMulticastAddress
 *
 * Returns true if a given IPv4 address is a Multicast IP;
 * else returns false
 */
bool
isMulticastAddress (uint32_t ip)
{
    return ((ip & 0xF0000000) == 0xE0000000);
}

/*
 * isValidIP
 *
 * Returns false if a given IPv4 address is not Zero/Multicast/Broadcast;
 * else returns true
 */
bool
isValidIP (uint32_t ip)
{
    if ((ip == 0) || isMulticastAddress(ip) || isBroadcastAddress(ip)) {
        return false;
    } else {
        return true;
    }
}

/*
 * icmpa_get_vlan_id
 *
 * Check if the packet has 802.1q header and extract the vlan id
 */
static bool
icmpa_get_vlan_id (ppe_packet_t *ppep, uint32_t *vlan_id, uint32_t *vlan_pcp)
{
    ppe_header_t format;

    if (!ppep || !vlan_id || !vlan_pcp) return false;

    ppe_packet_format_get(ppep, &format);
    if (format != PPE_HEADER_8021Q) return false;

    ppe_field_get(ppep, PPE_FIELD_8021Q_VLAN, vlan_id);
    ppe_field_get(ppep, PPE_FIELD_8021Q_PRI, vlan_pcp);

    return true;
}

/*
 * icmpa_build_pdu
 *
 * Build an ICMP packet
 */
static bool
icmpa_build_pdu (ppe_packet_t *ppep_rx, of_octets_t *octets, uint32_t vlan_id,
                 uint32_t vlan_pcp, uint32_t ip_total_len, uint32_t router_ip,
                 uint32_t type, uint32_t code, uint32_t hdr_data,
                 uint8_t *icmp_data, uint32_t icmp_data_len)
{
    ppe_packet_t               ppep_tx;
    uint8_t                    src_mac[OF_MAC_ADDR_BYTES];
    uint8_t                    dest_mac[OF_MAC_ADDR_BYTES];
    uint32_t                   dest_ip;

    if (!ppep_rx || !octets || !icmp_data) return false;

    AIM_LOG_TRACE("Build ICMP PDU with type: %d, code: %d", type, code);
    ppe_packet_init(&ppep_tx, octets->data, octets->bytes);

    /*
     * Set ethertype as 802.1Q and type as IPv4
     * Parse to recognize tagged Ethernet packet.
     */
    octets->data[12] = ICMPA_CONFIG_ETHERTYPE_DOT1Q >> 8;
    octets->data[13] = ICMPA_CONFIG_ETHERTYPE_DOT1Q & 0xFF;
    octets->data[16] = PPE_ETHERTYPE_IP4 >> 8;
    octets->data[17] = PPE_ETHERTYPE_IP4 & 0xFF;
    if (ppe_parse(&ppep_tx) < 0) {
        AIM_LOG_INTERNAL("ICMPA: Packet_out parsing failed after IPv4 header");
        return false;
    }

    /*
     * Get the Src Mac, Dest Mac from the incoming frame
     */
    ppe_wide_field_get(ppep_rx, PPE_FIELD_ETHERNET_SRC_MAC, src_mac);
    ppe_wide_field_get(ppep_rx, PPE_FIELD_ETHERNET_DST_MAC, dest_mac);

    /*
     * Set the Src Mac, Dest Mac and the Vlan-ID in the outgoing frame
     */
    ppe_wide_field_set(&ppep_tx, PPE_FIELD_ETHERNET_SRC_MAC, dest_mac);
    ppe_wide_field_set(&ppep_tx, PPE_FIELD_ETHERNET_DST_MAC, src_mac);

    /*
     * Check in icmp table if there is a vlan translation available for
     * this (vlan_id, vrouter_ip) key; else use the vlan in packet_in
     */
    icmp_entry_t *entry = icmpa_lookup(vlan_id, router_ip);
    if (entry == NULL) {
        AIM_LOG_TRACE("Entry with vlan: %u, ip: %{ipv4a} "
                      "not found in ICMP table", vlan_id, router_ip);
    } else {
        vlan_id = VLAN_VID(entry->value.vlan_id);
        vlan_pcp = VLAN_PCP(entry->value.vlan_id);
    }

    AIM_LOG_TRACE("Using vlan: %u, prio: %u for packet_out",
                  vlan_id, vlan_pcp);
    ppe_field_set(&ppep_tx, PPE_FIELD_8021Q_VLAN, vlan_id);
    ppe_field_set(&ppep_tx, PPE_FIELD_8021Q_PRI, vlan_pcp);

    /*
     * Src IP = Router IP
     * Dest IP = Get the Src IP and use it as the Dest IP
     */
    ppe_field_get(ppep_rx, PPE_FIELD_IP4_SRC_ADDR, &dest_ip);

    /*
     * Build the IP header
     */
    AIM_LOG_TRACE("Build IP header with src_ip: %{ipv4a}, dest_ip: %{ipv4a}",
                  router_ip, dest_ip);
    ppe_build_ipv4_header(&ppep_tx, router_ip, dest_ip, ip_total_len, 1, 128);

    /*
     * Build the ICMP packet (header + icmp data)
     */
    ppe_build_icmp_packet(&ppep_tx, type, code, hdr_data, icmp_data,
                          icmp_data_len);

    return true;
}

/*
 * icmpa_reply
 *
 * Driving logic for building and sending reply messages.
 * Currently we are only handling ICMP ECHO Requests.
 */
indigo_core_listener_result_t
icmpa_reply (ppe_packet_t *ppep, of_port_no_t port_no)
{
    of_octets_t                octets_out;
    uint32_t                   icmp_type;
    uint32_t                   hdr_data;
    uint32_t                   ip_total_len, ip_hdr_size;
    uint32_t                   icmp_data_len;
    uint32_t                   vlan_id, vlan_pcp;
    uint32_t                   src_ip, dest_ip;
    indigo_error_t             rv;

    if (!ppep) return INDIGO_CORE_LISTENER_RESULT_PASS;

    if (port_no > ICMPA_CONFIG_OF_PORTS_MAX) {
        AIM_LOG_INTERNAL("ICMPA: Port No: %d Out of Range %d",
                         port_no, ICMPA_CONFIG_OF_PORTS_MAX);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_field_get(ppep, PPE_FIELD_ICMP_TYPE, &icmp_type);

    /*
     * Check to make sure this is an ICMP ECHO Request
     * else, pass the packet-in to the controller
     */
    if (icmp_type != ICMP_ECHO_REQUEST) {
        AIM_LOG_TRACE("Not a ICMP ECHO Request Packet, type: %d", icmp_type);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * We should never receive an untagged frame
     */
    if (!icmpa_get_vlan_id(ppep, &vlan_id, &vlan_pcp)) {
        AIM_LOG_INTERNAL("ICMPA: Received Untagged Packet_in");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Echo requests should always be destined to Router IP
     */
    ppe_field_get(ppep, PPE_FIELD_IP4_DST_ADDR, &dest_ip);
    if (router_ip_check(dest_ip) == false) {
        AIM_LOG_TRACE("ICMPA: Echo request dest_ip: %{ipv4a} is not router IP",
                      dest_ip);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    AIM_LOG_TRACE("ICMP ECHO Request received on port: %d", port_no);
    ++port_pkt_counters[port_no].icmp_echo_packets;

    /*
     * MUST NOT reply to a multicast/broadcast IP address.
     */
    ppe_field_get(ppep, PPE_FIELD_IP4_SRC_ADDR, &src_ip);
    if (!isValidIP(src_ip)) {
        AIM_LOG_ERROR("ICMPA: Echo request src_ip %{ipv4a} not valid"
                      "(zero/multicast/broadcast)", src_ip);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    /*
     * Build the ICMP packet
     */
    ppe_field_get(ppep, PPE_FIELD_IP4_HEADER_SIZE, &ip_hdr_size);
    ppe_field_get(ppep, PPE_FIELD_IP4_TOTAL_LENGTH, &ip_total_len);
    ppe_field_get(ppep, PPE_FIELD_ICMP_HEADER_DATA, &hdr_data);

    ip_hdr_size *= 4;
    if (ip_hdr_size > ICMPA_CONFIG_IPV4_HEADER_SIZE) {
        AIM_LOG_ERROR("ICMPA: IP options set as ip header size %d is more "
                      "than 20 bytes", ip_hdr_size);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    octets_out.data = aim_zmalloc(ppep->size);
    ICMPA_MEMSET(octets_out.data, 0, ppep->size);
    octets_out.bytes = ppep->size;
    icmp_data_len = ip_total_len - ip_hdr_size - ICMP_HEADER_SIZE;
    if (!icmpa_build_pdu(ppep, &octets_out, vlan_id, vlan_pcp, ip_total_len,
        dest_ip, ICMP_ECHO_REPLY, 0, hdr_data,
        ppe_fieldp_get(ppep, PPE_FIELD_ICMP_PAYLOAD), icmp_data_len)) {
        AIM_LOG_INTERNAL("ICMPA: icmpa_build_pdu failed");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        aim_free(octets_out.data);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    /*
     * Send the ICMP echo reply out
     */
    rv = slshared_fwd_packet_out(&octets_out, OF_PORT_DEST_CONTROLLER,
                                 OF_PORT_DEST_USE_TABLE, QUEUE_ID_INVALID);
    if (rv < 0) {
        AIM_LOG_INTERNAL("ICMPA: Send packet_out failed for port: %d, reason: %s",
                         port_no, indigo_strerror(rv));
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
    } else {
        debug_counter_inc(&pkt_counters.icmp_total_out_packets);
        AIM_LOG_TRACE("Successfully sent packet out the port: %d", port_no);
    }

    aim_free(octets_out.data);
    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

/*
 * icmpa_send
 *
 * Send an ICMP message in response to below situation's
 * 1. TTL Expired
 * 2. Net Unreachable
 * 3. Port Unreachable
 *
 * RFC 1122: 3.2.2 MUST send at least the IP header and 8 bytes of header.
 *
 * NOTE: Controller will try to resolve unreachable hosts by sending ARP,
 * If host is still not found then controller should send ICMP Host unreachable.
 */
indigo_core_listener_result_t
icmpa_send (ppe_packet_t *ppep, of_port_no_t port_no, uint32_t type,
            uint32_t code)
{
    of_octets_t                octets_out;
    uint8_t                    *ip_hdr = NULL;
    uint32_t                   ip_total_len;
    uint8_t                    data[ICMP_PKT_BUF_SIZE];
    uint32_t                   vlan_id, vlan_pcp;
    uint32_t                   router_ip;
    of_mac_addr_t              router_mac;
    uint32_t                   src_ip, dest_ip;
    indigo_error_t             rv;

    if (!ppep) return INDIGO_CORE_LISTENER_RESULT_PASS;

    ICMPA_MEMSET(data, 0, ICMP_PKT_BUF_SIZE);

    if (port_no > ICMPA_CONFIG_OF_PORTS_MAX) {
        AIM_LOG_INTERNAL("ICMPA: Port No: %d Out of Range %d",
                         port_no, ICMPA_CONFIG_OF_PORTS_MAX);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    if (type == ICMP_DEST_UNREACHABLE && code == 0) {
        AIM_LOG_TRACE("ICMP Dest Network Unreachable received on port: %u",
                      port_no);
        ++port_pkt_counters[port_no].icmp_net_unreachable_packets;
    } else if (type == ICMP_DEST_UNREACHABLE && code == 3) {
        AIM_LOG_TRACE("ICMP Port Unreachable received on port: %u", port_no);
        ++port_pkt_counters[port_no].icmp_port_unreachable_packets;
    } else if (type == ICMP_TIME_EXCEEDED && code == 0) {
        AIM_LOG_TRACE("ICMP TTL Expired received on port: %u", port_no);
        ++port_pkt_counters[port_no].icmp_time_exceeded_packets;
    }

    ip_hdr = ppe_header_get(ppep, PPE_HEADER_IP4);
    if (!ip_hdr) {
        AIM_LOG_RL_TRACE(&icmp_pktin_log_limiter, os_time_monotonic(),
                         "Not an IP Packet");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * We should never receive an untagged frame
     */
    if (!icmpa_get_vlan_id(ppep, &vlan_id, &vlan_pcp)) {
        AIM_LOG_INTERNAL("ICMPA: Received Untagged Packet_in");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Traceroute is destined to the VRouter IP, hence we should use
     * that as the src IP for Icmp Port Unreachable.
     *
     * For Icmp TTL Expired and ICMP Net Unreachable cases we need to
     * lookup the Vrouter IP based on the Vlan to use as src ip
     *
     * In case of traceroute to a host, packet triggerring ttl expired will
     * arrive on SYSTEM_VLAN and will be destined to the host and hence a
     * lookup with SYSTEM_VLAN key in router_ip_table will fail.
     * Check in icmp table to determine the VRouter ip associated
     * with this host to use as src ip in ttl expired msg.
     */
    ppe_field_get(ppep, PPE_FIELD_IP4_DST_ADDR, &dest_ip);
    if (type == ICMP_DEST_UNREACHABLE && code == 3) {
        router_ip = dest_ip;
    } else {
        if (router_ip_table_lookup(vlan_id, &router_ip, &router_mac) < 0) {
            AIM_LOG_TRACE("ICMPA: Router IP lookup failed for vlan: %u",
                          vlan_id);

            if (!icmpa_router_ip_lookup(dest_ip, &router_ip)) {
                AIM_LOG_ERROR("ICMPA: Router IP lookup failed in icmp table "
                              "for dest_ip:%{ipv4a}", dest_ip);
                debug_counter_inc(&pkt_counters.icmp_internal_errors);
                return INDIGO_CORE_LISTENER_RESULT_DROP;
            }
        }
    }

    /*
     * MUST NOT send to a multicast/broadcast IP address.
     */
    ppe_field_get(ppep, PPE_FIELD_IP4_SRC_ADDR, &src_ip);
    if (!isValidIP(src_ip)) {
        AIM_LOG_ERROR("ICMPA: src_ip %{ipv4a} in original ip packet not valid"
                      "(zero/multicast/broadcast)", src_ip);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    AIM_LOG_TRACE("Send ICMP message with type: %d, code: %d", type, code);

    /*
     * Build the ICMP packet
     */
    octets_out.data = data;
    octets_out.bytes = ICMP_PKT_BUF_SIZE;
    ppe_field_get(ppep, PPE_FIELD_IP4_TOTAL_LENGTH, &ip_total_len);
    if (ip_total_len < ICMP_DATA_LEN) {
        AIM_LOG_ERROR("ICMPA: IP total len %d is less than required 28 bytes",
                      ip_total_len);
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    if (!icmpa_build_pdu(ppep, &octets_out, vlan_id, vlan_pcp, IP_TOTAL_LEN,
        router_ip, type, code, 0, ip_hdr, ICMP_DATA_LEN)) {
        AIM_LOG_INTERNAL("ICMPA: icmpa_build_pdu failed");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    }

    /*
     * Send the ICMP message out
     */
    rv = slshared_fwd_packet_out(&octets_out, OF_PORT_DEST_CONTROLLER,
                                 OF_PORT_DEST_USE_TABLE, QUEUE_ID_INVALID);
    if (rv < 0) {
        AIM_LOG_INTERNAL("ICMPA: Send packet_out failed for port: %d, reason: %s",
                         port_no, indigo_strerror(rv));
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
    } else {
        debug_counter_inc(&pkt_counters.icmp_total_out_packets);
        AIM_LOG_TRACE("Successfully sent packet out the port: %d", port_no);
    }

    return INDIGO_CORE_LISTENER_RESULT_DROP;
}
