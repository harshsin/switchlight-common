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
 * Implementation of ICMP Agent Handlers.
 *
 * This file contains the api's for initializing and handling incoming/outgoing
 * messages to/from icmp agent.
 */

#include "icmpa_int.h"

bool icmp_initialized = false;
aim_ratelimiter_t icmp_pktin_log_limiter;

icmpa_packet_counter_t pkt_counters;
icmpa_typecode_packet_counter_t port_pkt_counters[ICMPA_CONFIG_OF_PORTS_MAX+1];

static indigo_core_gentable_t *icmp_table;
static const indigo_core_gentable_ops_t icmp_ops;

#define TEMPLATE_NAME icmp_entries_hashtable
#define TEMPLATE_OBJ_TYPE icmp_entry_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

static bighash_table_t *icmp_entries;

/*
 * Iterate on entries of icmp_table.
 * bighash_iter_t _iter - Big Hash iterator,
 * icmp_entry_t   *_entry - Pointer to icmp_table entry.
 */
#define ICMP_TABLE_ITERATE(_iter, _entry)        \
    for (_entry = (icmp_entry_t *)bighash_iter_start(icmp_entries, &_iter); \
         _entry != NULL;                         \
         _entry = (icmp_entry_t *)bighash_iter_next(&_iter))

/*
 * is_ephemeral
 *
 * Returns true if a given port is ephemeral, else returns false
 */
static bool
is_ephemeral(uint32_t port)
{
    return (port >= 32768 && port <= 61000);
}

/*
 * icmp_packet_in_handler
 *
 * API for handling incoming packets
 */
indigo_core_listener_result_t
icmpa_packet_in_handler (of_packet_in_t *packet_in)
{
    of_octets_t                octets;
    of_port_no_t               port_no;
    of_match_t                 match;
    ppe_packet_t               ppep;
    uint32_t                   type, code;

    debug_counter_inc(&pkt_counters.icmp_total_in_packets);
    if (!packet_in) return INDIGO_CORE_LISTENER_RESULT_PASS;

    of_packet_in_data_get(packet_in, &octets);

    /*
     * Identify the recv port
     */
    if (packet_in->version <= OF_VERSION_1_1) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    } else {
        if (of_packet_in_match_get(packet_in, &match) < 0) {
            AIM_LOG_INTERNAL("ICMPA: match get failed");
            debug_counter_inc(&pkt_counters.icmp_internal_errors);
            return INDIGO_CORE_LISTENER_RESULT_PASS;
        }
        port_no = match.fields.in_port;
    }

    if (port_no == OF_PORT_DEST_CONTROLLER) {
        debug_counter_inc(&pkt_counters.icmp_total_passed_packets);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Check the packet-in reasons in metadata
     *
     * Icmp agent should not consume packets coming in due to L2 Src miss
     * and Station Move.
     */
    if ((match.fields.metadata & OFP_BSN_PKTIN_FLAG_STATION_MOVE) ||
        (match.fields.metadata & OFP_BSN_PKTIN_FLAG_NEW_HOST)) {
        debug_counter_inc(&pkt_counters.icmp_total_passed_packets);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_packet_init(&ppep, octets.data, octets.bytes);
    if (ppe_parse(&ppep) < 0) {
        AIM_LOG_RL_ERROR(&icmp_pktin_log_limiter, os_time_monotonic(),
                         "ICMPA: Packet_in parsing failed.");
        debug_counter_inc(&pkt_counters.icmp_internal_errors);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /*
     * Identify if the reason is L3 Destination miss.
     *
     * L3 destination miss needs to be processed before ICMP Echo
     * requests since echo requests could come to cpu with
     * L3 destination miss reason and in that case we should send
     * an ICMP Destination Unreachable message to the sender
     * instead of replying back to the ICMP Echo request.
     */
    if (match.fields.metadata & OFP_BSN_PKTIN_FLAG_L3_MISS) {
        type = ICMP_DEST_UNREACHABLE;
        code = 0;
        return icmpa_send(&ppep, port_no, type, code);
    }

    /*
     * Identify if this is an Echo Request, destined to one of VRouter
     */
    if (ppe_header_get(&ppep, PPE_HEADER_ICMP)) {
        if (icmpa_reply(&ppep, port_no) == INDIGO_CORE_LISTENER_RESULT_DROP) {
            return INDIGO_CORE_LISTENER_RESULT_DROP;
        }
    }

    /*
     * To handle traceroute, we need to check for
     * a) UDP Packet
     * b) dest IP is Vrouter IP
     * c) UDP src and dest ports are ephemeral
     */
    if (ppe_header_get(&ppep, PPE_HEADER_UDP) &&
        ppe_header_get(&ppep, PPE_HEADER_IP4)) {
        uint32_t dest_ip, src_port, dest_port;
        ppe_field_get(&ppep, PPE_FIELD_IP4_DST_ADDR, &dest_ip);
        ppe_field_get(&ppep, PPE_FIELD_UDP_SRC_PORT, &src_port);
        ppe_field_get(&ppep, PPE_FIELD_UDP_DST_PORT, &dest_port);

        if (router_ip_check(dest_ip) && is_ephemeral(src_port) &&
            is_ephemeral(dest_port)) {
            type = ICMP_DEST_UNREACHABLE;
            code = 3;
            return icmpa_send(&ppep, port_no, type, code);
        }
    }

    /*
     * Identify if the reason is TTL Expired.
     */
    if (match.fields.metadata & OFP_BSN_PKTIN_FLAG_TTL_EXPIRED) {
        type = ICMP_TIME_EXCEEDED;
        code = 0;
        return icmpa_send(&ppep, port_no, type, code);
    }

    return INDIGO_CORE_LISTENER_RESULT_PASS;
}

/*
 * icmpa_is_initialized
 *
 * true = ICMP Initialized
 * false = ICMP Uninitialized
 */
bool
icmpa_is_initialized (void)
{
    return icmp_initialized;
}

/*
 * icmpa_init
 *
 * API to init the ICMP Agent
 * This should only be done once at the beginning.
 */
indigo_error_t
icmpa_init (void)
{
    if (icmpa_is_initialized()) return INDIGO_ERROR_NONE;

    AIM_LOG_VERBOSE("init");

    indigo_core_gentable_register("icmp", &icmp_ops, NULL,
                                  SLSHARED_CONFIG_MAX_VLAN+1, 256, &icmp_table);
    icmp_entries = bighash_table_create(SLSHARED_CONFIG_MAX_VLAN+1);

    /*
     * Register system debug counters
     */
    debug_counter_register(&pkt_counters.icmp_total_in_packets,
                           "icmpa.icmp_total_in_packets",
                           "Packet-ins recv'd by icmpa");
    debug_counter_register(&pkt_counters.icmp_total_out_packets,
                           "icmpa.icmp_total_out_packets",
                           "Icmp packets sent by lacpa");
    debug_counter_register(&pkt_counters.icmp_total_passed_packets,
                           "icmpa.icmp_total_passed_packets",
                            "Packet-ins passed by icmpa");
    debug_counter_register(&pkt_counters.icmp_internal_errors,
                           "icmpa.icmp_internal_errors",
                           "Internal errors in icmpa");

    ICMPA_MEMSET(&port_pkt_counters[0], 0,
       sizeof(icmpa_typecode_packet_counter_t) * (ICMPA_CONFIG_OF_PORTS_MAX+1));
    aim_ratelimiter_init(&icmp_pktin_log_limiter, 1000*1000, 5, NULL);

#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    /*
     * Register listerner for packet_in
     */
    if (indigo_core_packet_in_listener_register(
        (indigo_core_packet_in_listener_f) icmpa_packet_in_handler) < 0) {
        AIM_LOG_FATAL("Failed to register for packet_in in ICMPA module");
        return INDIGO_ERROR_INIT;
    }
#endif

    icmp_initialized = true;
    return INDIGO_ERROR_NONE;
}

/*
 * icmpa_finish
 *
 * API to deinit the ICMP Agent
 * This will result in ICMP Agent being diabled in the system.
 */
void
icmpa_finish (void)
{
    if (!icmpa_is_initialized()) return;

    indigo_core_gentable_unregister(icmp_table);
    bighash_table_destroy(icmp_entries, NULL);

    /*
     * Unregister system debug counters
     */
    debug_counter_unregister(&pkt_counters.icmp_total_in_packets);
    debug_counter_unregister(&pkt_counters.icmp_total_out_packets);
    debug_counter_unregister(&pkt_counters.icmp_total_passed_packets);
    debug_counter_unregister(&pkt_counters.icmp_internal_errors);

    ICMPA_MEMSET(&port_pkt_counters[0], 0,
       sizeof(icmpa_typecode_packet_counter_t) * (ICMPA_CONFIG_OF_PORTS_MAX+1));

#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    /*
     * Unregister listerner for packet_in
     */
    indigo_core_packet_in_listener_unregister(icmpa_packet_in_handler);
#endif

    icmp_initialized = false;
}

/*
 * icmpa_table_entries_print
 *
 * Print the entries in icmp_table
 */
void
icmpa_table_entries_print (ucli_context_t* uc)
{
    icmp_entry_t *entry;
    bighash_iter_t iter;

    if (!bighash_entry_count(icmp_entries)) return;

    ucli_printf(uc, "VLAN\t VROUTER IP --> OUT VLAN\t NETMASK\n");

    ICMP_TABLE_ITERATE(iter, entry) {
        ucli_printf(uc, "%u\t %{ipv4a} --> %u\t\t %{ipv4a}\n",
                    entry->key.vlan_id, entry->key.ipv4,
                    entry->value.vlan_id, entry->value.ipv4_netmask);
    }
}

/*
 * icmpa_router_ip_lookup
 *
 * Given host ip, determine the VRouter ip associated with this host
 *
 * Return true if found; else false
 */
bool
icmpa_router_ip_lookup (uint32_t dest_ip, uint32_t *router_ip)
{
    icmp_entry_t *entry;
    bighash_iter_t iter;

    AIM_ASSERT(router_ip, "NULL router_ip");

    ICMP_TABLE_ITERATE(iter, entry) {
        if ((entry->key.ipv4 & entry->value.ipv4_netmask) ==
            (dest_ip & entry->value.ipv4_netmask)) {
            AIM_LOG_TRACE("Found router ip:%{ipv4a} for dest_ip:%{ipv4a}",
                          entry->key.ipv4, dest_ip);
            *router_ip = entry->key.ipv4;
            return true;
        }
    }

    return false;
}

/*
 * icmpa_lookup
 *
 * Hashtable lookup
 * key = System_Vlan, VRouter IP
 * value = Vlan (vlan to send echo replies)
 *
 * Return a pointer to icmp table entry if found; else NULL
 */
icmp_entry_t *
icmpa_lookup (uint16_t vlan_id, uint32_t ipv4)
{
    icmp_entry_key_t key;
    memset(&key, 0, sizeof(key));
    key.vlan_id = vlan_id;
    key.ipv4 = ipv4;

    return icmp_entries_hashtable_first(icmp_entries, &key);
}

/*
 * icmp_parse_key
 *
 * Parse key for icmp table entry from tlv list
 */
static indigo_error_t
icmp_parse_key (of_list_bsn_tlv_t *tlvs, icmp_entry_key_t *key)
{
    of_object_t tlv;

    ICMPA_MEMSET(key, 0, sizeof(*key));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty key list");
        return INDIGO_ERROR_PARAM;
    }

    /*
     * Vlan id
     */
    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &key->vlan_id);
    } else {
        AIM_LOG_ERROR("expected vlan key TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (key->vlan_id > SLSHARED_CONFIG_MAX_VLAN) {
        AIM_LOG_ERROR("%s: VLAN out of range (%u)", __FUNCTION__, key->vlan_id);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /*
     * Virtual Router IP
     */
    if (tlv.object_id == OF_BSN_TLV_IPV4) {
        of_bsn_tlv_ipv4_value_get(&tlv, &key->ipv4);
    } else {
        AIM_LOG_ERROR("expected ipv4 key TLV, instead got %s",
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
 * icmp_parse_value
 *
 * Parse values for icmp table entry from tlv list
 */
static indigo_error_t
icmp_parse_value (of_list_bsn_tlv_t *tlvs, icmp_entry_value_t *value)
{
    of_object_t tlv;

    ICMPA_MEMSET(value, 0, sizeof(*value));

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("empty value list");
        return INDIGO_ERROR_PARAM;
    }

    /*
     * Vlan id
     */
    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &value->vlan_id);
    } else {
        AIM_LOG_ERROR("expected vlan value TLV, instead got %s",
                      of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (value->vlan_id > SLSHARED_CONFIG_MAX_VLAN) {
        AIM_LOG_ERROR("%s: VLAN out of range (%u)",
                      __FUNCTION__, value->vlan_id);
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("unexpected end of value list");
        return INDIGO_ERROR_PARAM;
    }

    /*
     * Ipv4 netmask
     */
    if (tlv.object_id == OF_BSN_TLV_IPV4_NETMASK) {
        of_bsn_tlv_ipv4_netmask_value_get(&tlv, &value->ipv4_netmask);
    } else {
        AIM_LOG_ERROR("expected ipv4_netmask value TLV, instead got %s",
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
 * icmp_add
 *
 * Add a new entry to icmp table
 */
static indigo_error_t
icmp_add (void *table_priv, of_list_bsn_tlv_t *key_tlvs,
          of_list_bsn_tlv_t *value_tlvs, void **entry_priv)
{
    indigo_error_t rv;
    icmp_entry_key_t key;
    icmp_entry_value_t value;

    rv = icmp_parse_key(key_tlvs, &key);
    if (rv < 0) {
        return rv;
    }

    rv = icmp_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    icmp_entry_t *entry = aim_zmalloc(sizeof(icmp_entry_t));
    entry->key = key;
    entry->value = value;

    AIM_LOG_TRACE("Add icmp table entry, vlan_id:%u, ip:%{ipv4a} -> vlan_id:%u"
                  ", ip_netmask:%{ipv4a}", entry->key.vlan_id, entry->key.ipv4,
                  entry->value.vlan_id, entry->value.ipv4_netmask);

    *entry_priv = entry;
    icmp_entries_hashtable_insert(icmp_entries, entry);

    return INDIGO_ERROR_NONE;
}

/*
 * icmp_modify
 *
 * Modify a existing entry in icmp table
 */
static indigo_error_t
icmp_modify (void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs,
             of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    icmp_entry_value_t value;
    icmp_entry_t *entry = entry_priv;

    rv = icmp_parse_value(value_tlvs, &value);
    if (rv < 0) {
        return rv;
    }

    AIM_LOG_TRACE("Modify icmp table entry, vlan_id:%u, ip:%{ipv4a} -> from "
                  "vlan_id:%u, ip_netmask:%{ipv4a} to vlan_id:%u, "
                  "ip_netmask:%{ipv4a}", entry->key.vlan_id, entry->key.ipv4,
                  entry->value.vlan_id, entry->value.ipv4_netmask,
                  value.vlan_id, value.ipv4_netmask);

    entry->value = value;

    return INDIGO_ERROR_NONE;
}

/*
 * icmp_delete
 *
 * Remove a entry from icmp table
 */
static indigo_error_t
icmp_delete (void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key_tlvs)
{
    icmp_entry_t *entry = entry_priv;

    AIM_LOG_TRACE("Delete icmp table entry, vlan_id:%u, ip:%{ipv4a} -> "
                  "vlan_id:%u, ip_netmask:%{ipv4a}",
                  entry->key.vlan_id, entry->key.ipv4, entry->value.vlan_id,
                  entry->value.ipv4_netmask);

    bighash_remove(icmp_entries, &entry->hash_entry);
    aim_free(entry);
    return INDIGO_ERROR_NONE;
}

static void
icmp_get_stats (void *table_priv, void *entry_priv, of_list_bsn_tlv_t *key,
                of_list_bsn_tlv_t *stats)
{
    /*
     * No stats
     */
}

static const indigo_core_gentable_ops_t icmp_ops = {
    .add = icmp_add,
    .modify = icmp_modify,
    .del = icmp_delete,
    .get_stats = icmp_get_stats,
};
