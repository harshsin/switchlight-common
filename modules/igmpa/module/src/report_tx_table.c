/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * report tx gentable implementation.
 */

/*
 * Implements "igmp_report_tx" gentable.
 * key: reference (igmp_tx_port_group), vlan_vid, ipv4 (multicast address) 
 * value: vlan_vid, ipv4_src, eth_src
 * stats: tx_packets, idle_time
 */


#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <SocketManager/socketmanager.h>
#include <PPE/ppe.h>
#include <timer_wheel/timer_wheel.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared.h>

#include "igmpa_int.h"
#include "timeout_table.h"
#include "tx_port_group_table.h"
#include "report_tx_table.h"


/* timer wheel for sending reports to mrouters */
static timer_wheel_t *tw_report_tx;


/* debug counters */
static debug_counter_t report_tx_add_success;
static debug_counter_t report_tx_add_failure;
static debug_counter_t report_tx_modify_success;
static debug_counter_t report_tx_modify_failure;
static debug_counter_t report_tx_delete_success;
static debug_counter_t report_tx_count;
static debug_counter_t report_tx_failure;


static void
report_tx_send_packet(report_tx_entry_t *entry)
{
    tx_port_group_entry_t *tpg_entry =
        igmpa_tx_port_group_lookup_by_name(entry->key.tx_port_group_name);

    if (!tpg_entry) {
        debug_counter_inc(&report_tx_failure);
        return;
    }

    uint8_t dst_mac[OF_MAC_ADDR_BYTES] = {
        0x01, 0x00, 0x5e,
        (entry->key.ipv4 & 0x7f0000) >> 16,
        (entry->key.ipv4 & 0xff00) >> 8,
        (entry->key.ipv4 & 0xff),
    };

    igmpa_pkt_params_t params = {
        .eth_src = entry->value.eth_src.addr,
        .eth_dst = dst_mac,
        .vlan_vid = entry->value.vlan_vid_tx,
        .ipv4_src = entry->value.ipv4_src,
        .ipv4_dst = entry->key.ipv4,
        .igmp_type = PPE_IGMP_TYPE_V2_REPORT,
        .igmp_group_addr = entry->key.ipv4,
        .output_port_no = tpg_entry->value.port_no,
    };

    /* FIXME send v1 or v2 report? */
    AIM_LOG_VERBOSE("send report, port group %s, vlan_vid %u, ipv4 %{ipv4a}",
                    entry->key.tx_port_group_name, entry->key.vlan_vid,
                    entry->key.ipv4);

    if (igmpa_send_igmp_packet(&params)) {
        debug_counter_inc(&report_tx_failure);
    } else {
        debug_counter_inc(&report_tx_count);
    }
}


/*
 * triggers report to be sent to mrouter
 * default: 125 seconds
 */
static void
report_tx_timer(void *cookie)
{
    timer_wheel_entry_t *timer;
    indigo_time_t now = INDIGO_CURRENT_TIME;
    int reports = 0;
    const int max_reports = 32;

    while (reports < max_reports &&
           !ind_soc_should_yield() &&
           (timer = timer_wheel_next(tw_report_tx, now))) {
        report_tx_entry_t *entry = 
            container_of(timer, timer_entry, report_tx_entry_t);

        report_tx_send_packet(entry);
        reports++;
        timer_wheel_insert(tw_report_tx, &entry->timer_entry,
                           now + igmpa_report_tx_timeout);
    }

    /* reregister sooner if more expired entries */
    if (timer_wheel_peek(tw_report_tx, now)) {
        ind_soc_timer_event_register(report_tx_timer, NULL, 100);
    } else {
        ind_soc_timer_event_register(report_tx_timer, NULL, 1000);
    }
}


/*----------*/
/* gentable handling */

static indigo_error_t
report_tx_parse_key(of_list_bsn_tlv_t *tlvs, report_tx_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected name key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_REFERENCE) {
        uint16_t table_id;
        of_object_t refkey;
        tx_port_group_entry_t *tpg_entry;

        of_bsn_tlv_reference_table_id_get(&tlv, &table_id);
        of_bsn_tlv_reference_key_bind(&tlv, &refkey);

        tpg_entry = igmpa_tx_port_group_lookup_by_ref(table_id, &refkey);
        if (tpg_entry) {
            IGMPA_MEMCPY(key->tx_port_group_name,
                         tpg_entry->key.name, IGMP_NAME_LEN);
        } else {
            AIM_LOG_ERROR("%s: could not find tx port group",
                          __FUNCTION__);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected reference key TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: unexpected end of key list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &key->vlan_vid);
        /* masked to 12 bits */
        key->vlan_vid = VLAN_VID(key->vlan_vid);
    } else {
        AIM_LOG_ERROR("%s: expected vlan key TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: unexpected end of key list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_IPV4) {
        of_bsn_tlv_ipv4_value_get(&tlv, &key->ipv4);
        if (!IS_IPV4_MULTICAST_ADDR(key->ipv4)) {
            AIM_LOG_ERROR("%s: expected multicast address, got %08x",
                          __FUNCTION__, key->ipv4);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected ipv4 key TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of key TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_tx_parse_value(of_list_bsn_tlv_t *tlvs, report_tx_value_t *value)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &value->vlan_vid_tx);
        /* masked to 12 bits */
        value->vlan_vid_tx = VLAN_VID(value->vlan_vid_tx);
    } else {
        AIM_LOG_ERROR("%s: expected vlan_vid TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: unexpected end of value list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_IPV4_SRC) {
        of_bsn_tlv_ipv4_src_value_get(&tlv, &value->ipv4_src);
        if (IS_IPV4_MULTICAST_ADDR(value->ipv4_src)) {
            AIM_LOG_ERROR("%s: expected unicast address, got %08x",
                          __FUNCTION__, value->ipv4_src);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected ipv4_src TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: unexpected end of value list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_ETH_SRC) {
        of_bsn_tlv_eth_src_value_get(&tlv, &value->eth_src);
    } else {
        AIM_LOG_ERROR("%s: expected eth_src TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of value TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_tx_add(void *table_priv,
              of_list_bsn_tlv_t *key_tlvs,
              of_list_bsn_tlv_t *value_tlvs,
              void **entry_priv)
{
    indigo_error_t rv;
    report_tx_key_t key;
    report_tx_value_t value;
    report_tx_entry_t *entry;

    IGMPA_MEMSET(&key, 0, sizeof(key));
    rv = report_tx_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&report_tx_add_failure);
        return rv;
    }

    rv = report_tx_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&report_tx_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    report_tx_send_packet(entry);
    timer_wheel_insert(tw_report_tx, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_report_tx_timeout);

    *entry_priv = entry;
    debug_counter_inc(&report_tx_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_tx_modify(void *table_priv,
                 void *entry_priv,
                 of_list_bsn_tlv_t *key_tlvs,
                 of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    report_tx_value_t value;
    report_tx_entry_t *entry = entry_priv;

    rv = report_tx_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&report_tx_modify_failure);
        return rv;
    }

    entry->value = value;

    timer_wheel_remove(tw_report_tx, &entry->timer_entry);
    report_tx_send_packet(entry);
    timer_wheel_insert(tw_report_tx, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_report_tx_timeout);

    debug_counter_inc(&report_tx_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_tx_delete(void *table_priv,
                 void *entry_priv,
                 of_list_bsn_tlv_t *key_tlvs)
{
    report_tx_entry_t *entry = entry_priv;

    timer_wheel_remove(tw_report_tx, &entry->timer_entry);
    aim_free(entry);
    debug_counter_inc(&report_tx_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
report_tx_get_stats(void *table_priv,
                    void *entry_priv,
                    of_list_bsn_tlv_t *key_tlvs,
                    of_list_bsn_tlv_t *stats)
{
    report_tx_entry_t *entry = entry_priv;

    /* idle_time */
    {
        uint64_t idle_time = INDIGO_CURRENT_TIME - entry->time_last_hit;
        of_bsn_tlv_idle_time_t tlv;
        of_bsn_tlv_idle_time_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_idle_time_value_set(&tlv, idle_time);
    }

    /* rx_packets */
    {
        of_bsn_tlv_rx_packets_t tlv;
        of_bsn_tlv_rx_packets_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_rx_packets_value_set(&tlv, entry->tx_packets);
    }    
}

static indigo_core_gentable_t *report_tx_gentable;
static const indigo_core_gentable_ops_t report_tx_ops = {
    .add = report_tx_add,
    .modify = report_tx_modify,
    .del = report_tx_delete,
    .get_stats = report_tx_get_stats,
};


void
igmpa_report_tx_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "report_tx_add  %"PRIu64"\n",
               debug_counter_get(&report_tx_add_success));
    aim_printf(pvs, "report_tx_add_failure  %"PRIu64"\n",
               debug_counter_get(&report_tx_add_failure));
    aim_printf(pvs, "report_tx_modify  %"PRIu64"\n",
               debug_counter_get(&report_tx_modify_success));
    aim_printf(pvs, "report_tx_modify_failure  %"PRIu64"\n",
               debug_counter_get(&report_tx_modify_failure));
    aim_printf(pvs, "report_tx_delete  %"PRIu64"\n",
               debug_counter_get(&report_tx_delete_success));
    aim_printf(pvs, "report_tx_count  %"PRIu64"\n",
               debug_counter_get(&report_tx_count));
    aim_printf(pvs, "report_tx_failure  %"PRIu64"\n",
               debug_counter_get(&report_tx_failure));
}


void
igmpa_report_tx_table_init(void)
{
    indigo_error_t rv;

    /* to mrouter: 1k entries, 125s for tx */
    tw_report_tx = timer_wheel_create(1024, 128, INDIGO_CURRENT_TIME);

    rv = ind_soc_timer_event_register(report_tx_timer, NULL, 1000);
    AIM_ASSERT(rv == INDIGO_ERROR_NONE,
               "Failed to register report tx timer: %s",
               indigo_strerror(rv));

    indigo_core_gentable_register("igmp_report_tx",
                                  &report_tx_ops, NULL,
                                  8*1024, 1024, &report_tx_gentable);

    debug_counter_register(&report_tx_add_success,
                           "igmpa.report_tx_add",
                           "report_tx table entry added");
    debug_counter_register(&report_tx_add_failure,
                           "igmpa.report_tx_add_failure",
                           "report_tx table add unsuccessful");
    debug_counter_register(&report_tx_modify_success,
                           "igmpa.report_tx_modify",
                           "report_tx table entry modified");
    debug_counter_register(&report_tx_modify_failure,
                           "igmpa.report_tx_modify_failure",
                           "repoect_expect table modify unsuccessful");
    debug_counter_register(&report_tx_delete_success,
                           "igmpa.report_tx_delete",
                           "report_tx table entry deleted");
    debug_counter_register(&report_tx_count, "igmpa.report_tx",
                           "IGMP report transmitted");
    debug_counter_register(&report_tx_failure, "igmpa.report_tx_failure",
                           "IGMP report transmit unsuccessful");
}

void
igmpa_report_tx_table_finish(void)
{
    indigo_core_gentable_unregister(report_tx_gentable);

    ind_soc_timer_event_unregister(report_tx_timer, NULL);

    timer_wheel_destroy(tw_report_tx);
}
