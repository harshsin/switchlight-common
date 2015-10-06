/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * PIM expectation gentable implementation.
 */

/*
 * Implements "pim_expectation" gentable.
 * key: name (igmp_rx_port_group), vlan_vid
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
#include "pim_expect_table.h"


/* hash table supporting lookup by pim expectation key */
static bighash_table_t *ht_pim_expect;

#define TEMPLATE_NAME pim_expect_hashtable
#define TEMPLATE_OBJ_TYPE pim_expect_entry_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>

/* timer wheel for sending pim idle notifications */
static timer_wheel_t *tw_pim_expect;


/* debug counters */
DEBUG_COUNTER(pim_expect_add_success, "igmpa.pim_expect_add",
              "pim_expect table entry added");
DEBUG_COUNTER(pim_expect_add_failure, "igmpa.pim_expect_add_failure",
              "pim_expect table add unsuccessful");
DEBUG_COUNTER(pim_expect_modify_success, "igmpa.pim_expect_modify",
              "pim_expect table entry modified");
DEBUG_COUNTER(pim_expect_modify_failure, "igmpa.pim_expect_modify_failure",
              "pim_expect table modify unsuccessful");
DEBUG_COUNTER(pim_expect_delete_success, "igmpa.pim_expect_delete",
              "pim_expect table entry deleted");
DEBUG_COUNTER(pim_idle_notif, "igmpa.pim_idle_notif",
              "PIM idle notification sent");


static void
pim_expect_send_idle_notif(pim_expect_entry_t *entry)
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

    AIM_LOG_VERBOSE("send pim idle notif, rx port group %s vlan %u",
                    entry->key.name, entry->key.vlan_vid);

    of_bsn_generic_async_name_set(notif, "pim_idle");
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
    AIM_ASSERT(of_bsn_generic_async_tlvs_set(notif, list) == 0,
               "Cannot set idle notification tlvs");
    of_object_delete(list);

    /* send notification to controller */
    indigo_cxn_send_async_message(notif);
    debug_counter_inc(&pim_idle_notif);
}


/*
 * handles expired timeouts for expected PIM hellos from mrouter;
 * default: 60 seconds
 */
static void
pim_expect_timer(void *cookie)
{
    timer_wheel_entry_t *timer;
    indigo_time_t now = INDIGO_CURRENT_TIME;
    int notifs = 0;
    const int max_notifs = 32;

    while (notifs < max_notifs &&
           !ind_soc_should_yield() &&
           (timer = timer_wheel_next(tw_pim_expect, now))) {
        pim_expect_entry_t *entry = 
            container_of(timer, timer_entry, pim_expect_entry_t);

        pim_expect_send_idle_notif(entry);
        notifs++;
        timer_wheel_insert(tw_pim_expect, &entry->timer_entry,
                           now + igmpa_pim_expect_timeout);
    }

    /* reregister sooner if more expired entries */
    if (timer_wheel_peek(tw_pim_expect, now)) {
        ind_soc_timer_event_register(pim_expect_timer, NULL, 100);
    } else {
        ind_soc_timer_event_register(pim_expect_timer, NULL, 1000);
    }
}


void igmpa_pim_expect_reschedule(pim_expect_entry_t *pim_expect_entry,
                                 uint64_t new_deadline)
{
    timer_wheel_remove(tw_pim_expect, &pim_expect_entry->timer_entry);
    timer_wheel_insert(tw_pim_expect, &pim_expect_entry->timer_entry,
                       new_deadline);
}


/*----------*/
/* gentable handling */

static indigo_error_t
pim_expect_parse_key(of_list_bsn_tlv_t *tlvs, pim_expect_key_t *key)
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

    if (of_list_bsn_tlv_next(tlvs, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of key TLV list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
pim_expect_parse_value(of_list_bsn_tlv_t *tlvs)
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
pim_expect_add(void *table_priv,
               of_list_bsn_tlv_t *key_tlvs,
               of_list_bsn_tlv_t *value_tlvs,
               void **entry_priv)
{
    indigo_error_t rv;
    pim_expect_key_t key;
    pim_expect_entry_t *entry;

    IGMPA_MEMSET(&key, 0, sizeof(key));
    rv = pim_expect_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&pim_expect_add_failure);
        return rv;
    }

    rv = pim_expect_parse_value(value_tlvs);
    if (rv < 0) {
        debug_counter_inc(&pim_expect_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    pim_expect_hashtable_insert(ht_pim_expect, entry);

    timer_wheel_insert(tw_pim_expect, &entry->timer_entry, 
                       INDIGO_CURRENT_TIME + igmpa_pim_expect_timeout);

    *entry_priv = entry;
    debug_counter_inc(&pim_expect_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
pim_expect_modify(void *table_priv,
                  void *entry_priv,
                  of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;

    rv = pim_expect_parse_value(value_tlvs);
    if (rv < 0) {
        debug_counter_inc(&pim_expect_modify_failure);
        return rv;
    }

    /* no values to modify */

    debug_counter_inc(&pim_expect_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
pim_expect_delete(void *table_priv,
                  void *entry_priv,
                  of_list_bsn_tlv_t *key_tlvs)
{
    pim_expect_entry_t *entry = entry_priv;

    timer_wheel_remove(tw_pim_expect, &entry->timer_entry);
    bighash_remove(ht_pim_expect, &entry->hash_entry);
    aim_free(entry);
    debug_counter_inc(&pim_expect_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
pim_expect_get_stats(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs,
                     of_list_bsn_tlv_t *stats)
{
    pim_expect_entry_t *entry = entry_priv;

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

static indigo_core_gentable_t *pim_expect_gentable;
static const indigo_core_gentable_ops_t pim_expect_ops = {
    .add = pim_expect_add,
    .modify = pim_expect_modify,
    .del = pim_expect_delete,
    .get_stats = pim_expect_get_stats,
};


pim_expect_entry_t *
igmpa_pim_expect_lookup(char name[], uint16_t vlan_vid)
{
    pim_expect_key_t key;
    IGMPA_MEMSET(&key, 0, sizeof(key));
    IGMPA_MEMCPY(key.name, name, sizeof(key.name));
    key.vlan_vid = vlan_vid;
    return pim_expect_hashtable_first(ht_pim_expect, &key);
}


void
igmpa_pim_expect_stats_clear(void)
{
    debug_counter_reset(&pim_expect_add_success);
    debug_counter_reset(&pim_expect_add_failure);
    debug_counter_reset(&pim_expect_modify_success);
    debug_counter_reset(&pim_expect_modify_failure);
    debug_counter_reset(&pim_expect_delete_success);
    debug_counter_reset(&pim_idle_notif);
}


void
igmpa_pim_expect_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "pim_expect_add  %"PRIu64"\n",
               debug_counter_get(&pim_expect_add_success));
    aim_printf(pvs, "pim_expect_add_failure  %"PRIu64"\n",
               debug_counter_get(&pim_expect_add_failure));
    aim_printf(pvs, "pim_expect_modify  %"PRIu64"\n",
               debug_counter_get(&pim_expect_modify_success));
    aim_printf(pvs, "pim_expect_modify_failure  %"PRIu64"\n",
               debug_counter_get(&pim_expect_modify_failure));
    aim_printf(pvs, "pim_expect_delete  %"PRIu64"\n",
               debug_counter_get(&pim_expect_delete_success));
    aim_printf(pvs, "pim_idle_notif  %"PRIu64"\n",
               debug_counter_get(&pim_idle_notif));
}


void
igmpa_pim_expect_table_init(void)
{
    indigo_error_t rv;

    ht_pim_expect = bighash_table_create(1024);

    /* from mrouter: 1k entries, 60s timeout for expectations */
    /* bucket size is (60*1000)ms / 1024 entries ~= 64 */
    tw_pim_expect = timer_wheel_create(1024, 64, INDIGO_CURRENT_TIME);

    rv = ind_soc_timer_event_register(pim_expect_timer, NULL, 1000);
    AIM_ASSERT(rv == INDIGO_ERROR_NONE,
               "Failed to register pim expiration timer: %s",
               indigo_strerror(rv));

    indigo_core_gentable_register("pim_expectation",
                                  &pim_expect_ops, NULL,
                                  8*1024, 1024, &pim_expect_gentable);
}

void
igmpa_pim_expect_table_finish(void)
{
    indigo_core_gentable_unregister(pim_expect_gentable);
    ind_soc_timer_event_unregister(pim_expect_timer, NULL);
    timer_wheel_destroy(tw_pim_expect);
    bighash_table_destroy(ht_pim_expect, NULL);
}
