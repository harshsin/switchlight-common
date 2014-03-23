/****************************************************************
 *
 *        Copyright 2013, Big Switch Networks, Inc.
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
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

#include <icmpa/icmpa_config.h>
#include "icmpa_int.h"

#if ICMPA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static bool print_once = true;

static void
icmpa_clear_portcounters__(ucli_context_t* uc, uint32_t port_no)
{
    if (port_no > MAX_PORTS) return;

    ICMPA_MEMSET(&port_pkt_counters[port_no], 0, 
                 sizeof(icmpa_typecode_packet_counter_t));
}

static void
icmpa_show_portcounters__(ucli_context_t* uc, uint32_t port_no)
{
    icmpa_typecode_packet_counter_t zero = {0};

    if (port_no > MAX_PORTS) return;
    
    if (memcmp(&zero, &port_pkt_counters[port_no], sizeof(zero)) == 0) return; 

    if (print_once == true) {
        ucli_printf(uc,
                    "PORT    OF port number\n"
                    "echo    Echo Requests\n"
                    "ttl     TTL Excedded\n"
                    "frag    Fragmentation Needed\n"
                    "net     Network Unreachable\n"
                    "host    Host Unreachable\n"
                    "port    Port Unreachable\n");

        ucli_printf(uc, "PORT\techo\tttl\tfrag\tnet\thost\tport\n");
        print_once = false;
    }

    ucli_printf(uc, "%d\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64
                "\t%"PRId64"\n", port_no, 
                port_pkt_counters[port_no].icmp_echo_packets, 
                port_pkt_counters[port_no].icmp_time_exceeded_packets,
                port_pkt_counters[port_no].icmp_fragmentation_reqd_packets,
                port_pkt_counters[port_no].icmp_network_unreachable_packets,
                port_pkt_counters[port_no].icmp_host_unreachable_packets,
                port_pkt_counters[port_no].icmp_port_unreachable_packets);
}

static ucli_status_t
icmpa_ucli_ucli__show_counters__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "counters", -1,
                      "$summary#Display the icmp packet counters."
                      "$args#[Port]"); 

    if (!icmpa_is_initialized()) return UCLI_STATUS_E_ERROR;

    if (uc->pargs->count != 1) {
        ucli_printf(uc, "*************DUMPING SYSTEM COUNTERS*************\n");
        ucli_printf(uc, "TOTAL PACKETS RECV'D       : %" PRId64 "\n", 
                    debug_counter_get(&pkt_counters.icmp_total_in_packets));
        ucli_printf(uc, "TOTAL PACKETS SENT         : %" PRId64 "\n",
                    debug_counter_get(&pkt_counters.icmp_total_out_packets));
        ucli_printf(uc, "TOTAL PACKETS PASSED       : %" PRId64 "\n",
                    debug_counter_get(&pkt_counters.icmp_total_passed_packets));
        ucli_printf(uc, "INTERNAL ERRORS            : %" PRId64 "\n",
                    debug_counter_get(&pkt_counters.icmp_internal_errors));
        ucli_printf(uc, "*************END DUMPING INFO********************\n");
    }

    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        icmpa_show_portcounters__(uc, port);
    } else {
        
        for (port = 0; port <= MAX_PORTS; port++) {
            icmpa_show_portcounters__(uc, port);
        }
    }

    print_once = true;
    return UCLI_STATUS_OK;
}

static ucli_status_t
icmpa_ucli_ucli__clear_counters__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "clear", -1,
                      "$summary#Clear the icmp packet counters."
                      "$args#[Port]");
 
    if (!icmpa_is_initialized()) return UCLI_STATUS_E_ERROR;

    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        icmpa_clear_portcounters__(uc, port);
    } else {
        debug_counter_reset(&pkt_counters.icmp_total_in_packets);
        debug_counter_reset(&pkt_counters.icmp_total_out_packets);
        debug_counter_reset(&pkt_counters.icmp_total_passed_packets);
        debug_counter_reset(&pkt_counters.icmp_internal_errors);

        for (port = 0; port <= MAX_PORTS; port++) {
            icmpa_clear_portcounters__(uc, port);
        }
    }    
    
    return UCLI_STATUS_OK;
}

static ucli_status_t
icmpa_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(icmpa)
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f icmpa_ucli_ucli_handlers__[] = 
{
    icmpa_ucli_ucli__show_counters__,
    icmpa_ucli_ucli__clear_counters__,
    icmpa_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
icmpa_ucli_module__ =
    {
        "icmpa_ucli",
        NULL,
        icmpa_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
icmpa_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&icmpa_ucli_module__);
    n = ucli_node_create("icmpa", NULL, &icmpa_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("icmpa"));
    return n;
}

#else
void*
icmpa_ucli_node_create(void)
{
    return NULL;
}
#endif

