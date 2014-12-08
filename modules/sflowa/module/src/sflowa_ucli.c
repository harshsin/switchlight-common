/****************************************************************
 *
 *        Copyright 2014, Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the shard
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

#include <sflowa/sflowa_config.h>
#include "sflowa_log.h"
#include "sflowa_int.h"

#if SFLOWA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static bool print_once = true;

static ucli_status_t
sflowa_ucli_ucli__clear_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "clear", 0,
                      "$summary#Clear counters.");

    debug_counter_reset(&sflow_counters.packet_in);
    debug_counter_reset(&sflow_counters.packet_out);
    debug_counter_reset(&sflow_counters.counter_request);
    debug_counter_reset(&sflow_counters.port_status_notification);
    debug_counter_reset(&sflow_counters.port_features_update);

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
                debug_counter_get(&sflow_counters.packet_in));
    ucli_printf(uc, "TOTAL DATAGRAMS SENT       : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.packet_out));
    ucli_printf(uc, "COUNTER REQUESTS           : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.counter_request));
    ucli_printf(uc, "PORT STATUS NOTIF'S RECV'D : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.port_status_notification));
    ucli_printf(uc, "PORT FEATURES UPDATES      : %" PRId64 "\n",
                debug_counter_get(&sflow_counters.port_features_update));
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

    ucli_printf(uc, "IP\t --> MAC\t\t PORT\t STATS(tx-pkts: tx-bytes)\n");
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

static void
sflowa_show_portattributes__(ucli_context_t* uc, uint32_t port_no)
{
    if (port_no > SFLOWA_CONFIG_OF_PORTS_MAX) return;

    /*
     * Don't print anything for ports not sampling/polling
     */
    if (sampler_entries[port_no].value.sampling_rate == 0 &&
        sampler_entries[port_no].value.polling_interval == 0) {
        return;
    }

    if (print_once == true) {
        ucli_printf(uc,
                    "PORT      OF port number\n"
                    "rate      Ingress sampling rate\n"
                    "size      Header size(bytes)\n"
                    "poll      Polling interval(sec)\n"
                    "speed     Intf bandwidth(bits/sec)\n"
                    "direction 0 = Unknown, 1 = FD, 2 = HD\n"
                    "admin     Administrative status\n"
                    "link      Operational status\n"
                    "pkt_in    Num of sampled packet_ins\n");
        ucli_printf(uc, "PORT\trate\tsize\tpoll\tspeed\t\tdirection\t"
                    "admin\tlink\tpkt_in\n");
        print_once = false;
    }

    ucli_printf(uc, "%u\t%u\t%u\t%u\t%"PRId64"\t%u\t\t%s\t%s\t%"PRId64"\n",
                port_no, sampler_entries[port_no].value.sampling_rate,
                sampler_entries[port_no].value.header_size,
                sampler_entries[port_no].value.polling_interval,
                port_features[port_no].speed, port_features[port_no].direction,
                port_features[port_no].status & IF_ADMIN_UP? "up":"down",
                port_features[port_no].status & IF_OPER_UP? "up":"down",
                sampler_entries[port_no].stats.rx_packets);
}

static ucli_status_t
sflowa_ucli_ucli__show_portattributes__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "port_attributes", -1,
                      "$summary#Display the sflow attributes per port."
                      "$args#[Port]");

    print_once = true;

    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        sflowa_show_portattributes__(uc, port);
    } else {
        for (port = 0; port <= SFLOWA_CONFIG_OF_PORTS_MAX; port++) {
            sflowa_show_portattributes__(uc, port);
        }
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
    sflowa_ucli_ucli__show_portattributes__,
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

