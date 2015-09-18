/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * IGMP agent implementation.
 */

/*
 * The IGMPA offload agent provides the following functionality:
 * - Handling IGMP general queries from the mrouter
 * - Sending IGMP reports to the mrouter 
 *   for all interested IPv4 multicast addresses
 * - Sending IGMP general queries to the hosts
 * - Handling IGMP reports from all hosts interested in
 *   given multicast addresses
 *
 * Receipt of IGMP general queries from mrouter and reports from hosts
 * are expected.
 * Not receiving these packets in time will cause a timer to expire and
 * a notification to be sent to the controller.
 * An unexpected query or report will be sent to the controller.
 *
 * Sending IGMP general queries to the hosts and IGMP reports to the mrouter
 * is triggered periodically.
 */

#include <AIM/aim.h>
#include <OS/os.h>
#include <indigo/of_state_manager.h>
#include <PPE/ppe.h>
#include <debug_counter/debug_counter.h>

#include "timeout_table.h"
#include "rx_port_group_table.h"
#include "tx_port_group_table.h"
#include "report_expect_table.h"
#include "report_tx_table.h"
#include "gq_expect_table.h"
#include "gq_tx_table.h"

#include "igmpa_int.h"
#include "igmpa_log.h"


/* logging rate limiter */
static aim_ratelimiter_t igmpa_pktin_log_limiter;


/* debug counters */
static debug_counter_t pktin_count;
static debug_counter_t pktin_parse_failure;
static debug_counter_t pktin_bad_checksum;
static debug_counter_t gq_rx_count;
static debug_counter_t gsq_rx_count;
static debug_counter_t report_rx_count;
static debug_counter_t leave_rx_count;
static debug_counter_t unknown_rx_count;


/*----------*/
/* packet handlers */

static indigo_core_listener_result_t
handle_general_query(ppe_packet_t *ppep, 
                     of_port_no_t port_no, uint16_t vlan_vid)
{
    rx_port_group_entry_t *rpg_entry;
    gq_expect_entry_t *gq_entry;

    /* get rx port group name */
    rpg_entry = igmpa_rx_port_group_lookup(port_no);
    if (rpg_entry == NULL) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* get general query expectation */
    gq_entry = igmpa_gq_expect_lookup(rpg_entry->value.name, vlan_vid);
    if (gq_entry) {
        uint64_t now = INDIGO_CURRENT_TIME;
        /* expected packet: reset timer, do not send to controller */
        gq_entry->time_last_hit = now;
        gq_entry->rx_packets++;
        /* reschedule timer */
        AIM_LOG_TRACE("reschedule gq timer");
        igmpa_gq_expect_reschedule(gq_entry, now + igmpa_gq_expect_timeout);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    } else {
        /* not found, send to controller */
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}

static indigo_core_listener_result_t
handle_report(ppe_packet_t *ppep, 
              of_port_no_t port_no, uint16_t vlan_vid)
{
    rx_port_group_entry_t *rpg_entry;
    report_expect_entry_t *report_entry;
    uint32_t ipv4;

    ppe_field_get(ppep, PPE_FIELD_IGMP_GROUP_ADDRESS, &ipv4);

    /* get rx port group name */
    rpg_entry = igmpa_rx_port_group_lookup(port_no);
    if (rpg_entry == NULL) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* get report expectation */
    report_entry = igmpa_report_expect_lookup(rpg_entry->value.name,
                                              vlan_vid, ipv4);
    if (report_entry) {
        uint64_t now = INDIGO_CURRENT_TIME;
        /* expected packet: reset timer, do not send to controller */
        report_entry->time_last_hit = now;
        report_entry->rx_packets++;
        /* reschedule timer */
        AIM_LOG_TRACE("reschedule report timer");
        igmpa_report_expect_reschedule(report_entry,
                                       now + igmpa_report_expect_timeout);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    } else {
        /* not found, send to controller */
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* FIXME store v1/v2 received? */
}

/* top-level packet_in handler */
static indigo_core_listener_result_t
handle_pktin(of_packet_in_t *packet_in)
{
    indigo_core_listener_result_t rc;
    of_match_t match;
    of_port_no_t port_no;
    of_octets_t octets;
    ppe_packet_t ppep;
    uint32_t vlan_vid;
    uint32_t l3_len;
    uint32_t l3hdr_words;
    uint32_t l4_len;
    uint8_t *payload;
    uint16_t checksum;
    uint32_t msg_type;
    uint32_t ipv4_addr;

    debug_counter_inc(&pktin_count);

    if (packet_in->version <= OF_VERSION_1_1) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    AIM_TRUE_OR_DIE(of_packet_in_match_get(packet_in, &match) == 0);
    port_no = match.fields.in_port;
#if 0
    /* FIXME check metadata? */
    if ((match.fields.metadata & (OFP_BSN_PKTIN_FLAG_ARP|OFP_BSN_PKTIN_FLAG_ARP
                                  _TARGET)) == 0) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
#endif

    of_packet_in_data_get(packet_in, &octets);
    ppe_packet_init(&ppep, octets.data, octets.bytes);
    if (ppe_parse(&ppep) < 0) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed");
        debug_counter_inc(&pktin_parse_failure);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* igmpa always expects to receive the vlan tag */
    if (!ppe_header_get(&ppep, PPE_HEADER_8021Q)) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: missing vlan tag");
        debug_counter_inc(&pktin_parse_failure);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
    ppe_field_get(&ppep, PPE_FIELD_8021Q_VLAN, &vlan_vid);

    if (!ppe_header_get(&ppep, PPE_HEADER_IP4)) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: missing ipv4 header");
        debug_counter_inc(&pktin_parse_failure);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
    ppe_field_get(&ppep, PPE_FIELD_IP4_TOTAL_LENGTH, &l3_len);
    ppe_field_get(&ppep, PPE_FIELD_IP4_HEADER_SIZE, &l3hdr_words);
    l4_len = l3_len - l3hdr_words * 4;

    if (!ppe_header_get(&ppep, PPE_HEADER_IGMP)) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: not IGMP packet");
        debug_counter_inc(&pktin_parse_failure);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    if (l4_len < 8) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: IGMP packet length %u too short", l4_len);
        debug_counter_inc(&pktin_parse_failure);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* checksum entire payload */
    payload = ppe_header_get(&ppep, PPE_HEADER_IGMP);
    checksum = igmpa_sum16(payload, l4_len);
    if (checksum != 0xffff) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: bad checksum");
        debug_counter_inc(&pktin_bad_checksum);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_field_get(&ppep, PPE_FIELD_IGMP_TYPE, &msg_type);
    switch (msg_type) {
    case PPE_IGMP_TYPE_QUERY:
        ppe_field_get(&ppep, PPE_FIELD_IGMP_GROUP_ADDRESS, &ipv4_addr);
        if (!ipv4_addr) {
            debug_counter_inc(&gq_rx_count);
            rc = handle_general_query(&ppep, port_no, vlan_vid);
        } else {
            debug_counter_inc(&gsq_rx_count);
            rc = INDIGO_CORE_LISTENER_RESULT_DROP;
        }
        break;
    case PPE_IGMP_TYPE_V1_REPORT:  /* fall-through */
    case PPE_IGMP_TYPE_V2_REPORT:
        debug_counter_inc(&report_rx_count);
        rc = handle_report(&ppep, port_no, vlan_vid);
        break;
    case PPE_IGMP_TYPE_LEAVE:
        /* forward to controller */
        debug_counter_inc(&leave_rx_count);
        rc = INDIGO_CORE_LISTENER_RESULT_PASS;
        break;
    default:
        debug_counter_inc(&unknown_rx_count);
        rc = INDIGO_CORE_LISTENER_RESULT_DROP;
        break;
        
    }

    return rc;
}


/* print debug counters */
void
igmpa_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "Debug counters:\n");
    aim_printf(pvs, "pktin  %"PRIu64"\n",
               debug_counter_get(&pktin_count));
    aim_printf(pvs, "pktin_parse_failure  %"PRIu64"\n",
               debug_counter_get(&pktin_parse_failure));
    aim_printf(pvs, "pktin_bad_checksum  %"PRIu64"\n",
               debug_counter_get(&pktin_bad_checksum));
    aim_printf(pvs, "gq_rx  %"PRIu64"\n",
               debug_counter_get(&gq_rx_count));
    aim_printf(pvs, "gsq_rx  %"PRIu64"\n",
               debug_counter_get(&gsq_rx_count));
    aim_printf(pvs, "report_rx  %"PRIu64"\n",
               debug_counter_get(&report_rx_count));
    aim_printf(pvs, "leave_rx  %"PRIu64"\n",
               debug_counter_get(&leave_rx_count));
    aim_printf(pvs, "unknown_rx  %"PRIu64"\n",
               debug_counter_get(&unknown_rx_count));

    igmpa_timeout_stats_show(pvs);
    igmpa_rx_port_group_stats_show(pvs);
    igmpa_tx_port_group_stats_show(pvs);
    igmpa_report_expect_stats_show(pvs);
    igmpa_report_tx_stats_show(pvs);
    igmpa_gq_expect_stats_show(pvs);
    igmpa_gq_tx_stats_show(pvs);
}


indigo_error_t
igmpa_init(void)
{
    aim_ratelimiter_init(&igmpa_pktin_log_limiter, 1000*1000, 5, NULL);

    igmpa_timeout_table_init();
    igmpa_rx_port_group_table_init();
    igmpa_tx_port_group_table_init();
    igmpa_report_expect_table_init();
    igmpa_report_tx_table_init();
    igmpa_gq_expect_table_init();
    igmpa_gq_tx_table_init();

    indigo_core_packet_in_listener_register(handle_pktin);

    debug_counter_register(&pktin_count, "igmpa.pktin",
                           "IGMP pktin received");
    debug_counter_register(&pktin_parse_failure, "igmpa.pktin_parse_failure",
                           "IGMP pktin parsing failed, forward to controller");
    debug_counter_register(&pktin_bad_checksum,
                           "igmpa.pktin_bad_checksum",
                           "IGMP pktin bad checksum, forward to controller");
    debug_counter_register(&gq_rx_count, "igmpa.gq_rx",
                           "IGMP general query received");
    debug_counter_register(&gsq_rx_count, "igmpa.gsq_rx",
                           "IGMP group-specific query received");
    debug_counter_register(&report_rx_count, "igmpa.report_rx",
                           "IGMP report received");
    debug_counter_register(&leave_rx_count, "igmpa.leave_rx",
                           "IGMP leave received");
    debug_counter_register(&gq_rx_count, "igmpa.unknown_rx",
                           "Unknown IGMP packet received");

    return INDIGO_ERROR_NONE;
}


void
igmpa_finish(void)
{
    indigo_core_packet_in_listener_unregister(handle_pktin);

    igmpa_gq_tx_table_finish();
    igmpa_gq_expect_table_finish();
    igmpa_report_tx_table_finish();
    igmpa_report_expect_table_finish();
    igmpa_tx_port_group_table_finish();
    igmpa_rx_port_group_table_finish();
    igmpa_timeout_table_finish();
}

