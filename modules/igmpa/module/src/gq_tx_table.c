/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * general query tx gentable implementation.
 */

/*
 * Implements "igmp_general_query_tx" gentable.
 * key: name (igmp_tx_port_group), vlan_vid
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
#include "gq_tx_table.h"


/* timer wheel for sending general queries to hosts */
static timer_wheel_t *tw_gq_tx;


/* debug counters */
static debug_counter_t gq_tx_add_success;
static debug_counter_t gq_tx_add_failure;
static debug_counter_t gq_tx_modify_success;
static debug_counter_t gq_tx_modify_failure;
static debug_counter_t gq_tx_delete_success;
static debug_counter_t gq_tx_count;
static debug_counter_t gq_tx_failure;


static void
gq_tx_send_packet(gq_tx_entry_t *entry)
{
    tx_port_group_entry_t *tpg_entry =
        igmpa_tx_port_group_lookup_by_name(entry->key.tx_port_group_name);

    if (!tpg_entry) {
        debug_counter_inc(&gq_tx_failure);
        return;
    }

    uint8_t dst_mac[OF_MAC_ADDR_BYTES] =  /* L2 multicast MAC for 224.0.0.1 */
        { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };

    igmpa_pkt_params_t params = {
        .eth_src = entry->value.eth_src.addr,
        .eth_dst = dst_mac,
        .vlan_vid = entry->value.vlan_vid_tx,
        .ipv4_src = entry->value.ipv4_src,
        .ipv4_dst = 0xe0000001, /* 224.0.0.1 */
        .igmp_type = PPE_IGMP_TYPE_QUERY,
        .igmp_group_addr = 0,  /* field is cleared */
        .output_port_no = tpg_entry->value.port_no,
    };

    AIM_LOG_VERBOSE("send general query, port group %s, vlan_vid %u",
                    entry->key.tx_port_group_name, entry->key.vlan_vid);

    if (igmpa_send_igmp_packet(&params)) {
        debug_counter_inc(&gq_tx_failure);
    } else {
        entry->tx_packets++;
        debug_counter_inc(&gq_tx_count);
    }
}


/*
 * triggers general query to be sent to host
 * default: 125 seconds
 */
static void
gq_tx_timer(void *cookie)
{
    timer_wheel_entry_t *timer;
    indigo_time_t now = INDIGO_CURRENT_TIME;
    int gqs = 0;
    const int max_gqs = 32;

    while (gqs < max_gqs &&
           !ind_soc_should_yield() &&
           (timer = timer_wheel_next(tw_gq_tx, now))) {
        gq_tx_entry_t *entry = container_of(timer, timer_entry, gq_tx_entry_t);

        gq_tx_send_packet(entry);
        gqs++;
        timer_wheel_insert(tw_gq_tx, &entry->timer_entry,
                           now + igmpa_gq_tx_timeout);
    }

    /* reregister sooner if more expired entries */
    if (timer_wheel_peek(tw_gq_tx, now)) {
        ind_soc_timer_event_register(gq_tx_timer, NULL, 100);
    } else {
        ind_soc_timer_event_register(gq_tx_timer, NULL, 1000);
    }
}


/*----------*/
/* gentable handling */

static indigo_error_t
gq_tx_parse_key(of_list_bsn_tlv_t *tlvs, gq_tx_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected reference TLV, instead got end of list",
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
            char name[IGMP_NAME_LEN];
            igmpa_refkey_name_get(&refkey, name);
            AIM_LOG_ERROR("%s: could not find tx port group '%s'",
                          __FUNCTION__, name);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected reference TLV, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    if (of_list_bsn_tlv_next(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: unexpected end of key TLV list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_VLAN_VID) {
        of_bsn_tlv_vlan_vid_value_get(&tlv, &key->vlan_vid);
        /* masked to 12 bits */
        key->vlan_vid = VLAN_VID(key->vlan_vid);
    } else {
        AIM_LOG_ERROR("%s: expected vlan_vid key TLV, instead got %s",
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
gq_tx_parse_value(of_list_bsn_tlv_t *tlvs, gq_tx_value_t *value)
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
gq_tx_add(void *table_priv,
          of_list_bsn_tlv_t *key_tlvs,
          of_list_bsn_tlv_t *value_tlvs,
          void **entry_priv)
{
    indigo_error_t rv;
    gq_tx_key_t key;
    gq_tx_value_t value;
    gq_tx_entry_t *entry;

    IGMPA_MEMSET(&key, 0, sizeof(key));
    rv = gq_tx_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&gq_tx_add_failure);
        return rv;
    }

    rv = gq_tx_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&gq_tx_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    gq_tx_send_packet(entry);
    timer_wheel_insert(tw_gq_tx, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_gq_tx_timeout);

    *entry_priv = entry;
    debug_counter_inc(&gq_tx_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
gq_tx_modify(void *table_priv,
             void *entry_priv,
             of_list_bsn_tlv_t *key_tlvs,
             of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    gq_tx_value_t value;
    gq_tx_entry_t *entry = entry_priv;

    rv = gq_tx_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&gq_tx_modify_failure);
        return rv;
    }

    entry->value = value;

    timer_wheel_remove(tw_gq_tx, &entry->timer_entry);
    gq_tx_send_packet(entry);
    timer_wheel_insert(tw_gq_tx, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_gq_tx_timeout);

    debug_counter_inc(&gq_tx_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
gq_tx_delete(void *table_priv,
             void *entry_priv,
             of_list_bsn_tlv_t *key_tlvs)
{
    gq_tx_entry_t *entry = entry_priv;

    timer_wheel_remove(tw_gq_tx, &entry->timer_entry);
    aim_free(entry);
    debug_counter_inc(&gq_tx_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
gq_tx_get_stats(void *table_priv,
                void *entry_priv,
                of_list_bsn_tlv_t *key_tlvs,
                of_list_bsn_tlv_t *stats)
{
    gq_tx_entry_t *entry = entry_priv;

    /* idle_time */
    {
        uint64_t idle_time = INDIGO_CURRENT_TIME - entry->time_last_hit;
        of_bsn_tlv_idle_time_t tlv;
        of_bsn_tlv_idle_time_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_idle_time_value_set(&tlv, idle_time);
    }

    /* tx_packets */
    {
        of_bsn_tlv_tx_packets_t tlv;
        of_bsn_tlv_tx_packets_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_tx_packets_value_set(&tlv, entry->tx_packets);
    }    
}

static indigo_core_gentable_t *gq_tx_gentable;
static const indigo_core_gentable_ops_t gq_tx_ops = {
    .add = gq_tx_add,
    .modify = gq_tx_modify,
    .del = gq_tx_delete,
    .get_stats = gq_tx_get_stats,
};


void
igmpa_gq_tx_stats_clear(void)
{
    debug_counter_reset(&gq_tx_add_success);
    debug_counter_reset(&gq_tx_add_failure);
    debug_counter_reset(&gq_tx_modify_success);
    debug_counter_reset(&gq_tx_modify_failure);
    debug_counter_reset(&gq_tx_delete_success);
    debug_counter_reset(&gq_tx_count);
    debug_counter_reset(&gq_tx_failure);
}


void
igmpa_gq_tx_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "gq_tx_add  %"PRIu64"\n",
               debug_counter_get(&gq_tx_add_success));
    aim_printf(pvs, "gq_tx_add_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_add_failure));
    aim_printf(pvs, "gq_tx_modify  %"PRIu64"\n",
               debug_counter_get(&gq_tx_modify_success));
    aim_printf(pvs, "gq_tx_modify_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_modify_failure));
    aim_printf(pvs, "gq_tx_delete  %"PRIu64"\n",
               debug_counter_get(&gq_tx_delete_success));
    aim_printf(pvs, "gq_tx_count  %"PRIu64"\n",
               debug_counter_get(&gq_tx_count));
    aim_printf(pvs, "gq_tx_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_failure));
}


void
igmpa_gq_tx_table_init(void)
{
    indigo_error_t rv;

    /* to host: 16k entries, 125s for tx */
    /* bucket size is (125*1000)ms / (16*1024)entries ~= 8 */
    tw_gq_tx = timer_wheel_create(16*1024, 8, INDIGO_CURRENT_TIME);

    rv = ind_soc_timer_event_register(gq_tx_timer, NULL, 1000);
    AIM_ASSERT(rv == INDIGO_ERROR_NONE,
               "Failed to register general query tx timer: %s",
               indigo_strerror(rv));

    indigo_core_gentable_register("igmp_general_query_tx",
                                  &gq_tx_ops, NULL,
                                  16*1024, 1024, &gq_tx_gentable);

    debug_counter_register(&gq_tx_add_success,
                           "igmpa.gq_tx_add",
                           "gq_tx table entry added");
    debug_counter_register(&gq_tx_add_failure,
                           "igmpa.gq_tx_add_failure",
                           "gq_tx table add unsuccessful");
    debug_counter_register(&gq_tx_modify_success,
                           "igmpa.gq_tx_modify",
                           "gq_tx table entry modified");
    debug_counter_register(&gq_tx_modify_failure,
                           "igmpa.gq_tx_modify_failure",
                           "gq_tx table modify unsuccessful");
    debug_counter_register(&gq_tx_delete_success,
                           "igmpa.gq_tx_delete",
                           "gq_tx table entry deleted");
    debug_counter_register(&gq_tx_count, "igmpa.gq_tx",
                           "IGMP general query transmitted");
    debug_counter_register(&gq_tx_failure, "igmpa.gq_tx_failure",
                           "IGMP general query transmit unsuccessful");
}

void
igmpa_gq_tx_table_finish(void)
{
    indigo_core_gentable_unregister(gq_tx_gentable);

    ind_soc_timer_event_unregister(gq_tx_timer, NULL);

    timer_wheel_destroy(tw_gq_tx);
}
