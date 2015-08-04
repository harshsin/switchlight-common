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
    ppe_packet_t ppep;
    const int igmp_len = 8;
    const int pkt_len = 14 + 4 + 20 + igmp_len;
    uint8_t pkt_bytes[pkt_len];
    uint8_t dst_mac[OF_MAC_ADDR_BYTES] =  /* L2 multicast MAC for 224.0.0.1 */
        { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };
    uint32_t igmp_checksum;

    AIM_LOG_INFO("send general query, port %u, vlan_vid %u",
                 entry->key.port_no, entry->key.vlan_vid);

    memset(pkt_bytes, 0, sizeof(pkt_bytes));

    /* build and send general query */
    ppe_packet_init(&ppep, pkt_bytes, pkt_len);
    /* set ethertypes before parsing */
    /* FIXME use ppe_packet_format_set? */
    pkt_bytes[12] = 0x81;
    pkt_bytes[13] = 0x00;
    pkt_bytes[16] = 0x08;
    pkt_bytes[17] = 0x00;
    if (ppe_parse(&ppep) < 0) {
        AIM_DIE("ppe_parse failed sending IGMP general query");
    }

    /* ethernet */
    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_SRC_MAC, 
                           entry->value.eth_src.addr) < 0) {
        AIM_DIE("failed to set PPE_FIELD_ETHERNET_SRC_MAC");
    }
    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_DST_MAC, dst_mac) < 0) {
        AIM_DIE("failed to set PPE_FIELD_ETHERNET_DST_MAC");
    }

    /* tag */
    if (ppe_field_set(&ppep, PPE_FIELD_8021Q_VLAN,
                      entry->value.vlan_vid_tx) < 0) {
        AIM_DIE("failed to set PPE_FIELD_8021Q_VLAN");
    }

    /* ipv4 */
    /* FIXME add router alert? */
    if (ppe_build_ipv4_header(&ppep, entry->value.ipv4_src, 
                              0xe0000001, /* 224.0.0.1 */
                              28, 2 /* IGMP */, 1 /* TTL */) < 0) {
        AIM_DIE("failed to build ipv4 header");
    }
    /* igmp */
    if (ppe_field_set(&ppep, PPE_FIELD_IGMP_TYPE, PPE_IGMP_TYPE_QUERY) < 0) {
        AIM_DIE("failed to set PPE_FIELD_IGMP_TYPE");
    }

    /* compute and set IGMP checksum */
    igmp_checksum = igmpa_sum16(ppe_header_get(&ppep, PPE_HEADER_IGMP),
                                igmp_len);
    if (ppe_field_set(&ppep, PPE_FIELD_IGMP_CHECKSUM, 
                      (0xffff - igmp_checksum)) < 0) {
        AIM_DIE("failed to set PPE_FIELD_IGMP_CHECKSUM");
    }

    if (ppe_packet_update(&ppep) < 0) {
        AIM_DIE("ppe_packet_update failed for IGMP general query");
    }

    AIM_LOG_INFO("pkt len %u bytes\n%{idata}", pkt_len, pkt_bytes, pkt_len);

    /* send packet */
    of_octets_t octets = { pkt_bytes, sizeof(pkt_bytes) };
    indigo_error_t rv;
    rv = slshared_fwd_packet_out(&octets, OF_PORT_DEST_CONTROLLER,
                                 entry->key.port_no, QUEUE_ID_INVALID);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_INTERNAL("Failed to send IGMP general query: %s",
                         indigo_strerror(rv));
        debug_counter_inc(&gq_tx_failure);
    }

    debug_counter_inc(&gq_tx_count);
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

        of_bsn_tlv_reference_table_id_get(&tlv, &table_id);
        of_bsn_tlv_reference_key_bind(&tlv, &refkey);

        key->port_no = igmpa_tx_port_group_lookup(table_id, &refkey);
        if (key->port_no == OF_PORT_DEST_NONE) {
            AIM_LOG_ERROR("%s: could not find tx port group",
                          __FUNCTION__);
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

    memset(&key, 0, sizeof(key));
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

    /* rx_packets */
    {
        of_bsn_tlv_rx_packets_t tlv;
        of_bsn_tlv_rx_packets_init(&tlv, stats->version, -1, 1);
        of_list_bsn_tlv_append_bind(stats, &tlv);
        of_bsn_tlv_rx_packets_value_set(&tlv, entry->tx_packets);
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
    aim_printf(pvs, "gq_tx  %"PRIu64"\n",
               debug_counter_get(&gq_tx_count));
    aim_printf(pvs, "gq_tx_failure  %"PRIu64"\n",
               debug_counter_get(&gq_tx_failure));
}


void
igmpa_gq_tx_table_init(void)
{
    indigo_error_t rv;

    /* to host: 16k entries, 125s for tx */
    tw_gq_tx = timer_wheel_create(16*1024, 128, INDIGO_CURRENT_TIME);

    rv = ind_soc_timer_event_register(gq_tx_timer, NULL, 1000);
    AIM_ASSERT(rv == INDIGO_ERROR_NONE,
               "Failed to register general query tx timer: %s",
               indigo_strerror(rv));

    indigo_core_gentable_register("igmp_general_query_tx",
                                  &gq_tx_ops, NULL,
                                  8*1024, 1024, &gq_tx_gentable);

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
