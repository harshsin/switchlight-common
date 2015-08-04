/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * tx port group gentable implementation.
 */

/*
 * Implements "igmp_tx_port_group" gentable.
 * key: name
 * value: port
 */


#include <AIM/aim.h>
#include <indigo/of_state_manager.h>
#include <BigHash/bighash.h>
#include <debug_counter/debug_counter.h>
#include <slshared/slshared_config.h>

#include "igmpa_int.h"
#include "tx_port_group_table.h"


/* hash table supporting lookup by reference */
static bighash_table_t *ht_tx_port_group;

#define TEMPLATE_NAME tx_port_group_hashtable
#define TEMPLATE_OBJ_TYPE tx_port_group_entry_t
#define TEMPLATE_KEY_FIELD key
#define TEMPLATE_ENTRY_FIELD hash_entry
#include <BigHash/bighash_template.h>


/* debug counters */
static debug_counter_t tx_port_group_add_success;
static debug_counter_t tx_port_group_add_failure;
static debug_counter_t tx_port_group_modify_success;
static debug_counter_t tx_port_group_modify_failure;
static debug_counter_t tx_port_group_delete_success;


static indigo_error_t
tx_port_group_parse_key(of_list_bsn_tlv_t *tlvs, tx_port_group_key_t *key)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected name value TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_NAME) {
        indigo_error_t rv = igmpa_parse_name_tlv(&tlv, key->name);
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
tx_port_group_parse_value(of_list_bsn_tlv_t *tlvs, 
                          tx_port_group_value_t *value)
{
    of_object_t tlv;

    if (of_list_bsn_tlv_first(tlvs, &tlv) < 0) {
        AIM_LOG_ERROR("%s: expected port key TLV, instead got end of list",
                      __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&tlv, &value->port_no);
        if (value->port_no > SLSHARED_CONFIG_OF_PORT_MAX) {
            AIM_LOG_ERROR("%s: port %u exceeds maximum %u",
                          __FUNCTION__, value->port_no,
                          SLSHARED_CONFIG_OF_PORT_MAX);
            return INDIGO_ERROR_PARAM;
        }
    } else {
        AIM_LOG_ERROR("%s: expected port key TLV, instead got %s",
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
tx_port_group_add(void *table_priv,
                  of_list_bsn_tlv_t *key_tlvs,
                  of_list_bsn_tlv_t *value_tlvs,
                  void **entry_priv)
{
    indigo_error_t rv;
    tx_port_group_key_t key;
    tx_port_group_value_t value;
    tx_port_group_entry_t *entry;

    memset(key.name, 0, sizeof(key.name));
    rv = tx_port_group_parse_key(key_tlvs, &key);
    if (rv < 0) {
        debug_counter_inc(&tx_port_group_add_failure);
        return rv;
    }

    rv = tx_port_group_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&tx_port_group_add_failure);
        return rv;
    }

    entry = aim_zmalloc(sizeof(*entry));
    entry->key = key;
    entry->value = value;
    tx_port_group_hashtable_insert(ht_tx_port_group, entry);

    *entry_priv = entry;
    debug_counter_inc(&tx_port_group_add_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
tx_port_group_modify(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs,
                     of_list_bsn_tlv_t *value_tlvs)
{
    indigo_error_t rv;
    tx_port_group_value_t value;
    tx_port_group_entry_t *entry = entry_priv;

    rv = tx_port_group_parse_value(value_tlvs, &value);
    if (rv < 0) {
        debug_counter_inc(&tx_port_group_modify_failure);
        return rv;
    }

    entry->value = value;
    debug_counter_inc(&tx_port_group_modify_success);

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
tx_port_group_delete(void *table_priv,
                     void *entry_priv,
                     of_list_bsn_tlv_t *key_tlvs)
{
    tx_port_group_entry_t *entry = entry_priv;

    bighash_remove(ht_tx_port_group, &entry->hash_entry);
    aim_free(entry);
    debug_counter_inc(&tx_port_group_delete_success);

    return INDIGO_ERROR_NONE;
}

static void
tx_port_group_get_stats(void *table_priv,
                        void *entry_priv,
                        of_list_bsn_tlv_t *key_tlvs,
                        of_list_bsn_tlv_t *stats)
{
    /* do nothing */
}

static indigo_core_gentable_t *tx_port_group_gentable;
static const indigo_core_gentable_ops_t tx_port_group_ops = {
    .add = tx_port_group_add,
    .modify = tx_port_group_modify,
    .del = tx_port_group_delete,
    .get_stats = tx_port_group_get_stats,
};


of_port_no_t
igmpa_tx_port_group_lookup(uint16_t table_id, of_object_t *key)
{
    tx_port_group_entry_t *entry;

    if (table_id != indigo_core_gentable_id(tx_port_group_gentable)) {
        AIM_LOG_ERROR("table id %d does not match tx port group gentable %d",
                      table_id,
                      indigo_core_gentable_id(tx_port_group_gentable));
        return OF_PORT_DEST_NONE;
    }

    entry = indigo_core_gentable_lookup(tx_port_group_gentable, key);
    if (entry) {
        return entry->value.port_no;
    } else {
        AIM_LOG_ERROR("tx port group lookup failed");
        return OF_PORT_DEST_NONE;
    }
}


void
igmpa_tx_port_group_stats_show(aim_pvs_t *pvs)
{
    aim_printf(pvs, "tx_port_group_add  %"PRIu64"\n",
               debug_counter_get(&tx_port_group_add_success));
    aim_printf(pvs, "tx_port_group_add_failure  %"PRIu64"\n",
               debug_counter_get(&tx_port_group_add_failure));
    aim_printf(pvs, "tx_port_group_modify  %"PRIu64"\n",
               debug_counter_get(&tx_port_group_modify_success));
    aim_printf(pvs, "tx_port_group_modify_failure  %"PRIu64"\n",
               debug_counter_get(&tx_port_group_modify_failure));
    aim_printf(pvs, "tx_port_group_delete  %"PRIu64"\n",
               debug_counter_get(&tx_port_group_delete_success));
}


void
igmpa_tx_port_group_table_init(void)
{
    ht_tx_port_group = bighash_table_create(1024);
    indigo_core_gentable_register("igmp_tx_port_group",
                                  &tx_port_group_ops, NULL,
                                  8*1024, 1024, &tx_port_group_gentable);

    debug_counter_register(&tx_port_group_add_success,
                           "igmpa.tx_port_group_add",
                           "tx_port_group table entry added");
    debug_counter_register(&tx_port_group_add_failure,
                           "igmpa.tx_port_group_add_failure",
                           "tx_port_group table add unsuccessful");
    debug_counter_register(&tx_port_group_modify_success,
                           "igmpa.tx_port_group_modify",
                           "tx_port_group table entry modified");
    debug_counter_register(&tx_port_group_modify_failure,
                           "igmpa.tx_port_group_modify_failure",
                           "tx_port_group table modify unsuccessful");
    debug_counter_register(&tx_port_group_delete_success,
                           "igmpa.tx_port_group_delete",
                           "tx_port_group table entry deleted");
}

void
igmpa_tx_port_group_table_finish(void)
{
    indigo_core_gentable_unregister(tx_port_group_gentable);
    bighash_table_destroy(ht_tx_port_group, NULL);
}
