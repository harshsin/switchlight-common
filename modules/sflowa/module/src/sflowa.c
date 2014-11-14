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
#include <debug_counter/debug_counter.h>
#include <OS/os_time.h>
#include <PPE/ppe.h>
#include <fcntl.h>
#include "sflowa_int.h"
#include "sflowa_log.h"

/*
 * only one dummy receiver, so the receiverIndex is a constant
 */
#define SFLOW_RECEIVER_INDEX 1

static const of_mac_addr_t zero_mac = { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static indigo_core_gentable_t *sflow_collector_table;
static indigo_core_gentable_t *sflow_sampler_table;

static const indigo_core_gentable_ops_t sflow_collector_ops;
static const indigo_core_gentable_ops_t sflow_sampler_ops;

static bool sflowa_initialized = false;
static uint16_t sflow_enabled_ports = 0;

static sflow_sampler_entry_t sampler_entries[SFLOWA_CONFIG_OF_PORTS_MAX+1];
static LIST_DEFINE(sflow_collectors);

static sflowa_sampling_rate_handler_f sflowa_sampling_rate_handler;

static SFLAgent dummy_agent;
aim_ratelimiter_t sflow_pktin_log_limiter;

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
    AIM_ASSERT(sampler, "Packet_in handler: NULL Sampler");

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
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    } else {
        if (of_packet_in_match_get(packet_in, &match) < 0) {
            AIM_LOG_INTERNAL("match get failed");
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }
        in_port = match.fields.in_port;
    }

    if (in_port > SFLOWA_CONFIG_OF_PORTS_MAX) {
        AIM_LOG_INTERNAL("Port no: %u Out of Range %u",
                         in_port, SFLOWA_CONFIG_OF_PORTS_MAX);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Check the packet-in reasons in metadata to
     * identify if this is a sflow packet
     */
    if (match.fields.metadata ^ OFP_BSN_PKTIN_FLAG_SFLOW) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    sflowa_receive_packet(&octets, in_port);
    return INDIGO_CORE_LISTENER_RESULT_DROP;
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
            }

            break;
        }

        case SFLOW_SEND_MODE_DATAPLANE:
            break;

        default:
            break;
        }
    }
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
static void
sflow_sampling_rate_notify(of_port_no_t port_no, uint32_t sampling_rate)
{
    if (sflowa_sampling_rate_handler != NULL) {
        (*sflowa_sampling_rate_handler)(port_no, sampling_rate);
    }
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

    sflow_sampler_entry_t *entry = &sampler_entries[key.port_no];
    entry->key = key;
    entry->value = value;

    AIM_LOG_TRACE("Add sampler table entry, port: %u -> sampling_rate: %u, "
                  "header_size: %u", entry->key.port_no,
                  entry->value.sampling_rate, entry->value.header_size);

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

    //Todo: add a poller for this interface too

    /*
     * Send notifications to enable sampling on this port
     */
    sflow_sampling_rate_notify(entry->key.port_no, entry->value.sampling_rate);
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

    AIM_LOG_TRACE("Modify sampler table entry, port: %u -> from sampling_rate: "
                  "%u, header_size: %u to sampling_rate: %u, header_size: %u",
                  entry->key.port_no, entry->value.sampling_rate,
                  entry->value.header_size, value.sampling_rate,
                  value.header_size);

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
     * Notify about the change in sampling rate on this port
     */
    sflow_sampling_rate_notify(entry->key.port_no, entry->value.sampling_rate);

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
                  "header_size: %u", entry->key.port_no,
                  entry->value.sampling_rate, entry->value.header_size);

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
    sflow_remove_hsflow_agent();

    SFLOWA_MEMSET(entry, 0, sizeof(*entry));

    return INDIGO_ERROR_NONE;
}

/*
 * sflow_sampler_get_stats
 *
 * Dummy function
 */
static void
sflow_sampler_get_stats(void *table_priv, void *entry_priv,
                        of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    /* No stats */
}

static const indigo_core_gentable_ops_t sflow_sampler_ops = {
    .add = sflow_sampler_add,
    .modify = sflow_sampler_modify,
    .del = sflow_sampler_delete,
    .get_stats = sflow_sampler_get_stats,
};
