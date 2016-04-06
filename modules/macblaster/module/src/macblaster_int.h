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

/*************************************************************//**
 *
 * macblaster Internal Header
 *
 ****************************************************************/
#ifndef __MACBLASTER_INT_H__
#define __MACBLASTER_INT_H__

#include <indigo/of_state_manager.h>
#include <indigo/of_connection_manager.h>
#include <SocketManager/socketmanager.h>
#include <debug_counter/debug_counter.h>
#include <loci/loci_base.h>
#include <PPE/ppe.h>
#include <slshared/slshared.h>
#include <slshared/slshared_config.h>
#include <macblaster/macblaster_config.h>
#include "macblaster_log.h"

#define MACBLASTER_PORT_STATS                    \
    MACBLASTER_PORT_STAT(pktout_failure)         \
    MACBLASTER_PORT_STAT(pktout_success)

typedef struct macblaster_port_debug_s {
#define MACBLASTER_PORT_STAT(name)     \
    debug_counter_t name;              \
    char name##_counter_name_buf[DEBUG_COUNTER_NAME_SIZE];

    MACBLASTER_PORT_STATS
#undef MACBLASTER_PORT_STAT
} macblaster_port_debug_t;

#if PKTINA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>

extern void macblaster_debug_counters_print(ucli_context_t* uc);

extern void macblaster_debug_counters_clear(void);

extern void macblaster_port_debug_counters_print(ucli_context_t *uc, of_port_no_t of_port);

extern void macblaster_port_debug_counters_clear(of_port_no_t of_port);

#endif /* PKTINA_CONFIG_INCLUDE_UCLI == 1 */

#endif /* __MACBLASTER_INT_H__ */
