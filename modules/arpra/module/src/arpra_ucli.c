/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arpra/arpra_config.h>
#include "arpra_int.h"

#if ARPRA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
arpra_ucli_ucli__clear_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "clear", 0,
                      "$summary#Clear packet counters.");

    if (!arpra_is_initialized()) return UCLI_STATUS_E_ERROR;

    debug_counter_reset(&pkt_counters.total_in_packets);
    debug_counter_reset(&pkt_counters.total_out_packets);
    debug_counter_reset(&pkt_counters.internal_errors);
    
    return UCLI_STATUS_OK;
}

static ucli_status_t
arpra_ucli_ucli__show_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "counters", 0,
                      "$summary#Display packet counters.");

    if (!arpra_is_initialized()) return UCLI_STATUS_E_ERROR;

    ucli_printf(uc, "*************DUMPING PACKET COUNTERS*************\n");
    ucli_printf(uc, "TOTAL PACKETS RECV'D       : %" PRId64 "\n",
                debug_counter_get(&pkt_counters.total_in_packets));
    ucli_printf(uc, "TOTAL PACKETS SENT         : %" PRId64 "\n",
                debug_counter_get(&pkt_counters.total_out_packets));
    ucli_printf(uc, "INTERNAL ERRORS            : %" PRId64 "\n",
                debug_counter_get(&pkt_counters.internal_errors));
    ucli_printf(uc, "*************END DUMPING INFO********************\n");

    return UCLI_STATUS_OK;
}

static ucli_status_t
arpra_ucli_ucli__show_cache__(ucli_context_t* uc)
{
    list_head_t *cache = arp_cache_list();
    list_links_t *cur;

    UCLI_COMMAND_INFO(uc,
                      "cache", 0,
                      "$summary#Display ARP cache entries.");

    if (!arpra_is_initialized()) return UCLI_STATUS_E_ERROR;

    if (list_empty(cache)) return UCLI_STATUS_OK;

    ucli_printf(uc, "IP --> MAC\t\t\t\t Refcount\n");
    LIST_FOREACH(cache, cur) {
        arp_cache_entry_t *cache_entry = container_of(cur, links,
                                                      arp_cache_entry_t);
        ucli_printf(uc, "%{ipv4a} --> %{mac}\t %d\n", 
                    cache_entry->entry.ipv4, cache_entry->entry.mac.addr,
                    cache_entry->refcount); 
    }

    return UCLI_STATUS_OK;
}

static ucli_status_t
arpra_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(arpra)
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f arpra_ucli_ucli_handlers__[] = 
{
    arpra_ucli_ucli__clear_counters__,
    arpra_ucli_ucli__show_counters__,
    arpra_ucli_ucli__show_cache__,
    arpra_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
arpra_ucli_module__ =
    {
        "arpra_ucli",
        NULL,
        arpra_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
arpra_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&arpra_ucli_module__);
    n = ucli_node_create("arpra", NULL, &arpra_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("arpra"));
    return n;
}

#else
void*
arpra_ucli_node_create(void)
{
    return NULL;
}
#endif
