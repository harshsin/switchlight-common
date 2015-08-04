/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * rx port group gentable implementation.
 */

/*
 * Implements "igmp_rx_port_group" gentable.
 * key: port
 * value: name
 */

#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared_config.h>

#include "igmpa_int.h"
#include "rx_port_group_table.h"


/* hash table suppporting lookup by port number */
static bighash_table_t *ht_rx_port_group;

#define TEMPLATE_NAME rx_port_group_hashtable
#define TEMPLATE_OBJ_TYPE rx_port_group_entry_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>


/* debug counters */
static debug_counter_t rx_port_group_add_success;
static debug_counter_t rx_port_group_add_failure;
static debug_counter_t rx_port_group_modify_success;
static debug_counter_t rx_port_group_modify_failure;
static debug_counter_t rx_port_group_delete_success;


static indigo_error_t
rx_port_group_parse_key(of_list_bsn_tlv_t *tlvs, rx_port_group_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected port key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&tlv, &key->port_no);
        if (key->port_no > SLSHARED_CONFIG_OF_PORT_MAX) {
            AIM_LOG_ERROR("%s: port %u exceeds maximum %u",
                          __FUNCTION__, key->port_no,
                          SLSHARED_CONFIG_OF_PORT_MAX);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected port key TLV, instead got %s",
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
rx_port_group_parse_value(of_list_bsn_tlv_t *tlvs, 
                          rx_port_group_value_t *value)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected name value TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_NAME) {
        indigo_error_t rv = igmpa_parse_name_tlv(&tlv, value->name);
        if (rv != INDIGO_ERROR_NONE) {
            return rv;
        }
    } else {
        AIM_LOG_ERROR("%s: expected name value TLV, instead got %s",
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
rx_port_group_add(void *table_priv,
                  of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *value_tlvs,
                  void **entry_priv)
{
    indigo_error_t rv;
    rx_port_group_key_t key;
    rx_port_group_value_t value;
    rx_port_group_entry_t *entry;

    rv = rx_port_group_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&rx_port_group_add_failure);
        return rv;
    }

    memset(value.name, 0, sizeof(value.name));
    rv = rx_port_group_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&rx_port_group_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;
    rx_port_group_hashtable_insert(ht_rx_port_group, entry);

    *entry_priv = entry;
    debug_counter_inc(&rx_port_group_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
rx_port_group_modify(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs,
                     of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    rx_port_group_value_t value;
    rx_port_group_entry_t *entry = entry_priv;

    memset(value.name, 0, sizeof(value.name));
    rv = rx_port_group_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&rx_port_group_modify_failure);
        return rv;
    }

    entry->value = value;
    debug_counter_inc(&rx_port_group_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
rx_port_group_delete(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs)
{
    rx_port_group_entry_t *entry = entry_priv;

    bighash_remove(ht_rx_port_group, &entry->hash_entry);
    aim_free(entry);
    debug_counter_inc(&rx_port_group_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
rx_port_group_get_stats(void *table_priv,
                        void *entry_priv,
                        of_list_bsn_tlv_t *key_tlvs,
                        of_list_bsn_tlv_t *stats)
{
    /* do nothing */
}

static indigo_core_gentable_t *rx_port_group_gentable;
static const indigo_core_gentable_ops_t rx_port_group_ops = {
    .add = rx_port_group_add,
    .modify = rx_port_group_modify,
    .del = rx_port_group_delete,
    .get_stats = rx_port_group_get_stats,
};


rx_port_group_entry_t *
igmpa_rx_port_group_lookup(of_port_no_t port_no)
{
    rx_port_group_key_t key = { .port_no = port_no };
    return rx_port_group_hashtable_first(ht_rx_port_group, &key);
}


void
igmpa_rx_port_group_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "rx_port_group_add  %"PRIu64"\n",
               debug_counter_get(&rx_port_group_add_success));
    aim_printf(pvs, "rx_port_group_add_failure  %"PRIu64"\n",
               debug_counter_get(&rx_port_group_add_failure));
    aim_printf(pvs, "rx_port_group_modify  %"PRIu64"\n",
               debug_counter_get(&rx_port_group_modify_success));
    aim_printf(pvs, "rx_port_group_modify_failure  %"PRIu64"\n",
               debug_counter_get(&rx_port_group_modify_failure));
    aim_printf(pvs, "rx_port_group_delete  %"PRIu64"\n",
               debug_counter_get(&rx_port_group_delete_success));
}


void
igmpa_rx_port_group_table_init(void)
{
    ht_rx_port_group = bighash_table_create(1024);
    indigo_core_gentable_register("igmp_rx_port_group",
                                  &rx_port_group_ops, NULL,
                                  8*1024, 1024, &rx_port_group_gentable);

    debug_counter_register(&rx_port_group_add_success,
                           "igmpa.rx_port_group_add",
                           "rx_port_group table entry added");
    debug_counter_register(&rx_port_group_add_failure,
                           "igmpa.rx_port_group_add_failure",
                           "rx_port_group table add unsuccessful");
    debug_counter_register(&rx_port_group_modify_success,
                           "igmpa.rx_port_group_modify",
                           "rx_port_group table entry modified");
    debug_counter_register(&rx_port_group_modify_failure,
                           "igmpa.rx_port_group_modify_failure",
                           "rx_port_group table modify unsuccessful");
    debug_counter_register(&rx_port_group_delete_success,
                           "igmpa.rx_port_group_delete",
                           "rx_port_group table entry deleted");
}

void
igmpa_rx_port_group_table_finish(void)
{
    indigo_core_gentable_unregister(rx_port_group_gentable);
    bighash_table_destroy(ht_rx_port_group, NULL);
}
