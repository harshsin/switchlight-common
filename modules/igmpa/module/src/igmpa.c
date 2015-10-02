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
#include "pim_expect_table.h"

#include "igmpa_int.h"
#include "igmpa_log.h"


/* logging rate limiter */
static aim_ratelimiter_t igmpa_pktin_log_limiter;


/* debug counters */
DEBUG_COUNTER(pktin_count, "igmpa.pktin",
              "IGMP pktin received");
DEBUG_COUNTER(pktin_parse_failure, "igmpa.pktin_parse_failure",
              "IGMP pktin parsing failed, forward to controller");
DEBUG_COUNTER(igmp_rx_count, "igmpa.igmp_rx",
              "IGMP packet received");
DEBUG_COUNTER(igmp_too_short, "igmpa.igmp_too_short",
              "IGMP too short, forward to controller");
DEBUG_COUNTER(igmp_bad_checksum, "igmpa.igmp_bad_checksum",
              "IGMP bad checksum, forward to controller");
DEBUG_COUNTER(gq_rx_count, "igmpa.gq_rx",
              "IGMP general query received");
DEBUG_COUNTER(gsq_rx_count, "igmpa.gsq_rx",
              "IGMP group-specific query received");
DEBUG_COUNTER(report_rx_count, "igmpa.report_rx",
              "IGMP report received");
DEBUG_COUNTER(leave_rx_count, "igmpa.leave_rx",
              "IGMP leave received");
DEBUG_COUNTER(unknown_igmp_rx_count, "igmpa.unknown_igmp_rx",
              "Unknown IGMP packet received");
DEBUG_COUNTER(pim_rx_count, "igmpa.pim_rx",
              "PIM packet received");
DEBUG_COUNTER(pim_too_short, "igmpa.pim_too_short",
              "PIM too short, forward to controller");
DEBUG_COUNTER(pim_bad_version, "igmpa.pim_bad_version",
              "PIM version mismatch, forward to controller");
DEBUG_COUNTER(pim_bad_dstip, "igmpa.pim_bad_dstip",
              "PIM bad destination ip, forward to controller");
DEBUG_COUNTER(pim_bad_checksum, "igmpa.pim_bad_checksum",
              "PIM bad checksum, forward to controller");


/*----------*/
/* packet handlers */

static indigo_core_listener_result_t
handle_general_query(ppe_packet_t *ppep, 
                     of_port_no_t in_port, uint16_t vlan_vid)
{
    rx_port_group_entry_t *rpg_entry;
    gq_expect_entry_t *gq_entry;

    /* get rx port group name */
    rpg_entry = igmpa_rx_port_group_lookup(in_port);
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
              of_port_no_t in_port, uint16_t vlan_vid)
{
    rx_port_group_entry_t *rpg_entry;
    report_expect_entry_t *report_entry;
    uint32_t ipv4;

    ppe_field_get(ppep, PPE_FIELD_IGMP_GROUP_ADDRESS, &ipv4);

    /* get rx port group name */
    rpg_entry = igmpa_rx_port_group_lookup(in_port);
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
handle_igmp_pkt(ppe_packet_t *ppep, 
                of_port_no_t in_port, uint16_t vlan_vid, uint32_t l4_len)
{
    indigo_core_listener_result_t rc;
    uint8_t *payload;
    uint16_t checksum;
    uint32_t msg_type;
    uint32_t ipv4_addr;

    debug_counter_inc(&igmp_rx_count);

    if (l4_len < 8) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "IGMP packet length %u too short", l4_len);
        debug_counter_inc(&igmp_too_short);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* checksum entire payload */
    payload = ppe_header_get(ppep, PPE_HEADER_IGMP);
    checksum = igmpa_sum16(payload, l4_len);
    if (checksum != 0xffff) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "Packet_in parsing failed: bad checksum");
        debug_counter_inc(&igmp_bad_checksum);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_field_get(ppep, PPE_FIELD_IGMP_TYPE, &msg_type);
    switch (msg_type) {
    case PPE_IGMP_TYPE_QUERY:
        ppe_field_get(ppep, PPE_FIELD_IGMP_GROUP_ADDRESS, &ipv4_addr);
        if (!ipv4_addr) {
            debug_counter_inc(&gq_rx_count);
            rc = handle_general_query(ppep, in_port, vlan_vid);
        } else {
            debug_counter_inc(&gsq_rx_count);
            rc = INDIGO_CORE_LISTENER_RESULT_DROP;
        }
        break;
    case PPE_IGMP_TYPE_V1_REPORT:  /* fall-through */
    case PPE_IGMP_TYPE_V2_REPORT:
        debug_counter_inc(&report_rx_count);
        rc = handle_report(ppep, in_port, vlan_vid);
        break;
    case PPE_IGMP_TYPE_LEAVE:
        /* forward to controller */
        debug_counter_inc(&leave_rx_count);
        rc = INDIGO_CORE_LISTENER_RESULT_PASS;
        break;
    default:
        debug_counter_inc(&unknown_igmp_rx_count);
        rc = INDIGO_CORE_LISTENER_RESULT_DROP;
        break;
    }

    return rc;
}


static indigo_core_listener_result_t
handle_pim_hello(ppe_packet_t *ppep, 
                 of_port_no_t in_port, uint16_t vlan_vid)
{
    rx_port_group_entry_t *rpg_entry;
    pim_expect_entry_t *pim_entry;

    /* get rx port group name */
    rpg_entry = igmpa_rx_port_group_lookup(in_port);
    if (rpg_entry == NULL) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* get pim expectation */
    pim_entry = igmpa_pim_expect_lookup(rpg_entry->value.name, vlan_vid);
    if (pim_entry) {
        uint64_t now = INDIGO_CURRENT_TIME;
        /* expected packet: reset timer, do not send to controller */
        pim_entry->time_last_hit = now;
        pim_entry->rx_packets++;
        /* reschedule timer */
        AIM_LOG_TRACE("reschedule pim timer");
        igmpa_pim_expect_reschedule(pim_entry, now + igmpa_pim_expect_timeout);
        return INDIGO_CORE_LISTENER_RESULT_DROP;
    } else {
        /* not found, send to controller */
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}

static indigo_core_listener_result_t
handle_pim_pkt(ppe_packet_t *ppep, 
               of_port_no_t in_port, uint16_t vlan_vid, uint32_t l4_len)
{
    uint8_t *payload;
    const uint32_t all_routers = 0xe000000d;  /* 224.0.0.13 */
    uint32_t ipv4_dst;
    uint16_t checksum;
    uint32_t msg_ver;
    uint32_t msg_type;

    debug_counter_inc(&pim_rx_count);

    if (l4_len < 4) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "PIM packet length %u too short", l4_len);
        debug_counter_inc(&pim_too_short);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* checksum entire payload */
    payload = ppe_header_get(ppep, PPE_HEADER_PIM);
    checksum = igmpa_sum16(payload, l4_len);
    if (checksum != 0xffff) {
        AIM_LOG_RL_ERROR(&igmpa_pktin_log_limiter, os_time_monotonic(),
                         "PIM bad checksum %04x", 
                         checksum);
        debug_counter_inc(&pim_bad_checksum);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_field_get(ppep, PPE_FIELD_PIM_VERSION, &msg_ver);
    if (msg_ver != 2) {
        /* we only support version 2 */
        debug_counter_inc(&pim_bad_version);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* ip-dst for pim hello must be to all-routers */
    ppe_field_get(ppep, PPE_FIELD_IP4_DST_ADDR, &ipv4_dst);
    if (ipv4_dst != all_routers) {
        debug_counter_inc(&pim_bad_dstip);
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    ppe_field_get(ppep, PPE_FIELD_PIM_TYPE, &msg_type);
    if (msg_type == PPE_PIM_TYPE_HELLO) {
        return handle_pim_hello(ppep, in_port, vlan_vid);
    } else {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}


/* sends a IGMP or PIM packet directly to the IGMP agent */
indigo_core_listener_result_t
igmpa_receive_pkt(ppe_packet_t *ppep, of_port_no_t in_port)
{
    uint32_t vlan_vid;
    uint32_t l3_len;
    uint32_t l3hdr_words;
    uint32_t l4_len;

    /* igmpa always expects to receive the vlan tag */
    if (!ppe_header_get(ppep, PPE_HEADER_8021Q)) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
    ppe_field_get(ppep, PPE_FIELD_8021Q_VLAN, &vlan_vid);

    if (!ppe_header_get(ppep, PPE_HEADER_IP4)) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    /* FIXME extend PPE to get flags and frag offset */
    /* FIXME drop frags: ipv4 MF set or ipv4 frag offset nonzero */

    ppe_field_get(ppep, PPE_FIELD_IP4_TOTAL_LENGTH, &l3_len);
    ppe_field_get(ppep, PPE_FIELD_IP4_HEADER_SIZE, &l3hdr_words);
    l4_len = l3_len - l3hdr_words * 4;

    if (ppe_header_get(ppep, PPE_HEADER_IGMP)) {
        return handle_igmp_pkt(ppep, in_port, vlan_vid, l4_len);
    } else if (ppe_header_get(ppep, PPE_HEADER_PIM)) {
        return handle_pim_pkt(ppep, in_port, vlan_vid, l4_len);
    } else {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }
}


#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
/* top-level packet_in handler */
static indigo_core_listener_result_t
handle_pktin(of_packet_in_t *packet_in)
{
    of_match_t match;
    of_port_no_t in_port;
    of_octets_t octets;
    ppe_packet_t ppep;

    debug_counter_inc(&pktin_count);

    if (packet_in->version <= OF_VERSION_1_1) {
        return INDIGO_CORE_LISTENER_RESULT_PASS;
    }

    AIM_TRUE_OR_DIE(of_packet_in_match_get(packet_in, &match) == 0);
    in_port = match.fields.in_port;
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

    return igmpa_receive_pkt(&ppep, in_port);
}
#endif /* SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER */


/* clear debug counters */
void
igmpa_stats_clear(void)
{
    debug_counter_reset(&pktin_count);
    debug_counter_reset(&pktin_parse_failure);
    debug_counter_reset(&igmp_rx_count);
    debug_counter_reset(&igmp_too_short);
    debug_counter_reset(&igmp_bad_checksum);
    debug_counter_reset(&gq_rx_count);
    debug_counter_reset(&gsq_rx_count);
    debug_counter_reset(&report_rx_count);
    debug_counter_reset(&leave_rx_count);
    debug_counter_reset(&unknown_igmp_rx_count);
    debug_counter_reset(&pim_rx_count);
    debug_counter_reset(&pim_too_short);
    debug_counter_reset(&pim_bad_version);
    debug_counter_reset(&pim_bad_dstip);
    debug_counter_reset(&pim_bad_checksum);

    igmpa_timeout_stats_clear();
    igmpa_rx_port_group_stats_clear();
    igmpa_tx_port_group_stats_clear();
    igmpa_report_expect_stats_clear();
    igmpa_report_tx_stats_clear();
    igmpa_gq_expect_stats_clear();
    igmpa_gq_tx_stats_clear();
    igmpa_pim_expect_stats_clear();
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
    aim_printf(pvs, "igmp_rx  %"PRIu64"\n",
               debug_counter_get(&igmp_rx_count));
    aim_printf(pvs, "igmp_too_short  %"PRIu64"\n",
               debug_counter_get(&igmp_too_short));
    aim_printf(pvs, "igmp_bad_checksum  %"PRIu64"\n",
               debug_counter_get(&igmp_bad_checksum));
    aim_printf(pvs, "gq_rx  %"PRIu64"\n",
               debug_counter_get(&gq_rx_count));
    aim_printf(pvs, "gsq_rx  %"PRIu64"\n",
               debug_counter_get(&gsq_rx_count));
    aim_printf(pvs, "report_rx  %"PRIu64"\n",
               debug_counter_get(&report_rx_count));
    aim_printf(pvs, "leave_rx  %"PRIu64"\n",
               debug_counter_get(&leave_rx_count));
    aim_printf(pvs, "unknown_igmp_rx  %"PRIu64"\n",
               debug_counter_get(&unknown_igmp_rx_count));
    aim_printf(pvs, "pim_rx  %"PRIu64"\n",
               debug_counter_get(&pim_rx_count));
    aim_printf(pvs, "pim_too_short  %"PRIu64"\n",
               debug_counter_get(&pim_too_short));
    aim_printf(pvs, "pim_bad_version  %"PRIu64"\n",
               debug_counter_get(&pim_bad_version));
    aim_printf(pvs, "pim_bad_dstip  %"PRIu64"\n",
               debug_counter_get(&pim_bad_dstip));
    aim_printf(pvs, "pim_bad_checksum  %"PRIu64"\n",
               debug_counter_get(&pim_bad_checksum));

    igmpa_timeout_stats_show(pvs);
    igmpa_rx_port_group_stats_show(pvs);
    igmpa_tx_port_group_stats_show(pvs);
    igmpa_report_expect_stats_show(pvs);
    igmpa_report_tx_stats_show(pvs);
    igmpa_gq_expect_stats_show(pvs);
    igmpa_gq_tx_stats_show(pvs);
    igmpa_pim_expect_stats_show(pvs);
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
    igmpa_pim_expect_table_init();

#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_register(handle_pktin);
#endif

    return INDIGO_ERROR_NONE;
}


void
igmpa_finish(void)
{
#if SLSHARED_CONFIG_PKTIN_LISTENER_REGISTER == 1
    indigo_core_packet_in_listener_unregister(handle_pktin);
#endif

    igmpa_pim_expect_table_finish();
    igmpa_gq_tx_table_finish();
    igmpa_gq_expect_table_finish();
    igmpa_report_tx_table_finish();
    igmpa_report_expect_table_finish();
    igmpa_tx_port_group_table_finish();
    igmpa_rx_port_group_table_finish();
    igmpa_timeout_table_finish();
}

