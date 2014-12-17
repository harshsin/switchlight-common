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
 * either express or implied. See the License for the shard
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

/*
 * This file contains code that uses open source Host sFlow,
 * which is licensed as below.
 */

 /* Host sFlow software is distributed under the following license:
  * http://host-sflow.sourceforge.net/license.html
  */

/*
 * Implementation of Sflow Agent.
 *
 * This file contains code for initalizing sflow agent and
 * sflow gentable (sflow_collector, sflow_sampler) operations.
 */

#include <AIM/aim.h>
#include <OS/os_time.h>
#include <PPE/ppe.h>
#include <fcntl.h>
#include <linux/ethtool.h>
#include "sflowa_int.h"
#include "sflowa_log.h"

/*
 * only one dummy receiver, so the receiverIndex is a constant
 */
#define SFLOW_RECEIVER_INDEX 1

/*
 * This is the maximum header_size we can get from the sampled packet
 * because host sFlow agent breaks for anything over this size.
 */
#define SFLOW_MAX_HEADER_SIZE 1300

/*
 * Duplex, half or full.
 * 0 = unknown, 1 = full-duplex,
 * 2 = half-duplex, 3 = in, 4 = out
 */
#define SFLOW_DUPLEX_FULL 1
#define SFLOW_DUPLEX_HALF 2

static const of_mac_addr_t zero_mac = { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static indigo_core_gentable_t *sflow_collector_table;
static indigo_core_gentable_t *sflow_sampler_table;

static const indigo_core_gentable_ops_t sflow_collector_ops;
static const indigo_core_gentable_ops_t sflow_sampler_ops;

static bool sflowa_initialized = false;
static uint16_t sflow_enabled_ports = 0;

sflow_sampler_entry_t sampler_entries[SFLOWA_CONFIG_OF_PORTS_MAX+1];
static LIST_DEFINE(sflow_collectors);

static sflowa_sampling_rate_handler_f sflowa_sampling_rate_handler;

static SFLAgent dummy_agent;
aim_ratelimiter_t sflow_pktin_log_limiter;

sflow_port_features_t port_features[SFLOWA_CONFIG_OF_PORTS_MAX+1];
static bool port_features_stale = true;

sflow_debug_counters_t sflow_counters;

/*
 * sflowa_init
 *
 * API to init the Sflow Agent
 * This should only be done once at the beginning.
 */
indigo_error_t
sflowa_init(void)
{
    if (sflowa_initialized) return INDIGO_ERROR_NONE;

    AIM_LOG_TRACE("init");

    indigo_core_gentable_register("sflow_collector", &sflow_collector_ops, NULL,
                                  4, 4, &sflow_collector_table);
    indigo_core_gentable_register("sflow_sampler", &sflow_sampler_ops, NULL,
                                  SFLOWA_CONFIG_OF_PORTS_MAX, 128,
                                  &sflow_sampler_table);

    /*
     * Register listener for packet_in
     */
    if (indigo_core_packet_in_listener_register(
        (indigo_core_packet_in_listener_f) sflowa_packet_in_handler) < 0) {
        AIM_LOG_ERROR("Failed to register for packet_in in SFLOW module");
        return INDIGO_ERROR_INIT;
    }

    /*
     * Register listener for port_status msg
     */
    if (indigo_core_port_status_listener_register(
        (indigo_core_port_status_listener_f) sflowa_port_status_handler) < 0) {
        AIM_LOG_ERROR("Failed to register for port_status in SFLOW module");
        return INDIGO_ERROR_INIT;
    }

    /*
     * Register debug counters
     */
    debug_counter_register(&sflow_counters.packet_in, "sflowa.packet_in",
                           "Sampled pkt's recv'd by sflowa");
    debug_counter_register(&sflow_counters.packet_out, "sflowa.packet_out",
                           "Sflow datagrams sent by sflowa");
    debug_counter_register(&sflow_counters.counter_request,
                           "sflowa.counter_request",
                           "Counter requests polled by sflowa");
    debug_counter_register(&sflow_counters.port_status_notification,
                           "sflowa.port_status_notification",
                           "Port status notif's recv'd by sflowa");
    debug_counter_register(&sflow_counters.port_features_update,
                           "sflowa.port_features_update",
                            "Port features updated by sflowa");

    aim_ratelimiter_init(&sflow_pktin_log_limiter, 1000*1000, 5, NULL);

    sflowa_initialized = true;
    sflowa_sampling_rate_handler = NULL;

    return INDIGO_ERROR_NONE;
}

/*
 * sflowa_finish
 *
 * API to deinit the Sflow Agent
 */
void
sflowa_finish(void)
{
    indigo_core_gentable_unregister(sflow_collector_table);
    indigo_core_gentable_unregister(sflow_sampler_table);
    indigo_core_packet_in_listener_unregister(sflowa_packet_in_handler);
    indigo_core_port_status_listener_unregister(sflowa_port_status_handler);

    /*
     * Unregister debug counters
     */
    debug_counter_unregister(&sflow_counters.packet_in);
    debug_counter_unregister(&sflow_counters.packet_out);
    debug_counter_unregister(&sflow_counters.counter_request);
    debug_counter_unregister(&sflow_counters.port_status_notification);
    debug_counter_unregister(&sflow_counters.port_features_update);

    sflowa_initialized = false;
    sflowa_sampling_rate_handler = NULL;
}

/*
 * sflowa_sampling_rate_handler_register
 *
 * Documented in sflowa.h
 */
void
sflowa_sampling_rate_handler_register(sflowa_sampling_rate_handler_f fn)
{
    sflowa_sampling_rate_handler = fn;
}

/*
 * sflowa_sampling_rate_handler_unregister
 *
 * Documented in sflowa.h
 */
void
sflowa_sampling_rate_handler_unregister(sflowa_sampling_rate_handler_f fn)
{
    if (sflowa_sampling_rate_handler != fn) {
        return;
    }

    sflowa_sampling_rate_handler = NULL;
}

/*
 * sflowa_receive_packet
 *
 * API to construct a flow sample and submit it to host sFlow agent
 * which will construct a sflow datagram for us
 *
 * This api can be used to send a sflow sampled packet directly
 * to the sflow agent
 */
static indigo_error_t
sflowa_receive_packet(of_octets_t *octets, of_port_no_t in_port)
{
    ppe_packet_t ppep;

    AIM_ASSERT(octets, "NULL input to pkt receive api");
    AIM_ASSERT(octets->data, "NULL data in pkt receive api");

    AIM_LOG_TRACE("Sampled packet_in received for in_port: %u", in_port);
    debug_counter_inc(&sflow_counters.packet_in);
    ++sampler_entries[in_port].stats.rx_packets;

    ppe_packet_init(&ppep, octets->data, octets->bytes);
    if (ppe_parse(&ppep) < 0) {
        AIM_LOG_RL_ERROR(&sflow_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed.");
        return INDIGO_ERROR_PARSE;
    }

    /*
     * Identify if this is an ethernet Packet
     */
    if (!ppe_header_get(&ppep, PPE_HEADER_8021Q) &&
        !ppe_header_get(&ppep, PPE_HEADER_ETHERNET)) {
        AIM_LOG_RL_ERROR(&sflow_pktin_log_limiter, os_time_monotonic(),
                         "Not an ethernet packet.");
        return INDIGO_ERROR_UNKNOWN;
    }

    /*
     * Get the sampler for this in_port
     */
    SFLSampler *sampler = sfl_agent_getSamplerByIfIndex(&dummy_agent, in_port);
    if (sampler == NULL) {
        AIM_LOG_ERROR("NULL Sampler for port: %u", in_port);
        return INDIGO_ERROR_UNKNOWN;
    }

    /*
     * Construct a flow record entry for this sample
     */
    SFLFlow_sample_element hdr_element = { 0 };
    hdr_element.tag = SFLFLOW_HEADER;

    hdr_element.flowType.header.frame_length = octets->bytes + 4;
    hdr_element.flowType.header.stripped = 4; //CRC_bytes

    /*
     * Set the header_protocol to ethernet
     */
    hdr_element.flowType.header.header_protocol = SFLHEADER_ETHERNET_ISO8023;
    hdr_element.flowType.header.header_length = (octets->bytes <
                                          sampler->sFlowFsMaximumHeaderSize)?
                                          octets->bytes :
                                          sampler->sFlowFsMaximumHeaderSize;
    hdr_element.flowType.header.header_bytes = octets->data;

    /*
     * Construct a flow sample
     */
    SFL_FLOW_SAMPLE_TYPE fs = { 0 };
    SFLADD_ELEMENT(&fs, &hdr_element);
    fs.input = in_port;

    /*
     * Estimate the sample pool from the samples.
     */
    sampler->samplePool += sampler->sFlowFsPacketSamplingRate;

    /*
     * Submit the flow sample to the sampler for constructing
     * a sflow datagram. Host sFlow agent will take care of filling
     * the rest of the fields.
     */
    sfl_sampler_writeFlowSample(sampler, &fs);

    return INDIGO_ERROR_NONE;
}

/*
 * sflowa_packet_in_handler
 *
 * API for handling incoming sflow samples
 */
indigo_core_listener_result_t
sflowa_packet_in_handler(of_packet_in_t *packet_in)
{
    of_octets_t  octets;
    of_port_no_t in_port;
    of_match_t   match;

    if (!sflowa_initialized) return INDIGO_CORE_LISTENER_RESULT_PASS;

    of_packet_in_data_get(packet_in, &octets);

    /*
     * Identify the ingress port
     */
    if (packet_in->version <= OF_VERSION_1_1) {

        /*
         * For packets with version <= OF 1.1, check the reason field
         * to identify if this is a sflow packet
         */
        uint8_t reason;
        of_packet_in_reason_get(packet_in, &reason);
        if (reason != SLSHARED_CONFIG_PACKET_IN_REASON_SFLOW) {
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }

        of_packet_in_in_port_get(packet_in, &in_port);
    } else {
        if (of_packet_in_match_get(packet_in, &match) < 0) {
            AIM_LOG_INTERNAL("match get failed");
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }

        /*
         * Check the packet-in reasons in metadata to
         * identify if this is a sflow packet
         */
        if (match.fields.metadata ^ OFP_BSN_PKTIN_FLAG_SFLOW) {
            AIM_LOG_INFO("Sflow flag not set");
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }

        in_port = match.fields.in_port;
    }

    if (in_port > SFLOWA_CONFIG_OF_PORTS_MAX) {
        AIM_LOG_INTERNAL("Port no: %u Out of Range %u",
                         in_port, SFLOWA_CONFIG_OF_PORTS_MAX);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    sflowa_receive_packet(&octets, in_port);
    return INDIGO_CORE_LISTENER_RESULT_DROP;
}

/*
 * sflowa_port_status_handler
 *
 * API for handling port_status update notification.
 *
 * On receipt of this notification, we will mark the port_features
 * cache to be stale and we will repopulate the port_features next
 * time we receive a get request for port counters.
 */
indigo_core_listener_result_t
sflowa_port_status_handler(of_port_status_t *port_status)
{
    AIM_LOG_TRACE("Received port_status notification, "
                  "mark port_features cache to be_stale");
    port_features_stale = true;
    debug_counter_inc(&sflow_counters.port_status_notification);
    return INDIGO_CORE_LISTENER_RESULT_PASS;
}

/*
 * sflow_get_send_mode
 *
 * Return sflow datagram sending mode - mgmt nw or dataplane
 */
static sflow_send_mode_t
sflow_get_send_mode(sflow_collector_entry_t *entry)
{
    /*
     * If vlan_id, agent_mac, agent_udp_sport and collector_mac
     * are all zero's then we can safely assume to send sflow
     * datagrams over management network.
     */
    if (!entry->value.vlan_id && !entry->value.agent_udp_sport
        && !memcmp(&entry->value.agent_mac, &zero_mac, OF_MAC_ADDR_BYTES)
        && !memcmp(&entry->value.collector_mac, &zero_mac, OF_MAC_ADDR_BYTES)) {
        return SFLOW_SEND_MODE_MGMT;
    }

    return SFLOW_SEND_MODE_DATAPLANE;
}

/*
 * sflow_timer
 *
 * Trigger a tick to the host sFlow agent
 */
void
sflow_timer(void *cookie)
{
    sfl_agent_tick(&dummy_agent, time(NULL));
}

/*
 * sflow_alloc
 *
 * Callback to allocate memory for host sFlow agent
 */
static void *
sflow_alloc(void *magic, SFLAgent *agent, size_t bytes)
{
    return aim_zmalloc(bytes);
}

/*
 * sflow_free
 *
 * Callback to free memory allocated by sflow_alloc()
 */
static int
sflow_free(void *magic, SFLAgent *agent, void *obj)
{
    aim_free(obj);
    return 1;
}

/*
 * sflow_error
 *
 * Callback to log errors in host sFlow agent
 */
static void
sflow_error(void *magic, SFLAgent *agent, char *msg)
{
    AIM_LOG_ERROR("%s", msg);
}

/*
 * sflow_send_packet_out
 *
 * Slap Ethernet + IP + UDP headers on a sflow datagram and
 * send it out on dataplane
 */
static void
sflow_send_packet_out(uint8_t *pkt, uint32_t pktLen,
                      sflow_collector_entry_t *entry)
{
    ppe_packet_t       ppep;
    of_packet_out_t    *obj;
    of_list_action_t   *list;
    of_action_output_t *action;
    indigo_error_t     rv;
    uint8_t            data[1600];

    AIM_ASSERT(sizeof(data) > SFLOW_PKT_HEADER_SIZE+pktLen, "Buffer overflow");
    SFLOWA_MEMSET(data, 0, sizeof(data));
    SFLOWA_MEMCPY(data+SFLOW_PKT_HEADER_SIZE, pkt, pktLen);

    ppe_packet_init(&ppep, data, SFLOW_PKT_HEADER_SIZE+pktLen);

    /*
     * Set ethertype as 802.1Q and type as IPv4
     * Parse to recognize tagged Ethernet packet.
     */
    data[12] = SLSHARED_CONFIG_ETHERTYPE_DOT1Q >> 8;
    data[13] = SLSHARED_CONFIG_ETHERTYPE_DOT1Q & 0xFF;
    data[16] = PPE_ETHERTYPE_IP4 >> 8;
    data[17] = PPE_ETHERTYPE_IP4 & 0xFF;
    if (ppe_parse(&ppep) < 0) {
        AIM_DIE("Packet_out parsing failed after IPv4 header");
        return;
    }

    /*
     * Set the Src Mac, Dest Mac and the Vlan-ID in the outgoing frame
     */
    ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_SRC_MAC,
                       entry->value.agent_mac.addr);
    ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_DST_MAC,
                       entry->value.collector_mac.addr);

    ppe_field_set(&ppep, PPE_FIELD_8021Q_VLAN, VLAN_VID(entry->value.vlan_id));
    ppe_field_set(&ppep, PPE_FIELD_8021Q_PRI, VLAN_PCP(entry->value.vlan_id));

    /*
     * Build the IP header, ip_proto = UDP
     */
    AIM_LOG_TRACE("Build IP header with src_ip: %{ipv4a}, dest_ip: %{ipv4a}",
                  entry->value.agent_ip, entry->key.collector_ip);
    ppe_build_ipv4_header(&ppep, entry->value.agent_ip, entry->key.collector_ip,
                          pktLen+SLSHARED_CONFIG_IPV4_HEADER_SIZE+
                          SLSHARED_CONFIG_UDP_HEADER_SIZE,
                          PPE_IP_PROTOCOL_UDP, 128);

    /*
     * Build the UDP header
     */
    AIM_LOG_TRACE("Build UDP header with sport: %u, dport: %u, length: %u",
                  entry->value.agent_udp_sport, entry->value.collector_udp_dport,
                  pktLen+SLSHARED_CONFIG_UDP_HEADER_SIZE);
    ppe_build_udp_header(&ppep, entry->value.agent_udp_sport,
                         entry->value.collector_udp_dport,
                         pktLen+SLSHARED_CONFIG_UDP_HEADER_SIZE);

    /*
     * Send the packet out on dataplane
     */
    obj = of_packet_out_new(OF_VERSION_1_3);
    AIM_TRUE_OR_DIE(obj != NULL);

    list = of_list_action_new(obj->version);
    AIM_TRUE_OR_DIE(list != NULL);

    action = of_action_output_new(list->version);
    AIM_TRUE_OR_DIE(action != NULL);

    of_packet_out_buffer_id_set(obj, -1);
    of_packet_out_in_port_set(obj, OF_PORT_DEST_CONTROLLER);
    of_action_output_port_set(action, OF_PORT_DEST_USE_TABLE);
    of_list_append(list, action);
    of_object_delete(action);
    rv = of_packet_out_actions_set(obj, list);
    AIM_ASSERT(rv == 0);
    of_object_delete(list);

    of_octets_t octets;
    octets.data = data;
    octets.bytes = SFLOW_PKT_HEADER_SIZE+pktLen;
    rv = of_packet_out_data_set(obj, &octets);
    if (rv < 0) {
        AIM_DIE("Failed to set data on Sflow packet out");
    }

    rv = indigo_fwd_packet_out(obj);
    if (rv < 0) {
        AIM_LOG_ERROR("Failed to send packet to collector: %{ipv4a}",
                      entry->key.collector_ip);
    } else {
        AIM_LOG_TRACE("Successfully sent %u bytes to collector: %{ipv4a}",
                      octets.bytes, entry->key.collector_ip);
        ++entry->stats.tx_packets;
        entry->stats.tx_bytes += octets.bytes;
    }

    of_packet_out_delete(obj);
}

/*
 * sflow_send_packet
 *
 * Callback to send a sflow datagram out to the collectors
 */
static void
sflow_send_packet(void *magic, SFLAgent *agent, SFLReceiver *receiver,
                  uint8_t *pkt, uint32_t pktLen)
{
    AIM_LOG_TRACE("Received callback to send packet with %u bytes", pktLen);

    /*
     * Loop through the list of collectors and depending on their send mode,
     * send the sflow datagram
     */
    list_head_t *list = sflow_collectors_list();
    list_links_t *cur;
    LIST_FOREACH(list, cur) {
        sflow_collector_entry_t *entry = container_of(cur, links,
                                                      sflow_collector_entry_t);

        /*
         * Change the agent ip and sub_agent_id in the sflow datagram
         * from dummy to actual.
         * Agent ip starts at byte 8 and sub agent id at byte 12.
         */
        uint32_t sub_agent_id = htonl(entry->value.sub_agent_id);
        uint32_t agent_ip = htonl(entry->value.agent_ip);
        memcpy(pkt+8, &agent_ip, sizeof(entry->value.agent_ip));
        memcpy(pkt+12, &sub_agent_id, sizeof(sub_agent_id));

        debug_counter_inc(&sflow_counters.packet_out);

        switch(sflow_get_send_mode(entry)) {
        case SFLOW_SEND_MODE_MGMT: {

            struct sockaddr_in sa;
            sa.sin_port = htons(entry->value.collector_udp_dport);
            sa.sin_family = AF_INET;
            sa.sin_addr.s_addr = htonl(entry->key.collector_ip);

            /*
             * Send to collector socket, since sflow can be lossy,
             * log any errors while sending - EAGAIN or EWOULDBLOCK, EINTR
             * and move on
             */
            int result = sendto(entry->sd, pkt, pktLen, 0,
                                (struct sockaddr *)&sa,
                                sizeof(struct sockaddr_in));
            if (result < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    AIM_LOG_WARN("socket: %d, buffer full");
                } else {
                    AIM_LOG_ERROR("socket: %d, sendto error: %s", entry->sd,
                                  strerror(errno));
                }
            } else if (result == 0) {
                AIM_LOG_ERROR("socket: %d, sendto returned 0: %s", entry->sd,
                              strerror(errno));
            } else {
                AIM_LOG_TRACE("socket: %d, successfully sent %u bytes to "
                              "collector: %{ipv4a}", entry->sd, pktLen,
                              entry->key.collector_ip);
                ++entry->stats.tx_packets;
                entry->stats.tx_bytes += pktLen;
            }

            break;
        }

        case SFLOW_SEND_MODE_DATAPLANE: {
            sflow_send_packet_out(pkt, pktLen, entry);
            break;
        }

        default:
            break;
        }
    }
}

/*
 * sflow_update_port_features
 *
 * Get the latest values of port speed, direction and if-status
 * from of_port_desc_stats_reply
 */
void
sflow_update_port_features(void)
{
    of_port_desc_stats_reply_t *reply;
    of_list_port_desc_t list;
    of_port_desc_t port_desc;
    int rv;
    of_port_no_t port_no;
    of_version_t version = OF_VERSION_1_3;

    /*
     * Refresh the port_features if a they are stale
     */
    if (!port_features_stale) return;

    AIM_LOG_TRACE("Refresh port_features");

    if ((reply = of_port_desc_stats_reply_new(version)) == NULL) {
        AIM_DIE("Failed to allocate port_desc_stats reply message");
    }

    if (indigo_port_desc_stats_get(reply) < 0) {
        of_object_delete(reply);
        return;
    }

    debug_counter_inc(&sflow_counters.port_features_update);

    /*
     * reset everything
     */
    memset(port_features, 0, sizeof(port_features));

    of_port_desc_stats_reply_entries_bind(reply, &list);
    OF_LIST_PORT_DESC_ITER(&list, &port_desc, rv) {
        of_port_desc_port_no_get(&port_desc, &port_no);
        if (port_no > SFLOWA_CONFIG_OF_PORTS_MAX) {
            continue;
        }

        /*
         * Get speed and direction
         */
        uint32_t curr = 0;
        of_port_desc_curr_get(&port_desc, &curr);
        if (OF_PORT_FEATURE_FLAG_10MB_HD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_10 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_HALF;
        } else if (OF_PORT_FEATURE_FLAG_10MB_FD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_10 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else if (OF_PORT_FEATURE_FLAG_100MB_HD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_100 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_HALF;
        } else if (OF_PORT_FEATURE_FLAG_100MB_FD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_100 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else if (OF_PORT_FEATURE_FLAG_1GB_HD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_1000 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_HALF;
        } else if (OF_PORT_FEATURE_FLAG_1GB_FD_TEST(curr, version)) {
            port_features[port_no].speed = SPEED_1000 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else if (OF_PORT_FEATURE_FLAG_10GB_FD_TEST(curr, version)) {
            port_features[port_no].speed = (uint64_t)SPEED_10000 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else if (OF_PORT_FEATURE_FLAG_40GB_FD_TEST(curr, version)) {
            port_features[port_no].speed = (uint64_t)40000 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else if (OF_PORT_FEATURE_FLAG_100GB_FD_TEST(curr, version)) {
            port_features[port_no].speed = (uint64_t)100000 * 1000000;
            port_features[port_no].direction = SFLOW_DUPLEX_FULL;
        } else {
            AIM_LOG_VERBOSE("Unsupported feature type: (%u)", curr);
        }

        /*
         * Get if-status
         */
        uint32_t config = 0, state = 0;
        of_port_desc_config_get(&port_desc, &config);
        of_port_desc_state_get(&port_desc, &state);

        /*
         * ifAdminStatus: bit 0 = ifAdminStatus (0 = down, 1 = up)
         */
        if (!OF_PORT_CONFIG_FLAG_PORT_DOWN_TEST(config, version)) {
            port_features[port_no].status |= IF_ADMIN_UP;
        }

        /*
         * ifOperstatus: bit 1 = ifOperStatus (0 = down, 1 = up)
         */
        if (!OF_PORT_STATE_FLAG_LINK_DOWN_TEST(state, version)) {
            port_features[port_no].status |= IF_OPER_UP;
        }

        AIM_LOG_TRACE("Port: %u, speed: %" PRId64 ", direction: %u, status: %u",
                      port_no, port_features[port_no].speed,
                      port_features[port_no].direction,
                      port_features[port_no].status);
    }

    port_features_stale = false;
    of_object_delete(reply);
}

/*
 * sflow_get_counters
 *
 * Callback to get interface counters
 * For now we only support generic interface counters
 */
static void
sflow_get_counters(void *magic, SFLPoller *poller, SFL_COUNTERS_SAMPLE_TYPE *cs)
{
    SFLCounters_sample_element elem = { 0 };
    indigo_fi_port_stats_t stats;
    of_port_no_t port_no = SFL_DS_INDEX(poller->dsi);

    AIM_LOG_TRACE("Received callback to get counters for port: %u", port_no);

    /*
     * Default to "counter not supported"
     */
    memset(&stats, 0xff, sizeof(stats));

    debug_counter_inc(&sflow_counters.counter_request);

    indigo_port_extended_stats_get(port_no, &stats);
    sflow_update_port_features();

    /*
     * Generic interface counters
     */
    elem.tag = SFLCOUNTERS_GENERIC;
    elem.counterBlock.generic.ifIndex = port_no;
    elem.counterBlock.generic.ifType = 6; // Ethernet
    elem.counterBlock.generic.ifSpeed = port_features[port_no].speed;
    elem.counterBlock.generic.ifDirection = port_features[port_no].direction;
    elem.counterBlock.generic.ifStatus = port_features[port_no].status;
    elem.counterBlock.generic.ifPromiscuousMode = 1; //PromiscuousMode = ON

    elem.counterBlock.generic.ifInOctets = stats.rx_bytes;
    elem.counterBlock.generic.ifInUcastPkts = (uint32_t)stats.rx_packets_unicast;
    elem.counterBlock.generic.ifInMulticastPkts = (uint32_t)stats.rx_packets_multicast;
    elem.counterBlock.generic.ifInBroadcastPkts = (uint32_t)stats.rx_packets_broadcast;
    elem.counterBlock.generic.ifInDiscards = (uint32_t)stats.rx_dropped;
    elem.counterBlock.generic.ifInErrors = (uint32_t)stats.rx_errors;
    elem.counterBlock.generic.ifInUnknownProtos = UINT32_MAX; //UNSUPPORTED
    elem.counterBlock.generic.ifOutOctets = stats.tx_bytes;
    elem.counterBlock.generic.ifOutUcastPkts = (uint32_t)stats.tx_packets_unicast;
    elem.counterBlock.generic.ifOutMulticastPkts = (uint32_t)stats.tx_packets_multicast;
    elem.counterBlock.generic.ifOutBroadcastPkts = (uint32_t)stats.tx_packets_broadcast;
    elem.counterBlock.generic.ifOutDiscards = (uint32_t)stats.tx_dropped;
    elem.counterBlock.generic.ifOutErrors = (uint32_t)stats.tx_errors;

    SFLADD_ELEMENT(cs, &elem);
    sfl_poller_writeCountersSample(poller, cs);
}

/*
 * sflow_add_hsflow_agent
 *
 * Init the dummy host sflow agent when the first
 * sampler table entry is added
 */
static void
sflow_add_hsflow_agent(void)
{
    if (sflow_enabled_ports == 0) {

        AIM_LOG_TRACE("Init the host sflow agent");

        /*
         * Register a 1 sec timer which would send a tick
         * to the host sFlow agent
         */
        indigo_error_t rv;
        if ((rv = ind_soc_timer_event_register(sflow_timer, NULL, 1000)) < 0) {
            AIM_DIE("Failed to register Sflow agent timer: %s",
                    indigo_strerror(rv));
        }

        /*
         * Init the dummy agent
         */
        SFLAddress dummy_ip = {
            .type = SFLADDRESSTYPE_IP_V4,
            .address.ip_v4.addr = 0,
        };
        sfl_agent_init(&dummy_agent, &dummy_ip, 1, time(NULL), time(NULL), NULL,
                       sflow_alloc, sflow_free, sflow_error, sflow_send_packet);

        /*
         * Add a dummy receiver
         */
        SFLReceiver *dummy_receiver = sfl_agent_addReceiver(&dummy_agent);

        /*
         * set the receiver timeout to infinity
         */
        sfl_receiver_set_sFlowRcvrTimeout(dummy_receiver, 0xFFFFFFFF);
    }

    ++sflow_enabled_ports;
}

/*
 * sflow_remove_hsflow_agent
 *
 * Deinit the dummy host sflow agent when the last
 * sampler table entry is deleted
 */
static void
sflow_remove_hsflow_agent(void)
{
    --sflow_enabled_ports;

    if (sflow_enabled_ports == 0) {
        AIM_LOG_TRACE("Deinit the host sflow agent");
        ind_soc_timer_event_unregister(sflow_timer, NULL);

        /*
         * Deinit the dummy agent, this will remove the dummy receiver as well
         */
        sfl_agent_release(&dummy_agent);
    }
}

/*
 * sflow_sampling_rate_notify
 *
 * Notify handler about the change in sampling rate
 */
static indigo_error_t
sflow_sampling_rate_notify(of_port_no_t port_no, uint32_t sampling_rate)
{
    if (sflowa_sampling_rate_handler == NULL) {
        return INDIGO_ERROR_INIT;
    }

    return (*sflowa_sampling_rate_handler)(port_no, sampling_rate);
}

/*
 * sflow_collectors_list
 *
 * Return a list of sflow collector entries
 *
 * The list is through the 'links' field of sflow_collector_entry_t.
 */
list_head_t *
sflow_collectors_list(void)
{
    return &sflow_collectors;
}

/*
 * sflow_init_socket
 *
 * Open a UDP Socket to send sflow datagrams to the collector
 * and init socket params
 */
static indigo_error_t
sflow_init_socket(sflow_collector_entry_t *entry)
{
    if (sflow_get_send_mode(entry) != SFLOW_SEND_MODE_MGMT) {
        return INDIGO_ERROR_NONE;
    }

    /*
     * open the socket if not open already
     */
    if (entry->sd <= 0) {
        entry->sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (entry->sd < 0) {
            AIM_LOG_ERROR("Failed to create collector socket: %s", strerror(errno));
            return INDIGO_ERROR_UNKNOWN;
        } else {

            AIM_LOG_TRACE("Created socket: %d, for collector: %{ipv4a}", entry->sd,
                          entry->key.collector_ip);

            /*
             * Make the socket non-blocking
             */
            int soc_flags = fcntl(entry->sd, F_GETFL, 0);
            if (soc_flags == -1 || fcntl(entry->sd, F_SETFL,
                                         soc_flags | O_NONBLOCK) == -1) {
                AIM_LOG_ERROR("Failed to set non-blocking flag for socket: %s",
                              strerror(errno));
                close(entry->sd);
                return INDIGO_ERROR_UNKNOWN;
            }
        }
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_parse_key
 *
 * Parse key for sflow_collector table entry from tlv list
 */
static indigo_error_t
sflow_collector_parse_key(of_list_bsn_tlv_t *tlvs,
                          sflow_collector_entry_key_t *key)
{
    of_object_t tlv;

    SFLOWA_MEMSET(key, 0, sizeof(*key));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty key list");
        return INDIGO_ERROR_PARAM;
    }

    /* Collector ip */
    if (tlv.object_id == OF_BSN_TLV_IPV4_DST) {
        of_bsn_tlv_ipv4_dst_value_get(&tlv, &key->collector_ip);
    } else {
        AIM_LOG_ERROR("expected ipv4_dst key TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of key list, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_parse_value
 *
 * Parse values for sflow_collector table entry from tlv list
 */
static indigo_error_t
sflow_collector_parse_value(of_list_bsn_tlv_t *tlvs,
                            sflow_collector_entry_value_t *value)
{
    of_object_t tlv;

    SFLOWA_MEMSET(value, 0, sizeof(*value));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Vlan id */
    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &value->vlan_id);
    } else {
        AIM_LOG_ERROR("expected vlan value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }


    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Agent mac */
    if (tlv.object_id == OF_BSN_TLV_ETH_SRC) {
        of_bsn_tlv_eth_src_value_get(&tlv, &value->agent_mac);
    } else {
        AIM_LOG_ERROR("expected eth_src value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Agent ip */
    if (tlv.object_id == OF_BSN_TLV_IPV4_SRC) {
        of_bsn_tlv_ipv4_src_value_get(&tlv, &value->agent_ip);
    } else {
        AIM_LOG_ERROR("expected ipv4_src value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Agent udp src port */
    if (tlv.object_id == OF_BSN_TLV_UDP_SRC) {
        of_bsn_tlv_udp_src_value_get(&tlv, &value->agent_udp_sport);
    } else {
        AIM_LOG_ERROR("expected udp_src value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Collector mac */
    if (tlv.object_id == OF_BSN_TLV_ETH_DST) {
        of_bsn_tlv_eth_dst_value_get(&tlv, &value->collector_mac);
    } else {
        AIM_LOG_ERROR("expected eth_dst value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Collector udp dst port */
    if (tlv.object_id == OF_BSN_TLV_UDP_DST) {
        of_bsn_tlv_udp_dst_value_get(&tlv,
                                     &value->collector_udp_dport);
    } else {
        AIM_LOG_ERROR("expected udp_dst value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Sub agent id */
    if (tlv.object_id == OF_BSN_TLV_SUB_AGENT_ID) {
        of_bsn_tlv_sub_agent_id_value_get(&tlv,
                                          &value->sub_agent_id);
    } else {
        AIM_LOG_ERROR("expected sub_agent_id value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of value list, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_add
 *
 * Add a new entry to sflow_collector table
 */
static indigo_error_t
sflow_collector_add(void *table_priv, of_list_bsn_tlv_t *key_tlvs,
                    of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    sflow_collector_entry_key_t key;
    sflow_collector_entry_value_t value;
    sflow_collector_entry_t *entry;

    rv = sflow_collector_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = sflow_collector_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    rv = sflow_init_socket(entry);
    if (rv < 0) {
        aim_free(entry);
        return rv;
    }

    /*
     * Add this entry to a list to be used later for sending a sflow datagram out
     */
    list_push(&sflow_collectors, &entry->links);

    AIM_LOG_TRACE("Add collector table entry, collector_ip: %{ipv4a} -> vlan_id:"
                  " %u, agent_mac: %{mac}, agent_ip: %{ipv4a}, agent_udp_sport:"
                  " %u, collector_mac: %{mac}, collector_udp_dport: %u, "
                  "sub_agent_id: %u", entry->key.collector_ip,
                  entry->value.vlan_id, entry->value.agent_mac.addr,
                  entry->value.agent_ip, entry->value.agent_udp_sport,
                  entry->value.collector_mac.addr,
                  entry->value.collector_udp_dport, entry->value.sub_agent_id);

    *entry_priv = entry;

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_modify
 *
 * Modify a existing entry in sflow_collector table
 */
static indigo_error_t
sflow_collector_modify(void *table_priv, void *entry_priv,
                       of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    sflow_collector_entry_value_t value;
    sflow_collector_entry_t *entry = entry_priv;

    rv = sflow_collector_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    AIM_LOG_TRACE("Modify collector table entry, old collector_ip: %{ipv4a} ->"
                  " vlan_id:%u, agent_mac: %{mac}, agent_ip: %{ipv4a}, "
                  "agent_udp_sport: %u, collector_mac: %{mac}, "
                  "collector_udp_dport: %u, sub_agent_id: %u",
                  entry->key.collector_ip, entry->value.vlan_id,
                  entry->value.agent_mac.addr, entry->value.agent_ip,
                  entry->value.agent_udp_sport, entry->value.collector_mac.addr,
                  entry->value.collector_udp_dport, entry->value.sub_agent_id);

    AIM_LOG_TRACE("New, collector_ip: %{ipv4a} -> vlan_id: %u, agent_mac: "
                  "%{mac}, agent_ip: %{ipv4a}, agent_udp_sport: %u, "
                  "collector_mac: %{mac}, collector_udp_dport: %u, "
                  "sub_agent_id: %u", entry->key.collector_ip, value.vlan_id,
                  value.agent_mac.addr, value.agent_ip, value.agent_udp_sport,
                  value.collector_mac.addr, value.collector_udp_dport,
                  value.sub_agent_id);

    entry->value = value;

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_delete
 *
 * Remove a entry from sflow_collector table
 */
static indigo_error_t
sflow_collector_delete(void *table_priv, void *entry_priv,
                       of_list_bsn_tlv_t *key_tlvs)
{
    sflow_collector_entry_t *entry = entry_priv;

    AIM_LOG_TRACE("Delete collector table entry, collector_ip: %{ipv4a} -> vlan_id:"
                  " %u, agent_mac: %{mac}, agent_ip: %{ipv4a}, agent_udp_sport:"
                  " %u, collector_mac: %{mac}, collector_udp_dport: %u, "
                  "sub_agent_id: %u", entry->key.collector_ip,
                  entry->value.vlan_id, entry->value.agent_mac.addr,
                  entry->value.agent_ip, entry->value.agent_udp_sport,
                  entry->value.collector_mac.addr,
                  entry->value.collector_udp_dport, entry->value.sub_agent_id);

    /*
     * Delete this entry from the list
     */
    list_remove(&entry->links);

    /*
     * Close the socket
     */
    if (entry->sd > 0) close(entry->sd);

    aim_free(entry);

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_collector_get_stats
 *
 * Return the stats related with a entry in sflow_collector table
 */
static void
sflow_collector_get_stats(void *table_priv, void *entry_priv,
                          of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    sflow_collector_entry_t *entry = entry_priv;

    /* tx_packets */
    {
        of_bsn_tlv_tx_packets_t tlv;
        of_bsn_tlv_tx_packets_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_tx_packets_value_set(&tlv, entry->stats.tx_packets);
    }

    /* tx_bytes */
    {
        of_bsn_tlv_tx_bytes_t tlv;
        of_bsn_tlv_tx_bytes_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_tx_bytes_value_set(&tlv, entry->stats.tx_bytes);
    }
}

static const indigo_core_gentable_ops_t sflow_collector_ops = {
    .add = sflow_collector_add,
    .modify = sflow_collector_modify,
    .del = sflow_collector_delete,
    .get_stats = sflow_collector_get_stats,
};

/*
 * sflow_sampler_parse_key
 *
 * Parse key for sflow_sampler table entry from tlv list
 */
static indigo_error_t
sflow_sampler_parse_key(of_list_bsn_tlv_t *tlvs, sflow_sampler_entry_key_t *key)
{
    of_object_t tlv;

    SFLOWA_MEMSET(key, 0, sizeof(*key));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty key list");
        return INDIGO_ERROR_PARAM;
    }

    /* port */
    if (tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&tlv, &key->port_no);
    } else {
        AIM_LOG_ERROR("expected port key TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (key->port_no > SFLOWA_CONFIG_OF_PORTS_MAX) {
        AIM_LOG_ERROR("Port out of range (%u)", key->port_no);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of key list, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_parse_value
 *
 * Parse values for sflow_sampler table entry from tlv list
 */
static indigo_error_t
sflow_sampler_parse_value(of_list_bsn_tlv_t *tlvs,
                          sflow_sampler_entry_value_t *value)
{
    of_object_t tlv;

    SFLOWA_MEMSET(value, 0, sizeof(*value));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Sampling rate */
    if (tlv.object_id == OF_BSN_TLV_SAMPLING_RATE) {
        of_bsn_tlv_sampling_rate_value_get(&tlv,
                                           &value->sampling_rate);
    } else {
        AIM_LOG_ERROR("expected sampling_rate value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Header size */
    if (tlv.object_id == OF_BSN_TLV_HEADER_SIZE) {
        of_bsn_tlv_header_size_value_get(&tlv, &value->header_size);
    } else {
        AIM_LOG_ERROR("expected header_size value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (value->header_size > SFLOW_MAX_HEADER_SIZE) {
        AIM_LOG_ERROR("Header size out of range (%u), maximum allowed %u",
                      value->header_size, SFLOW_MAX_HEADER_SIZE);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /* Polling interval */
    if (tlv.object_id == OF_BSN_TLV_INTERVAL) {
        of_bsn_tlv_interval_value_get(&tlv, &value->polling_interval);

        /*
         * Convert polling_interval from milliseconds to seconds
         * polling_interval < 1000, will get computed as zero
         */
        value->polling_interval = 1.0*(value->polling_interval)/1000;
    } else {
        AIM_LOG_ERROR("expected interval value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of value list, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_add
 *
 * Add a new entry to sflow_sampler table
 */
static indigo_error_t
sflow_sampler_add(void *table_priv, of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    sflow_sampler_entry_key_t key;
    sflow_sampler_entry_value_t value;

    rv = sflow_sampler_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = sflow_sampler_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    /*
     * Send notifications to enable sampling on this port
     */
    rv = sflow_sampling_rate_notify(key.port_no, value.sampling_rate);
    if (rv < 0) {
        return rv;
    }

    sflow_sampler_entry_t *entry = &sampler_entries[key.port_no];
    entry->key = key;
    entry->value = value;

    AIM_LOG_TRACE("Add sampler table entry, port: %u -> sampling_rate: %u, "
                  "header_size: %u, polling_interval: %u", entry->key.port_no,
                  entry->value.sampling_rate, entry->value.header_size,
                  entry->value.polling_interval);

    *entry_priv = entry;

    sflow_add_hsflow_agent();

    /*
     * Add sampler for this port
     */
    SFLDataSource_instance dsi;
    SFL_DS_SET(dsi, 0, entry->key.port_no, 0);
    SFLSampler *sampler = sfl_agent_addSampler(&dummy_agent, &dsi);
    sfl_sampler_set_sFlowFsPacketSamplingRate(sampler,
                                              entry->value.sampling_rate);
    sfl_sampler_set_sFlowFsMaximumHeaderSize(sampler, entry->value.header_size);
    sfl_sampler_set_sFlowFsReceiver(sampler, SFLOW_RECEIVER_INDEX);

    /*
     * Add poller for this port if polling_interval is non zero
     */
    if (entry->value.polling_interval) {
        AIM_LOG_TRACE("%s: Add poller for port: %u", __FUNCTION__,
                      entry->key.port_no);
        SFLPoller *poller = sfl_agent_addPoller(&dummy_agent, &dsi, NULL,
                                                sflow_get_counters);
        sfl_poller_set_sFlowCpInterval(poller, entry->value.polling_interval);
        sfl_poller_set_sFlowCpReceiver(poller, SFLOW_RECEIVER_INDEX);
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_modify
 *
 * Modify a existing entry in sflow_sampler table
 */
static indigo_error_t
sflow_sampler_modify(void *table_priv, void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    sflow_sampler_entry_value_t value;
    sflow_sampler_entry_t *entry = entry_priv;

    rv = sflow_sampler_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    /*
     * Notify about the change in sampling rate on this port
     */
    rv = sflow_sampling_rate_notify(entry->key.port_no, value.sampling_rate);
    if (rv < 0) {
        return rv;
    }

    AIM_LOG_TRACE("Modify sampler table entry, port: %u -> from sampling_rate: "
                  "%u, header_size: %u, polling_interval: %u to sampling_rate: "
                  "%u, header_size: %u, polling_interval: %u",
                  entry->key.port_no, entry->value.sampling_rate,
                  entry->value.header_size, entry->value.polling_interval,
                  value.sampling_rate, value.header_size, value.polling_interval);

    uint32_t prev_polling_interval = entry->value.polling_interval;
    entry->value = value;

    /*
     * Update Sampler fields
     */
    SFLSampler *sampler = sfl_agent_getSamplerByIfIndex(&dummy_agent,
                                                        entry->key.port_no);
    AIM_ASSERT(sampler, "Sampler table modify: NULL Sampler");
    sfl_sampler_set_sFlowFsPacketSamplingRate(sampler,
                                              entry->value.sampling_rate);
    sfl_sampler_set_sFlowFsMaximumHeaderSize(sampler, entry->value.header_size);

    /*
     * Update Poller fields
     *
     * Modify is a bit complicated:
     * 1) Identify if there is a change in polling_interval.
     *    Set polling_interval only if changed, beacuse it will reset the
     *    random poll countdown too and we dont want to change the countdown
     *    if there is no change in the polling_interval.
     * 2) If there is a change:
     *    (a) 0 -> polling_interval => Add poller with polling_interval
     *    (b) x -> y => Set new polling_interval
     *    (c) polling_interval -> 0 => Remove poller
     */
    if (prev_polling_interval != entry->value.polling_interval) {
        SFLDataSource_instance dsi;
        SFL_DS_SET(dsi, 0, entry->key.port_no, 0);
        if (entry->value.polling_interval != 0) {
            SFLPoller *poller = sfl_agent_getPoller(&dummy_agent, &dsi);
            if (poller == NULL) {
                AIM_LOG_TRACE("%s: Add poller for port: %u", __FUNCTION__,
                              entry->key.port_no);
                poller = sfl_agent_addPoller(&dummy_agent, &dsi, NULL,
                                             sflow_get_counters);
                sfl_poller_set_sFlowCpReceiver(poller, SFLOW_RECEIVER_INDEX);
            }

            sfl_poller_set_sFlowCpInterval(poller,
                                           entry->value.polling_interval);
        } else {
            AIM_LOG_TRACE("%s: Remove poller for port: %u", __FUNCTION__,
                          entry->key.port_no);
            sfl_agent_removePoller(&dummy_agent, &dsi);
        }
    }

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_delete
 *
 * Remove a entry from sflow_sampler table
 */
static indigo_error_t
sflow_sampler_delete(void *table_priv, void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs)
{
    sflow_sampler_entry_t *entry = entry_priv;

    AIM_LOG_TRACE("Delete sampler table entry, port: %u -> sampling_rate: %u, "
                  "header_size: %u, polling_interval: %u", entry->key.port_no,
                  entry->value.sampling_rate, entry->value.header_size,
                  entry->value.polling_interval);

    /*
     * Set the sampling rate to 0 to disable sampling on this port
     * Send notifications to disable sampling on this port
     *
     * Also remove the Sampler and Poller instances.
     */
    sflow_sampling_rate_notify(entry->key.port_no, 0);

    SFLDataSource_instance dsi;
    SFL_DS_SET(dsi, 0, entry->key.port_no, 0);
    sfl_agent_removeSampler(&dummy_agent, &dsi);
    sfl_agent_removePoller(&dummy_agent, &dsi);
    sflow_remove_hsflow_agent();

    SFLOWA_MEMSET(entry, 0, sizeof(*entry));

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_get_stats
 *
 * Return the stats related with a entry in sflow_sampler table
 */
static void
sflow_sampler_get_stats(void *table_priv, void *entry_priv,
                        of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    sflow_sampler_entry_t *entry = entry_priv;

    /* rx_packets */
    {
        of_bsn_tlv_rx_packets_t tlv;
        of_bsn_tlv_rx_packets_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_rx_packets_value_set(&tlv, entry->stats.rx_packets);
    }
}

static const indigo_core_gentable_ops_t sflow_sampler_ops = {
    .add = sflow_sampler_add,
    .modify = sflow_sampler_modify,
    .del = sflow_sampler_delete,
    .get_stats = sflow_sampler_get_stats,
};
