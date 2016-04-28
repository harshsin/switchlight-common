/****************************************************************
 *
 *        Copyright 2016, Big Switch Networks, Inc.
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

#include <pktina/pktina_config.h>
#include "pktina_int.h"

#if PKTINA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
pktina_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(pktina)
}

static ucli_status_t
pktina_ucli_ucli__counters__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, "counters", 0,
                      "$summary#Print pktina global counters.");

    pktina_debug_counters_print(uc);

    return UCLI_STATUS_OK;
}

static ucli_status_t
pktina_ucli_ucli__counters_clear__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, "counters_clear", 0,
                      "$summary#Clear pktina global counters.");

    pktina_debug_counters_clear();

    return UCLI_STATUS_OK;
}

static ucli_status_t
pktina_ucli_ucli__port_counters__(ucli_context_t* uc)
{
    of_port_no_t of_port;

    UCLI_COMMAND_INFO(uc, "port_counters", -1,
                      "$summary#Print pktina port counters."
                      "$args#[of_port_no]");

    if (uc->pargs->count == 0) {
        for (of_port = 0; of_port <= PKTINA_CONFIG_OF_PORTS_MAX; of_port++) {
            pktina_port_debug_counters_print(uc, of_port);
        }
        pktina_port_debug_counters_print(uc, OF_PORT_DEST_CONTROLLER);
    } else if (uc->pargs->count == 1) {
        /* Print pktina stats of specific port */
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &of_port);
        if (of_port <= PKTINA_CONFIG_OF_PORTS_MAX) {
            pktina_port_debug_counters_print(uc, of_port);
        } else if (of_port == OF_PORT_DEST_CONTROLLER) {
            pktina_port_debug_counters_print(uc, OF_PORT_DEST_CONTROLLER);
        } else {
            ucli_printf(uc, "Invalid OF port number %d, should be <= %d\n",
                        of_port, PKTINA_CONFIG_OF_PORTS_MAX);
            return UCLI_STATUS_E_PARAM;
        }
    } else {
        ucli_printf(uc, "Invalid arguments\n");
        ucli_printf(uc, "port_counters <of_port_no> --> print port stats\n");
        ucli_printf(uc, "port_counters --> print all port stats\n");
    }

    return UCLI_STATUS_OK;
}

static ucli_status_t
pktina_ucli_ucli__port_counters_clear__(ucli_context_t* uc)
{
    of_port_no_t of_port;

    UCLI_COMMAND_INFO(uc, "port_counters_clear", -1,
                      "$summary#Clear pktina port counters."
                      "$args#[of_port_no]");

    if (uc->pargs->count == 0) {
        for (of_port = 0; of_port <= PKTINA_CONFIG_OF_PORTS_MAX; of_port++) {
            pktina_port_debug_counters_clear(of_port);
        }
        pktina_port_debug_counters_clear(OF_PORT_DEST_CONTROLLER);
    } else if (uc->pargs->count == 1) {
        /* Clear pktina stats of specific port */
        UCLI_ARGPARSE_OR_RETURN(uc, "i", &of_port);
        if (of_port <= PKTINA_CONFIG_OF_PORTS_MAX) {
            pktina_port_debug_counters_clear(of_port);
        } else if (of_port == OF_PORT_DEST_CONTROLLER) {
            pktina_port_debug_counters_clear(OF_PORT_DEST_CONTROLLER);
        } else {
            ucli_printf(uc, "Invalid OF port number %d, should be <= %d\n",
                        of_port, PKTINA_CONFIG_OF_PORTS_MAX);
            return UCLI_STATUS_E_PARAM;
        }
    } else {
        ucli_printf(uc, "Invalid arguments\n");
        ucli_printf(uc, "port_counters_clear <of_port_no> --> clear port stats\n");
        ucli_printf(uc, "port_counters_clear --> clear all port stats\n");
    }

    return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f pktina_ucli_ucli_handlers__[] =
{
    pktina_ucli_ucli__config__,
    pktina_ucli_ucli__counters__,
    pktina_ucli_ucli__counters_clear__,
    pktina_ucli_ucli__port_counters__,
    pktina_ucli_ucli__port_counters_clear__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
pktina_ucli_module__ =
    {
        "pktina_ucli",
        NULL,
        pktina_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
pktina_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&pktina_ucli_module__);
    n = ucli_node_create("pktina", NULL, &pktina_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("pktina"));
    return n;
}

#else
void*
pktina_ucli_node_create(void)
{
    return NULL;
}
#endif
