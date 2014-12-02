/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sflowa/sflowa_config.h>
#include "sflowa_log.h"
#include "sflowa_int.h"

#if SFLOWA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
sflowa_ucli_ucli__clear_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "clear", 0,
                      "$summary#Clear counters.");

    debug_counter_reset(&sflow_counters.total_in_packets);
    debug_counter_reset(&sflow_counters.total_out_packets);
    debug_counter_reset(&sflow_counters.counter_requests);
    debug_counter_reset(&sflow_counters.port_status_notifications);
    debug_counter_reset(&sflow_counters.port_features_updates);

    return UCLI_STATUS_OK;
}

static ucli_status_t
sflowa_ucli_ucli__show_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "counters", 0,
                      "$summary#Display counters.");

    ucli_printf(uc, "*************DUMPING COUNTERS*************\n");
    ucli_printf(uc, "TOTAL SAMPLES RECV'D       : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.total_in_packets));
    ucli_printf(uc, "TOTAL DATAGRAMS SENT       : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.total_out_packets));
    ucli_printf(uc, "COUNTER REQUESTS           : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.counter_requests));
    ucli_printf(uc, "PORT STATUS NOTIF'S RECV'D : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.port_status_notifications));
    ucli_printf(uc, "PORT FEATURES UPDATES      : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.port_features_updates));
    ucli_printf(uc, "*************END DUMPING INFO********************\n");

    return UCLI_STATUS_OK;
}

static ucli_status_t
sflowa_ucli_ucli__show_collectors__(ucli_context_t* uc)
{
    list_head_t *list = sflow_collectors_list();
    list_links_t *cur;

    UCLI_COMMAND_INFO(uc,
                      "collectors", 0,
                      "$summary#Display sflow collector entries.");

    if (list_empty(list)) return UCLI_STATUS_OK;

    ucli_printf(uc, "IP --> MAC\t\t\t\t PORT\t STATS(tx-pkts: tx-bytes)\n");
    LIST_FOREACH(list, cur) {
        sflow_collector_entry_t *entry = container_of(cur, links,
                                                      sflow_collector_entry_t);
        ucli_printf(uc, "%{ipv4a} --> %{mac}\t %u\t %" PRId64 " : %" PRId64 "\n",
                    entry->key.collector_ip, entry->value.collector_mac.addr,
                    entry->value.collector_udp_dport,
                    entry->stats.tx_packets,entry->stats.tx_bytes);
    }

    return UCLI_STATUS_OK;
}

static ucli_status_t
sflowa_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(sflowa)
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f sflowa_ucli_ucli_handlers__[] =
{
    sflowa_ucli_ucli__clear_counters__,
    sflowa_ucli_ucli__show_counters__,
    sflowa_ucli_ucli__show_collectors__,
    sflowa_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
sflowa_ucli_module__ =
    {
        "sflowa_ucli",
        NULL,
        sflowa_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
sflowa_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&sflowa_ucli_module__);
    n = ucli_node_create("sflowa", NULL, &sflowa_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("sflowa"));
    return n;
}

#else
void*
sflowa_ucli_node_create(void)
{
    return NULL;
}
#endif

