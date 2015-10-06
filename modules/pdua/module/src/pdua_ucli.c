/****************************************************************
 *
 *        Copyright 2015, Big Switch Networks, Inc.
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
#include <pdua/pdua_config.h>

#if PDUA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

#include "pdua_int.h"
extern pdua_system_t pdua_port_sys;
extern int           pdua_dump_data;
uint32_t dummy_test_data[] = {0xdeafbeef, 0x12345678, 0xdeafbeef};
ucli_context_t* ucg;

/* must assign ucg before using this function */
static void
ucli_data_display (char *str) {
    if (ucg)
        ucli_printf(ucg, "%s\n", str);
}

static ucli_status_t
pdua_ucli_ucli__show_pdua_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "sys_cnts", 0,
                      "$summary#Display the pdua system packet counters.");

    ucli_printf(uc, "TOTAL PDUA PORT NUMBER : %u\n",
                pdua_port_sys.pdua_total_of_ports);
    ucli_printf(uc, "TOTAL PACKETS RECV    : %" PRId64 "\n",
                pdua_port_sys.total_pkt_in_cnt);
    ucli_printf(uc, "TOTAL PACKETS EXPECTED    : %" PRId64 "\n",
                pdua_port_sys.total_pkt_exp_cnt);
    ucli_printf(uc, "TOTAL MESSAGES RECV    : %" PRId64 "\n",
                pdua_port_sys.total_msg_in_cnt);

    return UCLI_STATUS_OK;
}

static ucli_status_t
pdua_ucli_ucli__clear_pdua_counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "clr_sys_cnts", 0,
                      "$summary#Clear the pdua system packet counters.");

    pdua_port_sys.total_pkt_in_cnt  = 0;
    pdua_port_sys.total_pkt_exp_cnt = 0;
    pdua_port_sys.total_msg_in_cnt  = 0;

    return UCLI_STATUS_OK;
}

static void
pdua_show_portcounters__(ucli_context_t* uc, uint32_t port_no)
{
    pdua_port_t  *port = NULL;

    /*
     * Find any port corresponding to the info received
     */
    port = pdua_find_port(port_no);
    if (!port) return;
 
    ucli_printf(uc, "%d\t%d\t%d\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\n" , 
                port->port_no, port->rx_pkt.interval_ms, port->tx_pkt.interval_ms,
                port->rx_pkt_in_cnt, port->tx_pkt_out_cnt, port->timeout_pkt_cnt,
                port->rx_pkt_mismatched_no_data, port->rx_pkt_mismatched_len,
                port->rx_pkt_mismatched_data, port->rx_pkt_matched,
                port->tx_req_cnt, port->rx_req_cnt);

}

static ucli_status_t
pdua_ucli_ucli__show_pdua_portcounters__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "port_cnts", -1,
                      "$summary#Display the pdua counters per port."
                      "$args#[Port]");

    ucli_printf(uc, 
                "PORT    OF port number\n"
                "r_intv  Rx time interval\n"
                "t_intv  Tx time interval\n"
                "pkt_in  Num of packet_ins fr the data plane\n"
                "pk_out  Num of packet_outs to the data plane\n"
                "TOmsg   Num of timeout_msgs to the control plane\n"
                "MM_ND   Mismatched due to no data\n"
                "MM_len  Mismatched due to len\n"
                "MM_data Mismatched due to data\n"
                "Matchd  Data matched\n"
                "txReq   Num of tx req fr the control plane\n"
                "rxReq   Num of rx req fr the control plane\n");

    ucli_printf(uc, "PORT\tr_intv\tt_intv\tpkt_in\tpk_out\tTOmsg\tMM_ND\tMM_len\tMM_data\tMATCHD\ttxReq\trxReq\n");
    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        pdua_show_portcounters__(uc, port);
    } else {
        for (port = 0; port < pdua_port_sys.pdua_total_of_ports; port++) {
            pdua_show_portcounters__(uc, port);
        }
    }
    ucli_printf(uc, "**************END DUMPING PORT INFO************\n");
    return UCLI_STATUS_OK;
}

static void
pdua_clear_portcounters__(ucli_context_t* uc, uint32_t port_no)
{
    pdua_port_t  *port = NULL;

    /*
     * Find any port corresponding to the info received
     */
    port = pdua_find_port(port_no);
    if (!port) return;
 
    port->rx_pkt_in_cnt   = 0;
    port->tx_pkt_out_cnt  = 0;
    port->timeout_pkt_cnt = 0;
    port->rx_pkt_mismatched_no_data = 0;
    port->rx_pkt_mismatched_len = 0;
    port->rx_pkt_mismatched_data = 0;
    port->rx_pkt_matched = 0;
    port->tx_req_cnt = 0;
    port->rx_req_cnt = 0;
}

static ucli_status_t
pdua_ucli_ucli__clear_pdua_portcounters__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "clr_port_cnts", -1,
                      "$summary#Clear the pdua counters per port."
                      "$args#[Port]");

    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        pdua_clear_portcounters__(uc, port);
    } else {
        for (port = 0; port < pdua_port_sys.pdua_total_of_ports; port++) {
            pdua_clear_portcounters__(uc, port);
        }
    }
    return UCLI_STATUS_OK;
}

static void
pdua_show_portdata__(ucli_context_t* uc, uint32_t port_no)
{
    pdua_port_t  *port = NULL;

    /*
     * Find any port corresponding to the info received
     */
    port = pdua_find_port(port_no);
    if (!port) return;

    ucli_printf(uc, "PORT:%d\n", port_no);

    ucg = uc;
    if (port->tx_pkt.data.data) {
        ucli_printf(uc, "TX:\n"); 
        pdua_data_hexdump(port->tx_pkt.data.data,
                           port->tx_pkt.data.bytes,
                           ucli_data_display);
    }
    if (port->rx_pkt.data.data) {
        ucli_printf(uc, "RX:\n");
        pdua_data_hexdump(port->rx_pkt.data.data,
                           port->rx_pkt.data.bytes,
                           ucli_data_display );
    }
}

static ucli_status_t
pdua_ucli_ucli__show_pdua_portdata__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "port_data", -1,
                      "$summary#Display the data per port."
                      "$args#[Port]");

    ucli_printf(uc, "START DUMPING DATA PORT INFO\n");
    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        pdua_show_portdata__(uc, port);
    } else {
        for (port = 0; port < pdua_port_sys.pdua_total_of_ports; port++) {
            pdua_show_portdata__(uc, port);
        }
    }
     ucli_printf(uc, "**************END DUMPING DATA PORT INFO************\n");
    return UCLI_STATUS_OK;
}

static ucli_status_t
pdua_ucli_ucli__set_pkt_hexdump__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "port_dump", -1,
                      "$summary#Set pkt_data hexdump (used with trace)"
                      "$args#[Port]");

    ucg = uc;
    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        ucli_printf(uc, "Enable pkt_data_dump for port %d\n", port);
        pdua_dump_data = port;
        
    } else {
        ucli_printf(uc, "Toggle pkt_data_dump switch\n");
        if (pdua_dump_data == PDUA_DUMP_DISABLE_ALL_PORTS) {
            ucli_printf(uc, "Enable pkt_data_dump for all ports - test data hexdump\n");
            pdua_dump_data = PDUA_DUMP_ENABLE_ALL_PORTS;
            pdua_data_hexdump((uint8_t *)dummy_test_data,
                               sizeof(dummy_test_data),
                               ucli_data_display);
        } else {
            ucli_printf(uc, "Disable pkt_data_dump for all ports\n");
            pdua_dump_data = PDUA_DUMP_DISABLE_ALL_PORTS;
        }
    }
    return UCLI_STATUS_OK;
}

static ucli_status_t
pdua_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(pdua)
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f pdua_ucli_ucli_handlers__[] = 
{
    pdua_ucli_ucli__show_pdua_counters__,
    pdua_ucli_ucli__clear_pdua_counters__,
    pdua_ucli_ucli__show_pdua_portcounters__,
    pdua_ucli_ucli__clear_pdua_portcounters__,
    pdua_ucli_ucli__show_pdua_portdata__,
    pdua_ucli_ucli__set_pkt_hexdump__,
    pdua_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
pdua_ucli_module__ =
    {
        "pdua_ucli",
        NULL,
        pdua_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
pdua_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&pdua_ucli_module__);
    n = ucli_node_create("pdua", NULL, &pdua_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("pdua"));
    return n;
}

#else
void*
pdua_ucli_node_create(void)
{
    return NULL;
}
#endif

