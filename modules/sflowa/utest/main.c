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

#include <sflowa/sflowa_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>
#include "sflowa_int.h"

static const indigo_core_gentable_ops_t *ops_collector;
static const indigo_core_gentable_ops_t *ops_sampler;
static void *table_priv_collector;
static void *table_priv_sampler;
static uint32_t current_port_no;
static uint32_t current_sampling_rate;
static ind_soc_config_t soc_cfg;

static const of_mac_addr_t zero_mac = { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static const sflow_collector_entry_t collector_entry_1 = {
    .key.collector_ip = 0xc0a86401, //192.168.100.1
    .value.vlan_id = 7,
    .value.vlan_pcp = 4,
    .value.agent_mac = { .addr = {0x55, 0x16, 0xc7, 0x01, 0x02, 0x03} },
    .value.agent_ip = 0xc0a80101, //192.168.1.1
    .value.agent_udp_sport = 50000,
    .value.collector_mac = { .addr = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f} },
    .value.collector_udp_dport = SFL_DEFAULT_COLLECTOR_PORT,
    .value.sub_agent_id = 10,
};
static sflow_collector_entry_t collector_entry_2 = {
    .key.collector_ip = 0x0a0a0505, //10.10.5.5
    .value.vlan_id = 2,
    .value.vlan_pcp = 4,
    .value.agent_mac = { .addr = {0x55, 0x16, 0xc7, 0x01, 0x02, 0x03} },
    .value.agent_ip = 0x0a0a6401, //10.10.100.1
    .value.agent_udp_sport = 45000,
    .value.collector_mac = { .addr = {0xca, 0xfe, 0xc0, 0xff, 0xee, 0x00} },
    .value.collector_udp_dport = SFL_DEFAULT_COLLECTOR_PORT,
    .value.sub_agent_id = 2,
};

#define PACKET_BUF_SIZE 78

uint8_t sample[PACKET_BUF_SIZE] = {0x00, 0x50, 0x56, 0xe0, 0x14, 0x49, 0x00, 0x0c, 0x29, 0x34, 0x0b, 0xde, 0x81, 0x00, 0x40, 0x07, 0x08, 0x00,
                             0x45, 0x00, 0x00, 0x3c, 0xd7, 0x43, 0x00, 0x00, 0x80, 0x01, 0x2b, 0x73, 0xc0, 0xa8, 0x9e, 0x8b, 0xae, 0x89, 0x2a, 0x4d,
                             0x08, 0x00, 0x2a, 0x5c, 0x02, 0x00, 0x21, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c,
                             0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};

uint8_t expected[172] = {0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xa8, 0x01, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x01,
                         0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x01,
                         0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4e};

static int fd;
static int port_desc_count = 0;
indigo_cxn_id_t cxn_id = 1;

void
hex_dump(uint8_t *ptr, int length)
{
    printf("received message: ");
    int i;
    for (i=0; i < length; i++) {
        printf("%02x ", ptr[i]);
        printf(" ");
    }
    printf("\n");
}

void
indigo_core_gentable_register(
    const of_table_name_t name,
    const indigo_core_gentable_ops_t *_ops,
    void *_table_priv,
    uint32_t max_size,
    uint32_t buckets_size,
    indigo_core_gentable_t **gentable)
{
    if (!strcmp(name, "sflow_collector")) {
        ops_collector = _ops;
        table_priv_collector = _table_priv; //NULL
    } else if (!strcmp(name, "sflow_sampler")) {
        ops_sampler = _ops;
        table_priv_sampler = _table_priv; //NULL
    }

    *gentable = (void *)1;
}

void
indigo_core_gentable_unregister(indigo_core_gentable_t *gentable)
{
    AIM_ASSERT(gentable == (void *)1);
}

indigo_error_t
indigo_core_packet_in_listener_register(indigo_core_packet_in_listener_f fn)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_core_packet_in_listener_unregister(indigo_core_packet_in_listener_f fn)
{
}

void
indigo_port_extended_stats_get(of_port_no_t port_no,
                               indigo_fi_port_stats_t *stats)
{
    static indigo_fi_port_stats_t port_stats = { 0 };

    /* Default to "counter not supported" */
    memset(stats, 0xff, sizeof(*stats));

    stats->rx_bytes = (port_stats.rx_bytes += 100);
    stats->rx_dropped = (port_stats.rx_dropped += 1);
    stats->rx_errors = (port_stats.rx_errors += 2);
    stats->tx_bytes = (port_stats.tx_bytes += 200);
    stats->tx_dropped = (port_stats.tx_dropped += 2);
    stats->tx_errors = (port_stats.tx_errors += 4);
    stats->rx_packets = (port_stats.rx_packets += 3);
    stats->tx_packets = (port_stats.tx_packets += 6);
    stats->rx_packets_unicast = (port_stats.rx_packets_unicast += 1);
    stats->rx_packets_broadcast = (port_stats.rx_packets_broadcast += 1);
    stats->rx_packets_multicast = (port_stats.rx_packets_multicast += 1);
    stats->tx_packets_unicast = (port_stats.tx_packets_unicast += 2);
    stats->tx_packets_broadcast = (port_stats.tx_packets_broadcast += 2);
    stats->tx_packets_multicast = (port_stats.tx_packets_multicast += 2);
}

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *of_packet_out)
{
    of_octets_t      of_octets;
    uint32_t         flow_sample = htonl(SFLFLOW_SAMPLE);
    uint32_t         counter_sample = htonl(SFLCOUNTERS_SAMPLE);
    static int       cs_count = 0;

    if (!of_packet_out) return INDIGO_ERROR_NONE;

    of_packet_out_data_get(of_packet_out, &of_octets);

    printf("Send a packet with %u bytes out\n", of_octets.bytes);

    if (!memcmp(&of_octets.data[74], &flow_sample, 4)) {
        printf("Flow Sample received\n");
        AIM_ASSERT(of_octets.bytes == 218, "mismatch in flow sample length, "
                   "expected 218, got (%u)", of_octets.bytes);
        AIM_ASSERT(!memcmp(&of_octets.data[46], expected, 172),
                   "mismatch in flow sample recv'd");
    } else if (!memcmp(&of_octets.data[74], &counter_sample, 4)) {
        printf("Counter Sample received\n");
        if (cs_count == 0) {
            AIM_ASSERT(of_octets.bytes == 190, "mismatch in counter sample "
                       "length, expected 190, got (%u)", of_octets.bytes);
            cs_count++;
        } else if (cs_count == 1) {
            AIM_ASSERT(of_octets.bytes == 306, "mismatch in counter sample "
                       "length, expected 306, got (%u)", of_octets.bytes);
        }
    }

#ifdef PRINT_PKT
    hex_dump(of_octets.data, of_octets.bytes);
#endif

    return INDIGO_ERROR_NONE;
}

static void
port_desc_set(of_port_desc_t *of_port_desc, uint32_t port_no)
{
    uint32_t config = 0;
    uint32_t state = 0;
    uint32_t curr = 0;

    of_port_desc_port_no_set(of_port_desc, port_no);

    /* Set Admin down for port 10 and oper down for port 20 */
    if (port_no == 10) {
        OF_PORT_CONFIG_FLAG_PORT_DOWN_SET(config, of_port_desc->version);
        curr = OF_PORT_FEATURE_FLAG_1GB_HD |
               OF_PORT_FEATURE_FLAG_COPPER_BY_VERSION(of_port_desc->version);
    } else if (port_no == 20) {
        OF_PORT_STATE_FLAG_LINK_DOWN_SET(state, of_port_desc->version);
        curr = OF_PORT_FEATURE_FLAG_40GB_FD;
    }

    of_port_desc_config_set(of_port_desc, config);
    of_port_desc_state_set(of_port_desc, state);
    of_port_desc_curr_set(of_port_desc, curr);
}

indigo_error_t
indigo_port_desc_stats_get(of_port_desc_stats_reply_t *port_desc_stats_reply)
{
    indigo_error_t result = INDIGO_ERROR_NONE;
    of_port_desc_t *of_port_desc = 0;
    of_list_port_desc_t *of_list_port_desc = 0;

    ++port_desc_count;

    if ((of_port_desc = of_port_desc_new(port_desc_stats_reply->version)) == 0) {
        result = INDIGO_ERROR_UNKNOWN;
        goto done;
    }

    if ((of_list_port_desc = of_list_port_desc_new(port_desc_stats_reply->version)) == 0) {
        result = INDIGO_ERROR_UNKNOWN;
        goto done;
    }

    port_desc_set(of_port_desc, 10);
    of_list_port_desc_append(of_list_port_desc, of_port_desc);

    port_desc_set(of_port_desc, 20);
    of_list_port_desc_append(of_list_port_desc, of_port_desc);

    if (of_port_desc_stats_reply_entries_set(port_desc_stats_reply, of_list_port_desc) < 0) {
        result = INDIGO_ERROR_UNKNOWN;
        goto done;
    }

done:
    if (of_list_port_desc) of_list_port_desc_delete(of_list_port_desc);
    if (of_port_desc) of_port_desc_delete(of_port_desc);

    return result;
}

indigo_error_t
indigo_core_port_status_listener_register(indigo_core_port_status_listener_f fn)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_core_port_status_listener_unregister(indigo_core_port_status_listener_f fn)
{
}

static sflow_collector_entry_t*
sflow_collectors_list_find(sflow_collector_entry_key_t key)
{
    list_head_t *list = sflow_collectors_list();
    list_links_t *cur;
    LIST_FOREACH(list, cur) {
        sflow_collector_entry_t *entry = container_of(cur, links,
                                                      sflow_collector_entry_t);
        if (entry->key.collector_ip == key.collector_ip) {
            return entry;
        }
    }

    return NULL;
}

static void
verify_entry_present(sflow_collector_entry_t test_entry)
{
    sflow_collector_entry_t *entry = sflow_collectors_list_find(test_entry.key);

    AIM_ASSERT(entry != NULL, "Collector entry with key: 0x%x missing from list",
               test_entry.key.collector_ip);
    AIM_ASSERT(!memcmp(&entry->value, &test_entry.value,
               sizeof(sflow_collector_entry_value_t)),
               "Mismatch in Collector entry value");
}

static void
verify_entry_absent(sflow_collector_entry_t test_entry)
{
    sflow_collector_entry_t *entry = sflow_collectors_list_find(test_entry.key);

    AIM_ASSERT(entry == NULL);
}

static of_list_bsn_tlv_t *
make_key_collector(uint32_t dst_ip)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    of_bsn_tlv_ipv4_dst_t *tlv = of_bsn_tlv_ipv4_dst_new(OF_VERSION_1_3);
    of_bsn_tlv_ipv4_dst_value_set(tlv, dst_ip);
    of_list_append(list, tlv);
    of_object_delete(tlv);
    return list;
}

static of_list_bsn_tlv_t *
make_value(uint16_t vlan, uint8_t vlan_pcp, of_mac_addr_t src_mac,
           uint32_t src_ip, uint16_t sport, of_mac_addr_t dst_mac,
           uint16_t dport, uint32_t sub_agent_id)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_3);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_pcp_t *tlv = of_bsn_tlv_vlan_pcp_new(OF_VERSION_1_3);
        of_bsn_tlv_vlan_pcp_value_set(tlv, vlan_pcp);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_eth_src_t *tlv = of_bsn_tlv_eth_src_new(OF_VERSION_1_3);
        of_bsn_tlv_eth_src_value_set(tlv, src_mac);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_src_t *tlv = of_bsn_tlv_ipv4_src_new(OF_VERSION_1_3);
        of_bsn_tlv_ipv4_src_value_set(tlv, src_ip);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_udp_src_t *tlv = of_bsn_tlv_udp_src_new(OF_VERSION_1_3);
        of_bsn_tlv_udp_src_value_set(tlv, sport);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_eth_dst_t *tlv = of_bsn_tlv_eth_dst_new(OF_VERSION_1_3);
        of_bsn_tlv_eth_dst_value_set(tlv, dst_mac);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_udp_dst_t *tlv = of_bsn_tlv_udp_dst_new(OF_VERSION_1_3);
        of_bsn_tlv_udp_dst_value_set(tlv, dport);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_sub_agent_id_t *tlv = of_bsn_tlv_sub_agent_id_new(OF_VERSION_1_3);
        of_bsn_tlv_sub_agent_id_value_set(tlv, sub_agent_id);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static void
test_sflow_collector_table(void)
{
    indigo_error_t rv;
    of_list_bsn_tlv_t *key, *value;
    void *entry_priv_1, *entry_priv_2;

    /*
     * Test add
     */
    key = make_key_collector(collector_entry_1.key.collector_ip);
    value = make_value(collector_entry_1.value.vlan_id,
                       collector_entry_1.value.vlan_pcp,
                       collector_entry_1.value.agent_mac,
                       collector_entry_1.value.agent_ip,
                       collector_entry_1.value.agent_udp_sport,
                       collector_entry_1.value.collector_mac,
                       collector_entry_1.value.collector_udp_dport,
                       collector_entry_1.value.sub_agent_id);

    AIM_ASSERT((rv = ops_collector->add(table_priv_collector, key, value,
               &entry_priv_1)) == INDIGO_ERROR_NONE,
               "Error in collector table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    /*
     * Verify entry got added to collectors list
     */
    verify_entry_present(collector_entry_1);
    verify_entry_absent(collector_entry_2);

    key = make_key_collector(collector_entry_2.key.collector_ip);
    value = make_value(collector_entry_2.value.vlan_id,
                       collector_entry_2.value.vlan_pcp,
                       collector_entry_2.value.agent_mac,
                       collector_entry_2.value.agent_ip,
                       collector_entry_2.value.agent_udp_sport,
                       collector_entry_2.value.collector_mac,
                       collector_entry_2.value.collector_udp_dport,
                       collector_entry_2.value.sub_agent_id);

    AIM_ASSERT((rv = ops_collector->add(table_priv_collector, key, value,
               &entry_priv_2)) == INDIGO_ERROR_NONE,
               "Error in collector table add: %s\n", indigo_strerror(rv));

    of_object_delete(value);

    verify_entry_present(collector_entry_1);
    verify_entry_present(collector_entry_2);

    /*
     * Test modify
     */
    collector_entry_2.value.vlan_id = 15;
    collector_entry_2.value.agent_ip = 0x0a0a6464; //10.10.100.100
    value = make_value(collector_entry_2.value.vlan_id,
                       collector_entry_2.value.vlan_pcp,
                       collector_entry_2.value.agent_mac,
                       collector_entry_2.value.agent_ip,
                       collector_entry_2.value.agent_udp_sport,
                       collector_entry_2.value.collector_mac,
                       collector_entry_2.value.collector_udp_dport,
                       collector_entry_2.value.sub_agent_id);
    AIM_ASSERT((rv = ops_collector->modify(table_priv_sampler, entry_priv_2, key,
               value)) == INDIGO_ERROR_NONE,
               "Error in collector table modify: %s\n", indigo_strerror(rv));

    verify_entry_present(collector_entry_1);
    verify_entry_present(collector_entry_2);

    of_object_delete(value);

    /*
     * Test delete
     */
    AIM_ASSERT((rv = ops_collector->del(table_priv_collector, entry_priv_2, key))
               == INDIGO_ERROR_NONE,
               "Error in collector table delete: %s\n", indigo_strerror(rv));

    of_object_delete(key);

    verify_entry_present(collector_entry_1);
    verify_entry_absent(collector_entry_2);

    key = make_key_collector(collector_entry_1.key.collector_ip);
    AIM_ASSERT((rv = ops_collector->del(table_priv_collector, entry_priv_1, key))
               == INDIGO_ERROR_NONE,
               "Error in collector table delete: %s\n", indigo_strerror(rv));

    of_object_delete(key);

    verify_entry_absent(collector_entry_1);
    verify_entry_absent(collector_entry_2);
}

static of_list_bsn_tlv_t *
make_key_sampler(of_port_no_t port_no)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    of_bsn_tlv_port_t *tlv = of_bsn_tlv_port_new(OF_VERSION_1_3);
    of_bsn_tlv_port_value_set(tlv, port_no);
    of_list_append(list, tlv);
    of_object_delete(tlv);
    current_port_no = port_no;
    return list;
}

static of_list_bsn_tlv_t *
make_value_sampler(uint32_t sampling_rate, uint32_t header_size,
                   uint32_t polling_interval)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    {
        of_bsn_tlv_sampling_rate_t *tlv = of_bsn_tlv_sampling_rate_new(OF_VERSION_1_3);
        of_bsn_tlv_sampling_rate_value_set(tlv, sampling_rate);
        of_list_append(list, tlv);
        of_object_delete(tlv);
        current_sampling_rate = sampling_rate;
    }
    {
        of_bsn_tlv_header_size_t *tlv = of_bsn_tlv_header_size_new(OF_VERSION_1_3);
        of_bsn_tlv_header_size_value_set(tlv, header_size);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_interval_t *tlv = of_bsn_tlv_interval_new(OF_VERSION_1_3);
        of_bsn_tlv_interval_value_set(tlv, polling_interval);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static indigo_error_t
handler(uint32_t port_no, uint32_t sampling_rate, indigo_cxn_id_t test_cxn_id)
{
    AIM_ASSERT(port_no == current_port_no, "Mismatch in port");
    AIM_ASSERT(sampling_rate == current_sampling_rate, "Mismatch in sampling rate");
    AIM_ASSERT(test_cxn_id == cxn_id, "Mismatch in cxn_id");

    return INDIGO_ERROR_NONE;
}

static void
test_sflow_sampler_table(void)
{
    of_list_bsn_tlv_t *key, *value;
    indigo_error_t rv;
    void *entry_priv_1, *entry_priv_2;

    /*
     *  Register a sampling_rate handler fn
     */
    sflowa_sampling_rate_handler_register(handler);

    /*
     * Test add
     */
    key = make_key_sampler(57);
    value = make_value_sampler(512, 128, 20000);

    AIM_ASSERT((rv = ops_sampler->add2(cxn_id, table_priv_sampler, key, value,
               &entry_priv_1)) == INDIGO_ERROR_NONE,
               "Error in sampler table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    key = make_key_sampler(92);
    value = make_value_sampler(1024, 64, 0);

    AIM_ASSERT((rv = ops_sampler->add2(cxn_id, table_priv_sampler, key, value,
               &entry_priv_2)) == INDIGO_ERROR_NONE,
               "Error in sampler table add: %s\n", indigo_strerror(rv));

    of_object_delete(value);

    /*
     * Test modify
     */
    value = make_value_sampler(2048, 64, 30982);
    AIM_ASSERT((rv = ops_sampler->modify2(cxn_id, table_priv_sampler,
               entry_priv_2, key, value)) == INDIGO_ERROR_NONE,
               "Error in sampler table modify: %s\n", indigo_strerror(rv));

    of_object_delete(value);

    value = make_value_sampler(2048, 64, 0);
    AIM_ASSERT((rv = ops_sampler->modify2(cxn_id, table_priv_sampler,
               entry_priv_2, key, value)) == INDIGO_ERROR_NONE,
               "Error in sampler table modify: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    /*
     * Test delete
     */
    current_sampling_rate = 0;
    AIM_ASSERT((rv = ops_sampler->del2(cxn_id, table_priv_sampler,
               entry_priv_2, NULL)) == INDIGO_ERROR_NONE,
               "Error in sampler table delete: %s\n", indigo_strerror(rv));

    current_port_no = 57;
    AIM_ASSERT((rv = ops_sampler->del2(cxn_id, table_priv_sampler,
               entry_priv_1, NULL)) == INDIGO_ERROR_NONE,
               "Error in sampler table delete: %s\n", indigo_strerror(rv));

    sflowa_sampling_rate_handler_unregister(handler);
}

static void
sflow_create_send_packet_in(of_octets_t *of_octets, of_port_no_t in_port)
{
    of_packet_in_t *of_packet_in;
    of_match_t     match;

    memset(&match, 0, sizeof(of_match_t));

    if (!of_octets) return;

    if ((of_packet_in = of_packet_in_new(OF_VERSION_1_3)) == NULL) {
        return;
    }

    of_packet_in_total_len_set(of_packet_in, of_octets->bytes);
    match.version = OF_VERSION_1_3;
    match.fields.in_port = in_port;
    OF_MATCH_MASK_IN_PORT_EXACT_SET(&match);
    match.fields.metadata |= OFP_BSN_PKTIN_FLAG_SFLOW;
    OF_MATCH_MASK_METADATA_EXACT_SET(&match);
    if ((of_packet_in_match_set(of_packet_in, &match)) != OF_ERROR_NONE) {
        printf("Failed to write match to packet-in message\n");
        of_packet_in_delete(of_packet_in);
        return;
    }

    if ((of_packet_in_data_set(of_packet_in, of_octets)) != OF_ERROR_NONE) {
        printf("Failed to write packet data to packet-in message\n");
        of_packet_in_delete(of_packet_in);
        return;
    }

    AIM_ASSERT(sflowa_packet_in_handler(of_packet_in) ==
               INDIGO_CORE_LISTENER_RESULT_DROP, "Listener passed packet-in");

    of_packet_in_delete(of_packet_in);
}

static void
init_socket(void)
{
    struct sockaddr_in addr;

    /* create a UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("create socket failed\n");
        return;
    }

    /* set timeout for recv operation */
    struct timeval tv;

    tv.tv_sec = 10;  /* 10 sec timeout */
    tv.tv_usec = 0;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    /* bind the socket to any valid IP address and a specific port */
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(SFL_DEFAULT_COLLECTOR_PORT);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("bind failed");
        return;
    }
}

static void
close_socket(void)
{
    close(fd);
}

static void
receive_socket(int type)
{
    uint8_t buf[1500];

    /* receive data and print what we received */
    printf("waiting on fd for %s\n", (type == SFLFLOW_SAMPLE)?
           "flow sample": "counter sample");
    int recvlen = recvfrom(fd, buf, 1500, 0, 0, 0);
    AIM_ASSERT(recvlen, "received 0 bytes from fd");
    AIM_ASSERT(recvlen != -1, "recvfrom() failed with %s", strerror(errno));
    if (type == SFLFLOW_SAMPLE) {
        AIM_ASSERT(!memcmp(buf, expected, recvlen), "mismatch in recv output");
    } else if (type == SFLCOUNTERS_SAMPLE) {
        if (port_desc_count == 1) {
            AIM_ASSERT(recvlen == 144, "mismatch in recvlen, expected 144, "
                       "got (%d)", recvlen);
        } else if (port_desc_count == 2) {
            AIM_ASSERT(recvlen == 260, "mismatch in recvlen, expected 260, "
                       "got (%d)", recvlen);
        }
    }

#ifdef PRINT_PKT
    printf("received %d bytes\n", recvlen);
    if (recvlen > 0) {
        hex_dump(buf, recvlen);
    }
#endif
}

static void
test_sampled_packet_in(void)
{
    of_list_bsn_tlv_t *key, *value;
    indigo_error_t rv;
    void *sampler_entry_priv_1, *sampler_entry_priv_2;
    void *collector_entry_priv_1, *collector_entry_priv_2;
    of_octets_t octets;

    memcpy(&expected[92], sample, PACKET_BUF_SIZE);

    sflowa_sampling_rate_handler_register(handler);

    /* init socket */
    init_socket();

    /* Add sampler_table entries */
    key = make_key_sampler(10);
    value = make_value_sampler(512, 256, 2000);

    AIM_ASSERT((rv = ops_sampler->add2(cxn_id, table_priv_sampler, key, value,
               &sampler_entry_priv_1)) == INDIGO_ERROR_NONE,
               "Error in sampler table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    key = make_key_sampler(20);
    value = make_value_sampler(1024, 128, 1000);

    AIM_ASSERT((rv = ops_sampler->add2(cxn_id, table_priv_sampler, key, value,
               &sampler_entry_priv_2)) == INDIGO_ERROR_NONE,
               "Error in sampler table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    /* Add bt collector_table entry */
    key = make_key_collector(0x7F000001); //127.0.0.1
    value = make_value(0, 0, zero_mac, 0xc0a80101, 0, zero_mac,
                       SFL_DEFAULT_COLLECTOR_PORT, 10);

    AIM_ASSERT((rv = ops_collector->add(table_priv_collector, key, value,
               &collector_entry_priv_1)) == INDIGO_ERROR_NONE,
               "Error in collector table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    /* Add bcf collector_table entry */
    key = make_key_collector(collector_entry_1.key.collector_ip);
    value = make_value(collector_entry_1.value.vlan_id,
                       collector_entry_1.value.vlan_pcp,
                       collector_entry_1.value.agent_mac,
                       collector_entry_1.value.agent_ip,
                       collector_entry_1.value.agent_udp_sport,
                       collector_entry_1.value.collector_mac,
                       collector_entry_1.value.collector_udp_dport,
                       collector_entry_1.value.sub_agent_id);

    AIM_ASSERT((rv = ops_collector->add(table_priv_collector, key, value,
               &collector_entry_priv_2)) == INDIGO_ERROR_NONE,
               "Error in collector table add: %s\n", indigo_strerror(rv));

    of_object_delete(key);
    of_object_delete(value);

    /* Create and send a sflow sampled packet_in */
    octets.data = sample;
    octets.bytes = PACKET_BUF_SIZE;
    sflow_create_send_packet_in(&octets, 10);

    /*
     * trigger a tick for the agent to:
     *     (a) send flow sample
     *     (b) get port counters for port 20
     *     (c) refresh port_feature
     */
    sflow_timer(NULL);

    /* receive flow sample from agent */
    receive_socket(SFLFLOW_SAMPLE);

    /* check that port_features are refreshed */
    AIM_ASSERT(port_desc_count == 1, "port_desc_count (%d) should be 1",
               port_desc_count);

    /*
     * trigger another tick for the agent to:
     *     (a) send counter sample for port 20
     *     (b) get port counters for port 10, 20
     *     (c) do not refresh port_feature
     */
    sflow_timer(NULL);

    /* check that port_features are not refreshed */
    AIM_ASSERT(port_desc_count == 1, "port_desc_count (%d) should be 1",
               port_desc_count);

    /* receive counter sample from agent with port 20 counters */
    receive_socket(SFLCOUNTERS_SAMPLE);

    /* trigger port_status change */
    sflowa_port_status_handler(NULL);

    /*
     * trigger another tick for the agent to:
     *     (a) send counter sample for port 10, 20
     *     (b) get counter sample for port 20
     *     (c) refresh port_feature
     */
    sflow_timer(NULL);

    /* check that port_features are refreshed */
    AIM_ASSERT(port_desc_count == 2, "port_desc_count (%d) should be 2",
               port_desc_count);

    /* receive counter sample from agent with port 10 and 20 counters */
    receive_socket(SFLCOUNTERS_SAMPLE);

    /* Close socket */
    close_socket();

    /* Delete sampler_table entries */
    current_sampling_rate = 0;
    AIM_ASSERT((rv = ops_sampler->del2(cxn_id, table_priv_sampler,
               sampler_entry_priv_2, NULL)) == INDIGO_ERROR_NONE,
               "Error in sampler table delete: %s\n", indigo_strerror(rv));

    current_port_no = 10;
    AIM_ASSERT((rv = ops_sampler->del2(cxn_id, table_priv_sampler,
               sampler_entry_priv_1, NULL)) == INDIGO_ERROR_NONE,
               "Error in sampler table delete: %s\n", indigo_strerror(rv));

    /* Delete collector_table entries */
    AIM_ASSERT((rv = ops_collector->del(table_priv_collector,
               collector_entry_priv_1, NULL)) == INDIGO_ERROR_NONE,
               "Error in collector table delete: %s\n", indigo_strerror(rv));

    AIM_ASSERT((rv = ops_collector->del(table_priv_collector,
               collector_entry_priv_2, NULL)) == INDIGO_ERROR_NONE,
               "Error in collector table delete: %s\n", indigo_strerror(rv));

    sflowa_sampling_rate_handler_unregister(handler);
}

int aim_main(int argc, char* argv[])
{
    ind_soc_init(&soc_cfg);
    sflowa_init();

    test_sflow_collector_table();
    test_sflow_sampler_table();
    test_sampled_packet_in();

    sflowa_finish();
    ind_soc_finish();
    return 0;
}

