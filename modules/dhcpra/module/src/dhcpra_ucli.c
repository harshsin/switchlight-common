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

#include <dhcpra/dhcpra_config.h>

#if DHCPRA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

#include <arpa/inet.h>
#include "dhcp.h"
#include "dhcpra_int.h"
#include "dhcpr_table.h"
#include "dhcrelay.h"

extern int            dhcpra_dump_data;

static ucli_status_t
dhcpra_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(dhcpra)
}

static ucli_status_t
dhcpra_ucli_ucli__set_pkt_hexdump__(ucli_context_t* uc)
{
    uint32_t port = 0;

    UCLI_COMMAND_INFO(uc,
                      "port_dump", -1,
                      "$summary#Set pkt_data hexdump (used with trace)"
                      "$args#[Port]");

    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &port);
        ucli_printf(uc, "Enable pkt_data_dump for port %d\n", port);
        dhcpra_dump_data = port;

    } else {
        ucli_printf(uc, "Toggle DHCPRA pkt_data_dump switch\n");
        if (dhcpra_dump_data == DHCPRA_DUMP_DISABLE_ALL_PORTS) {
            ucli_printf(uc, "Enable pkt_data_dump for all ports\n");
            dhcpra_dump_data = DHCPRA_DUMP_ENABLE_ALL_PORTS;
        } else {
            ucli_printf(uc, "Disable pkt_data_dump for all ports\n");
            dhcpra_dump_data = DHCPRA_DUMP_DISABLE_ALL_PORTS;
        }
    }
    return UCLI_STATUS_OK;
}

static void
dhcpra_show_dhcp_relay__(ucli_context_t* uc, uint32_t vlan_id)
{
    dhc_relay_t *de = dhcpr_get_dhcpr_entry_from_vlan_table(vlan_id);
    struct in_addr ip;
    uint8_t *p;

    if (de) {
        ucli_printf(uc, "%d\t", vlan_id);
        ip.s_addr   = de->vrouter_ip;
        ucli_printf(uc, "%s\t", inet_ntoa(ip));
        p = de->vrouter_mac.addr;
        ucli_printf(uc,"%02x:%02x:%02x:%02x:%02x:%02x\t",
                       p[0], p[1], p[2], p[3], p[4], p[5]);
        ip.s_addr = de->dhcp_server_ip;
        ucli_printf(uc, "%s\t", inet_ntoa(ip));
        ucli_printf(uc,"%{data}\n", de->opt_id.circuit_id.data, de->opt_id.circuit_id.bytes);
    }
}

static ucli_status_t
dhcpra_ucli_ucli__show_dhcpr_table__(ucli_context_t* uc)
{
    uint32_t vlan_id = 0;

    UCLI_COMMAND_INFO(uc,
                      "dhcpr_table", -1,
                      "$summary#Display the config per internal vlan."
                      "$args#[Vlan_id]");

    ucli_printf(uc, "START DHCP CONFIG INFO\n");
    ucli_printf(uc, "VLAN_ID\tROUTER_IP\tROUTER_MAC\tDHCP_SERVER_IP\tCIRCUIT_DATA\n");
    if (uc->pargs->count == 1) {
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &vlan_id);
        dhcpra_show_dhcp_relay__(uc, vlan_id);
    } else {
        for (vlan_id = 0; vlan_id <= VLAN_MAX; vlan_id++) {
            dhcpra_show_dhcp_relay__(uc, vlan_id);
        }
    }
    return UCLI_STATUS_OK;
}

static ucli_status_t
dhcpra_ucli_ucli__show_dhcrelay_stat__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "dhcrelay_stat", -1,
                      "$summary#Display dhcrelay statistics");

    ucli_printf(uc, "agent_option_errors=%u\n",dhc_relay_stat.agent_option_errors);
    ucli_printf(uc, "dhcp_request_cookie_unfound=%u\n",dhc_relay_stat.missing_request_cookie);
    ucli_printf(uc, "dhcp_request_message_missing=%u\n",dhc_relay_stat.missing_request_message);
    ucli_printf(uc, "missing_circuit_id=%u\n",dhc_relay_stat.missing_circuit_id);
    ucli_printf(uc, "bad_circuit_id=%u\n",dhc_relay_stat.bad_circuit_id);
    ucli_printf(uc, "corrupt_agent_options=%u\n",dhc_relay_stat.corrupt_agent_options);
    ucli_printf(uc, "missing_dhcp_agent_optio=%u\n",dhc_relay_stat.missing_dhcp_agent_option);
    ucli_printf(uc, "dhcp_reply_cookie_unfound=%u\n",dhc_relay_stat.missing_reply_cookie);
    ucli_printf(uc, "dhcp_reply_message_missing=%u\n",dhc_relay_stat.missing_reply_message);

    return UCLI_STATUS_OK;
}

static ucli_status_t
dhcpra_ucli_ucli__clear_dhcrelay_stat__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "clr_relay_stat", -1,
                      "$summary#Clear dhcrelay statistics");

    dhc_relay_stat.agent_option_errors = 0 ;
    dhc_relay_stat.missing_request_cookie = 0;
    dhc_relay_stat.missing_request_message = 0;
    dhc_relay_stat.missing_circuit_id = 0;
    dhc_relay_stat.bad_circuit_id = 0;
    dhc_relay_stat.corrupt_agent_options = 0;
    dhc_relay_stat.missing_dhcp_agent_option = 0;
    dhc_relay_stat.missing_reply_cookie = 0;
    dhc_relay_stat.missing_reply_message = 0;

    return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f dhcpra_ucli_ucli_handlers__[] =
{
    dhcpra_ucli_ucli__show_dhcrelay_stat__,
    dhcpra_ucli_ucli__clear_dhcrelay_stat__,
    dhcpra_ucli_ucli__show_dhcpr_table__,
    dhcpra_ucli_ucli__set_pkt_hexdump__,
    dhcpra_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
dhcpra_ucli_module__ =
    {
        "dhcpra_ucli",
        NULL,
        dhcpra_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
dhcpra_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&dhcpra_ucli_module__);
    n = ucli_node_create("dhcpra", NULL, &dhcpra_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("dhcpra"));
    return n;
}

#else
void*
dhcpra_ucli_node_create(void)
{
    return NULL;
}
#endif

