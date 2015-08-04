/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * timeout gentable implementation.
 */

/*
 * Implements "igmp_timeout" gentable.
 * key: name
 * value: interval
 */


#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <debug_counter/debug_counter.h>

#include "igmpa_int.h"
#include "timeout_table.h"


/* debug counters */
static debug_counter_t timeout_add_success;
static debug_counter_t timeout_add_failure;
static debug_counter_t timeout_modify_success;
static debug_counter_t timeout_modify_failure;
static debug_counter_t timeout_delete_success;


/* timeouts are in milliseconds */
#define GQ_EXPECT_TIMEOUT_DEFAULT (255*1000)
#define REPORT_EXPECT_TIMEOUT_DEFAULT (255*1000)
#define GQ_TX_TIMEOUT_DEFAULT (125*1000)
#define REPORT_TX_TIMEOUT_DEFAULT (125*1000)

uint32_t igmpa_gq_expect_timeout = GQ_EXPECT_TIMEOUT_DEFAULT;
uint32_t igmpa_report_expect_timeout = REPORT_EXPECT_TIMEOUT_DEFAULT;
uint32_t igmpa_gq_tx_timeout = GQ_TX_TIMEOUT_DEFAULT;
uint32_t igmpa_report_tx_timeout = REPORT_TX_TIMEOUT_DEFAULT;



/* crappy code to verify name is valid */
static indigo_error_t
validate_name(char *name)
{
    indigo_error_t rv = INDIGO_ERROR_PARAM;

    if (!strncmp(name, "general_query_expectation", IGMP_NAME_LEN)) {
        rv = INDIGO_ERROR_NONE;
    } else if (!strncmp(name, "report_expectation", IGMP_NAME_LEN)) {
        rv = INDIGO_ERROR_NONE;
    } else if (!strncmp(name, "general_query_tx", IGMP_NAME_LEN)) {
        rv = INDIGO_ERROR_NONE;
    } else if (!strncmp(name, "report_tx", IGMP_NAME_LEN)) {
        rv = INDIGO_ERROR_NONE;
    }

    return rv;
}

/* crappy code to update the right global timeout value */
/* timeout_val of zero means reset to default */
static void
update_timeout(char *name, uint32_t timeout_val)
{
    if (!strncmp(name, "general_query_expectation", IGMP_NAME_LEN)) {
        igmpa_gq_expect_timeout = timeout_val? timeout_val : 
            GQ_EXPECT_TIMEOUT_DEFAULT;
    } else if (!strncmp(name, "report_expectation", IGMP_NAME_LEN)) {
        igmpa_report_expect_timeout = timeout_val? timeout_val : 
            REPORT_EXPECT_TIMEOUT_DEFAULT;
    } else if (!strncmp(name, "general_query_tx", IGMP_NAME_LEN)) {
        igmpa_gq_tx_timeout = timeout_val? timeout_val : 
            GQ_TX_TIMEOUT_DEFAULT;
    } else if (!strncmp(name, "report_tx", IGMP_NAME_LEN)) {
        igmpa_report_tx_timeout = timeout_val? timeout_val : 
            REPORT_TX_TIMEOUT_DEFAULT;
    } else {
        AIM_DIE("unhandled timeout %s", name);
    }
}

static indigo_error_t
timeout_parse_key(of_list_bsn_tlv_t *tlvs, timeout_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected name key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_NAME) {
        indigo_error_t rv;
        rv = igmpa_parse_name_tlv(&tlv, key->name);
        if (rv != INDIGO_ERROR_NONE) {
            return rv;
        }
        rv = validate_name(key->name);
        if (rv != INDIGO_ERROR_NONE) {
            AIM_LOG_ERROR("%s: invalid name %s",
                          __FUNCTION__, key->name);
            return rv;
        }
    } else {
        AIM_LOG_ERROR("%s: expected name value TLV, instead got %s",
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
timeout_parse_value(of_list_bsn_tlv_t *tlvs, 
                    timeout_value_t *value)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected value TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_INTERVAL) {
        of_bsn_tlv_interval_value_get(&tlv, &value->timeout);
    } else {
        AIM_LOG_ERROR("%s: expected interval value TLV, instead got %s",
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
timeout_add(void *table_priv,
            of_list_bsn_tlv_t *key_tlvs,
            of_list_bsn_tlv_t *value_tlvs,
            void **entry_priv)
{
    indigo_error_t rv;
    timeout_key_t key;
    timeout_value_t value;
    timeout_entry_t *entry;

    memset(key.name, 0, sizeof(key.name));
    rv = timeout_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&timeout_add_failure);
        return rv;
    }

    rv = timeout_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&timeout_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;

    update_timeout(key.name, entry->value.timeout);

    *entry_priv = entry;
    debug_counter_inc(&timeout_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
timeout_modify(void *table_priv,
               void *entry_priv,
               of_list_bsn_tlv_t *key_tlvs,
               of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    timeout_value_t value;
    timeout_entry_t *entry = entry_priv;

    rv = timeout_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&timeout_modify_failure);
        return rv;
    }

    entry->value = value;

    update_timeout(entry->key.name, entry->value.timeout);

    debug_counter_inc(&timeout_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
timeout_delete(void *table_priv,
               void *entry_priv,
               of_list_bsn_tlv_t *key_tlvs)
{
    timeout_entry_t *entry = entry_priv;

    update_timeout(entry->key.name, 0);
    aim_free(entry);

    debug_counter_inc(&timeout_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
timeout_get_stats(void *table_priv,
                  void *entry_priv,
                  of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *stats)
{
    /* do nothing */
}

static indigo_core_gentable_t *timeout_gentable;
static const indigo_core_gentable_ops_t timeout_ops = {
    .add = timeout_add,
    .modify = timeout_modify,
    .del = timeout_delete,
    .get_stats = timeout_get_stats,
};


void
igmpa_timeout_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "timeout_add  %"PRIu64"\n",
               debug_counter_get(&timeout_add_success));
    aim_printf(pvs, "timeout_add_failure  %"PRIu64"\n",
               debug_counter_get(&timeout_add_failure));
    aim_printf(pvs, "timeout_modify  %"PRIu64"\n",
               debug_counter_get(&timeout_modify_success));
    aim_printf(pvs, "timeout_modify_failure  %"PRIu64"\n",
               debug_counter_get(&timeout_modify_failure));
    aim_printf(pvs, "timeout_delete  %"PRIu64"\n",
               debug_counter_get(&timeout_delete_success));
}


void
igmpa_timeout_table_init(void)
{
    indigo_core_gentable_register("igmp_timeout",
                                  &timeout_ops, NULL,
                                  16, 16, &timeout_gentable);

    debug_counter_register(&timeout_add_success,
                           "igmpa.timeout_add",
                           "timeout table entry added");
    debug_counter_register(&timeout_add_failure,
                           "igmpa.timeout_add_failure",
                           "timeout table add unsuccessful");
    debug_counter_register(&timeout_modify_success,
                           "igmpa.timeout_modify",
                           "timeout table entry modified");
    debug_counter_register(&timeout_modify_failure,
                           "igmpa.timeout_modify_failure",
                           "timeout table modify unsuccessful");
    debug_counter_register(&timeout_delete_success,
                           "igmpa.timeout_delete",
                           "timeout table entry deleted");
}

void
igmpa_timeout_table_finish(void)
{
    indigo_core_gentable_unregister(timeout_gentable);
}
