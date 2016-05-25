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
 * either express or implied. See the License for the shard
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

/*
 * Implementation of BMF Collect Agent
 *
 * This file contains code for initializing collect agent and
 * associated gentable (collect_collector) operations
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/ethtool.h>
#include <netinet/in.h>
#include <AIM/aim.h>
#include <OS/os_time.h>
#include <PPE/ppe.h>
#include <Configuration/configuration.h>
#include <indigo/of_state_manager.h>
#include <slshared/slshared_config.h>
#include <collecta/collecta.h>

#include "collecta_int.h"
#include "collecta_log.h"

/*
 * Collect header format
0                           32
 ---------------------------
|datagram version(=1)       |
 --------------------------- 
|port                       |
 ---------------------------
|datapath_id                |
 ---------------------------
|datapath_id(cont)          |
 --------------------------- 
*/
typedef struct collect_header_s {
    uint32_t version;
    uint32_t in_port;
    uint64_t datapath_id;
} collect_header_t;

static bool collecta_initialized = false;
static indigo_core_gentable_t *collect_collector_table;
static const indigo_core_gentable_ops_t collect_collector_ops;
static LIST_DEFINE(collect_collectors);
static indigo_core_listener_result_t collecta_packet_in_handler(of_packet_in_t *packet_in);

collect_debug_counters_t collect_counters;
aim_ratelimiter_t collect_pktin_log_limiter;

/*
 * collecta_init
 *
 * API to init the Collect Agent
 * This should only be done once at the beginning.
 */
indigo_error_t
collecta_init(void)
{
    if (collecta_initialized) return INDIGO_ERROR_NONE;

    AIM_LOG_TRACE("init");

    /* register for config ops to get datapath id */
    ind_cfg_register(&collecta_cfg_ops);

    indigo_core_gentable_register("collect_collector", &collect_collector_ops, NULL,
                                  4, 4, &collect_collector_table);

#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    /*
     * Register listener for packet_in
     */
    if (indigo_core_packet_in_listener_register(
        (indigo_core_packet_in_listener_f) collecta_packet_in_handler) < 0) {
        AIM_LOG_ERROR("Failed to register for packet_in in COLLECT module");
        return INDIGO_ERROR_INIT;
    }
#endif

    /*
     * Register debug counters
     */
    debug_counter_register(&collect_counters.packet_in, "collecta.packet_in",
                           "Sampled pkt's recv'd by collecta");
    debug_counter_register(&collect_counters.packet_out, "collecta.packet_out",
                           "Collect datagrams sent by collecta");

    aim_ratelimiter_init(&collect_pktin_log_limiter, 1000*1000, 5, NULL);

    collecta_initialized = true;

    return INDIGO_ERROR_NONE;
}

/*
 * collecta_finish
 *
 * API to deinit the Collect Agent
 */
void
collecta_finish(void)
{
    indigo_core_gentable_unregister(collect_collector_table);
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_unregister(collecta_packet_in_handler);
#endif

    /*
     * Unregister debug counters
     */
    debug_counter_unregister(&collect_counters.packet_in);
    debug_counter_unregister(&collect_counters.packet_out);

    collecta_initialized = false;
}

/*
 * collect_collectors_list
 *
 * Return a list of collect collector entries
 *
 * The list is through the 'links' field of collect_collector_entry_t.
 */
list_head_t *
collect_collectors_list(void)
{
    return &collect_collectors;
}

/*
 * collect_send_packet
 *
 * Callback to send a collect datagram out to the collectors
 */
static void
collect_send_packet(uint8_t *pkt, uint32_t pktLen)
{
    AIM_LOG_TRACE("Received callback to send packet with %u bytes", pktLen);

    /*
     * Loop through the list of collectors and depending on their send mode,
     * send the collect datagram
     */
    list_head_t *list = collect_collectors_list();
    list_links_t *cur;
    LIST_FOREACH(list, cur) {
        collect_collector_entry_t *entry = container_of(cur, links,
                                                        collect_collector_entry_t);

        debug_counter_inc(&collect_counters.packet_out);

        struct sockaddr_in sa;
        sa.sin_port = htons(entry->value.collector_udp_dport);
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(entry->key.collector_ip);

        /*
         * Send to collector socket, since collect can be lossy,
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
    }
}

/*
 * collecta_receive_packet
 *
 * API to construct a flow sample and submit it to host sFlow agent
 * which will construct a collect datagram for us
 *
 * This api can be used to send a collect sampled packet directly
 * to the collect agent
 */
static indigo_core_listener_result_t
collecta_receive_packet(ppe_packet_t *ppep, of_port_no_t in_port)
{
    uint32_t pktLen;
    uint8_t *pkt;
    collect_header_t *hdr;
    if (in_port == OF_PORT_DEST_CONTROLLER) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    AIM_LOG_TRACE("packet_in received for in_port: %u", in_port);

    debug_counter_inc(&collect_counters.packet_in);

    /*
     * Identify if this is an ethernet Packet
     */
    if (!ppe_header_get(ppep, PPE_HEADER_8021Q) &&
        !ppe_header_get(ppep, PPE_HEADER_ETHERNET)) {
        AIM_LOG_RL_ERROR(&collect_pktin_log_limiter, os_time_monotonic(),
                         "Not an ethernet packet.");
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Construct a collect datagram and send out.
     */
    pktLen = ppep->size + sizeof(collect_header_t);
    pkt = aim_zmalloc((size_t)pktLen);
    hdr = (collect_header_t *)pkt;
    hdr->version = 1;
    hdr->datapath_id = datapath_id;
    hdr->in_port = in_port;
    memcpy(pkt + sizeof(*hdr), ppep->data, ppep->size);
    collect_send_packet(pkt, pktLen);
    aim_free(pkt);

    return INDIGO_CORE_LISTENER_RESULT_PASS;
}

/*
 * collecta_packet_in_handler
 *
 * API for handling incoming collect samples
 */
static indigo_core_listener_result_t
collecta_packet_in_handler(of_packet_in_t *packet_in)
{
    of_octets_t  octets;
    of_port_no_t in_port;
    of_match_t   match;

    if (!collecta_initialized) return INDIGO_CORE_LISTENER_RESULT_PASS;

    of_packet_in_data_get(packet_in, &octets);

    /*
     * Identify the ingress port
     */
    if (of_packet_in_match_get(packet_in, &match) < 0) {
        AIM_LOG_INTERNAL("match get failed");
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Check the packet-in reasons in metadata to
     * identify if this is a packet in.
     * TODO: add a unique bit for this
     */
    if (match.fields.metadata ^ OF_PACKET_IN_REASON_NO_MATCH) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    in_port = match.fields.in_port;

    ppe_packet_t ppep;
    ppe_packet_init(&ppep, octets.data, octets.bytes);
    if (ppe_parse(&ppep) < 0) {
        AIM_LOG_RL_ERROR(&collect_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed.");
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    return collecta_receive_packet(&ppep, in_port);
}

/*
 * collect_init_socket
 *
 * Open a UDP Socket to send collect datagrams to the collector
 * and init socket params
 */
static indigo_error_t
collect_init_socket(collect_collector_entry_t *entry)
{
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
 * collect_collector_parse_key
 *
 * Parse key for collect_collector table entry from tlv list
 */
static indigo_error_t
collect_collector_parse_key(of_list_bsn_tlv_t *tlvs,
                            collect_collector_entry_key_t *key)
{
    of_object_t tlv;

    memset(key, 0, sizeof(*key));

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
 * collect_collector_parse_value
 *
 * Parse values for collect_collector table entry from tlv list
 */
static indigo_error_t
collect_collector_parse_value(of_list_bsn_tlv_t *tlvs,
                              collect_collector_entry_value_t *value)
{
    of_object_t tlv;

    memset(value, 0, sizeof(*value));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
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

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("expected end of value list, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

/*
 * collect_collector_add
 *
 * Add a new entry to collect_collector table
 */
static indigo_error_t
collect_collector_add(void *table_priv, of_list_bsn_tlv_t *key_tlvs,
                      of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    collect_collector_entry_key_t key;
    collect_collector_entry_value_t value;
    collect_collector_entry_t *entry;

    rv = collect_collector_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = collect_collector_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    rv = collect_init_socket(entry);
    if (rv < 0) {
        aim_free(entry);
        return rv;
    }

    /*
     * Add this entry to a list to be used later for sending a collect datagram out
     */
    list_push(&collect_collectors, &entry->links);

    AIM_LOG_TRACE("Add collector table entry, collector_ip: %{ipv4a} -> "
                  "collector_udp_dport: %u",
                  entry->key.collector_ip,
                  entry->value.collector_udp_dport);

    *entry_priv = entry;

    return INDIGO_ERROR_NONE;
}


/*
 * collect_collector_modify
 *
 * Modify a existing entry in collect_collector table
 */
static indigo_error_t
collect_collector_modify(void *table_priv, void *entry_priv,
                       of_list_bsn_tlv_t *key_tlvs, of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    collect_collector_entry_value_t value;
    collect_collector_entry_t *entry = entry_priv;

    rv = collect_collector_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    AIM_LOG_TRACE("Modify collector table entry, old collector_ip: %{ipv4a} ->"
                  "collector_udp_dport: %u",
                  entry->key.collector_ip,
                  entry->value.collector_udp_dport);

    AIM_LOG_TRACE("New, collector_ip: %{ipv4a} -> collector_udp_dport: %u",
                  entry->key.collector_ip,
                  value.collector_udp_dport);

    entry->value = value;

    return INDIGO_ERROR_NONE;
}

/*
 * collect_collector_delete
 *
 * Remove a entry from collect_collector table
 */
static indigo_error_t
collect_collector_delete(void *table_priv, void *entry_priv,
                       of_list_bsn_tlv_t *key_tlvs)
{
    collect_collector_entry_t *entry = entry_priv;

    AIM_LOG_TRACE("Delete collector table entry, collector_ip: %{ipv4a} -> "
                  "collector_udp_dport: %u",
                  entry->key.collector_ip,
                  entry->value.collector_udp_dport);

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
 * collect_collector_get_stats
 *
 * Return the stats related with a entry in collect_collector table
 */
static void
collect_collector_get_stats(void *table_priv, void *entry_priv,
                            of_list_bsn_tlv_t *key, of_list_bsn_tlv_t *stats)
{
    collect_collector_entry_t *entry = entry_priv;

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

static const indigo_core_gentable_ops_t collect_collector_ops = {
    .add = collect_collector_add,
    .modify = collect_collector_modify,
    .del = collect_collector_delete,
    .get_stats = collect_collector_get_stats,
};
