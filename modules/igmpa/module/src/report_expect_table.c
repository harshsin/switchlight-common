/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * report expectation gentable implementation.
 */

/*
 * Implements "igmp_report_expectation" gentable.
 * key: name (igmp_rx_port_group), vlan_vid, ipv4 (multicast address) 
 * value: none
 * stats: rx_packets, idle_time
 */


#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <SocketManager/socketmanager.h>
#include <BigHash/bighash.h>
#include <timer_wheel/timer_wheel.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared.h>

#include "igmpa_int.h"
#include "timeout_table.h"
#include "report_expect_table.h"


/* hash table supporting lookup by report expectation key */
static bighash_table_t *ht_report_expect;

#define TEMPLATE_NAME report_expect_hashtable
#define TEMPLATE_OBJ_TYPE report_expect_entry_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

/* timer wheel for sending report idle notifications */
static timer_wheel_t *tw_report_expect;


/* debug counters */
static debug_counter_t report_expect_add_success;
static debug_counter_t report_expect_add_failure;
static debug_counter_t report_expect_modify_success;
static debug_counter_t report_expect_modify_failure;
static debug_counter_t report_expect_delete_success;
static debug_counter_t report_idle_notif;


static void
report_expect_send_idle_notif(report_expect_entry_t *entry)
{
    of_version_t version;
    if (indigo_cxn_get_async_version(&version) < 0) {
        /* no controller connected */
        return;
    } else if (version < OF_VERSION_1_4) {
        /* async notification requires OF 1.4+ */
        return;
    }

    of_object_t *notif = of_bsn_generic_async_new(version);
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(version);

    AIM_LOG_INFO("send report idle notif, rx port group %s vlan %u ipv4 %{ipv4a}",
                 entry->key.name, entry->key.vlan_vid, entry->key.ipv4);

    of_bsn_generic_async_name_set(notif, "igmp_report_idle");
    {
        of_bsn_tlv_name_t *tlv = of_bsn_tlv_name_new(version);
        of_octets_t octets;
        octets.data = (uint8_t*) entry->key.name;
        octets.bytes = strlen(entry->key.name);
        AIM_ASSERT(of_bsn_tlv_name_value_set(tlv, &octets) == 0,
                   "Cannot set idle notification name to %s", entry->key.name);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(version);
        of_bsn_tlv_vlan_vid_value_set(tlv, entry->key.vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_new(version);
        of_bsn_tlv_ipv4_value_set(tlv, entry->key.ipv4);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    AIM_ASSERT(of_bsn_generic_async_tlvs_set(notif, list) == 0,
               "Cannot set idle notification tlvs");
    of_object_delete(list);

    /* send notification to controller */
    indigo_cxn_send_async_message(notif);
}

/*
 * handles expired timeouts for expected reports from hosts;
 * default: 255 seconds
 */
static void
report_expect_timer(void *cookie)
{
    timer_wheel_entry_t *timer;
    indigo_time_t now = INDIGO_CURRENT_TIME;
    int notifs = 0;
    const int max_notifs = 32;

    while (notifs < max_notifs &&
           !ind_soc_should_yield() &&
           (timer = timer_wheel_next(tw_report_expect, now))) {
        report_expect_entry_t *entry = 
            container_of(timer, timer_entry, report_expect_entry_t);

        report_expect_send_idle_notif(entry);
        notifs++;
        timer_wheel_insert(tw_report_expect, &entry->timer_entry,
                           now + igmpa_report_expect_timeout);
        debug_counter_inc(&report_idle_notif);
    }

    /* reregister sooner if more expired entries */
    if (timer_wheel_peek(tw_report_expect, now)) {
        ind_soc_timer_event_register(report_expect_timer, NULL, 100);
    } else {
        ind_soc_timer_event_register(report_expect_timer, NULL, 1000);
    }
}


void
igmpa_report_expect_reschedule(report_expect_entry_t *report_expect_entry,
                               uint64_t new_deadline)
{
    timer_wheel_remove(tw_report_expect, &report_expect_entry->timer_entry);
    timer_wheel_insert(tw_report_expect, &report_expect_entry->timer_entry,
                       new_deadline);
}


/*----------*/
/* gentable handling */

static indigo_error_t
report_expect_parse_key(of_list_bsn_tlv_t *tlvs, report_expect_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected name key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_NAME) {
        indigo_error_t rv = igmpa_parse_name_tlv(&tlv, key->name);
        if (rv != INDIGO_ERROR_NONE) {
            return rv;
        }
    } else {
        AIM_LOG_ERROR("%s: expected name key TLV, instead got %s",
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
report_expect_parse_value(of_list_bsn_tlv_t *tlvs)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of value list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_expect_add(void *table_priv,
                  of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *value_tlvs,
                  void **entry_priv)
{
    indigo_error_t rv;
    report_expect_key_t key;
    report_expect_entry_t *entry;

    memset(&key, 0, sizeof(key));
    rv = report_expect_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&report_expect_add_failure);
        return rv;
    }

    rv = report_expect_parse_value(value_tlvs);
    if (rv < 0) {
        debug_counter_inc(&report_expect_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    report_expect_hashtable_insert(ht_report_expect, entry);

    timer_wheel_insert(tw_report_expect, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_report_expect_timeout);

    *entry_priv = entry;
    debug_counter_inc(&report_expect_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_expect_modify(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs,
                     of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;

    rv = report_expect_parse_value(value_tlvs);
    if (rv < 0) {
        debug_counter_inc(&report_expect_modify_failure);
        return rv;
    }

    /* no values to modify */

    debug_counter_inc(&report_expect_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
report_expect_delete(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs)
{
    report_expect_entry_t *entry = entry_priv;

    timer_wheel_remove(tw_report_expect, &entry->timer_entry);
    bighash_remove(ht_report_expect, &entry->hash_entry);
    aim_free(entry);
    debug_counter_inc(&report_expect_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
report_expect_get_stats(void *table_priv,
                        void *entry_priv,
                        of_list_bsn_tlv_t *key_tlvs,
                        of_list_bsn_tlv_t *stats)
{
    report_expect_entry_t *entry = entry_priv;

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
        of_bsn_tlv_rx_packets_value_set(&tlv, entry->rx_packets);
    }    
}

static indigo_core_gentable_t *report_expect_gentable;
static const indigo_core_gentable_ops_t report_expect_ops = {
    .add = report_expect_add,
    .modify = report_expect_modify,
    .del = report_expect_delete,
    .get_stats = report_expect_get_stats,
};


report_expect_entry_t *
igmpa_report_expect_lookup(char name[], uint16_t vlan_vid, uint32_t ipv4)
{
    report_expect_key_t key;
    memset(&key, 0, sizeof(key));
    memcpy(key.name, name, sizeof(key.name));
    key.vlan_vid = vlan_vid;
    key.ipv4 = ipv4;
    return report_expect_hashtable_first(ht_report_expect, &key);
}


void
igmpa_report_expect_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "report_expect_add  %"PRIu64"\n",
               debug_counter_get(&report_expect_add_success));
    aim_printf(pvs, "report_expect_add_failure  %"PRIu64"\n",
               debug_counter_get(&report_expect_add_failure));
    aim_printf(pvs, "report_expect_modify  %"PRIu64"\n",
               debug_counter_get(&report_expect_modify_success));
    aim_printf(pvs, "report_expect_modify_failure  %"PRIu64"\n",
               debug_counter_get(&report_expect_modify_failure));
    aim_printf(pvs, "report_expect_delete  %"PRIu64"\n",
               debug_counter_get(&report_expect_delete_success));
    aim_printf(pvs, "report_idle_notif  %"PRIu64"\n",
               debug_counter_get(&report_idle_notif));
}


void
igmpa_report_expect_table_init(void)
{
    indigo_error_t rv;

    ht_report_expect = bighash_table_create(1024);

    /* from host: 16k entries, 255s timeout for expectations */
    tw_report_expect = timer_wheel_create(16*1024, 256, INDIGO_CURRENT_TIME);

    rv = ind_soc_timer_event_register(report_expect_timer, NULL, 1000);
    AIM_ASSERT(rv == INDIGO_ERROR_NONE,
               "Failed to register report expiration timer: %s",
               indigo_strerror(rv));

    indigo_core_gentable_register("igmp_report_expectation",
                                  &report_expect_ops, NULL,
                                  8*1024, 1024, &report_expect_gentable);

    debug_counter_register(&report_expect_add_success,
                           "igmpa.report_expect_add",
                           "report_expect table entry added");
    debug_counter_register(&report_expect_add_failure,
                           "igmpa.report_expect_add_failure",
                           "report_expect table add unsuccessful");
    debug_counter_register(&report_expect_modify_success,
                           "igmpa.report_expect_modify",
                           "report_expect table entry modified");
    debug_counter_register(&report_expect_modify_failure,
                           "igmpa.report_expect_modify_failure",
                           "report_expect table modify unsuccessful");
    debug_counter_register(&report_expect_delete_success,
                           "igmpa.report_expect_delete",
                           "report_expect table entry deleted");
    debug_counter_register(&report_idle_notif, "igmpa.report_idle_notif",
                           "IGMP report idle notification sent");
}

void
igmpa_report_expect_table_finish(void)
{
    indigo_core_gentable_unregister(report_expect_gentable);
    ind_soc_timer_event_unregister(report_expect_timer, NULL);
    timer_wheel_destroy(tw_report_expect);
    bighash_table_destroy(ht_report_expect, NULL);
}
