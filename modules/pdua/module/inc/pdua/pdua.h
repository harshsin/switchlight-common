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

#ifndef __PDUA_H__
#define __PDUA_H__

#include <indigo/of_state_manager.h>

/* <--auto.start.enum(ALL).header> */
/* <auto.end.enum(ALL).header> */

/****************************
 **** PDUA external APIs ****
 ****************************/

int pdua_system_init(void);
void pdua_system_finish(void);
indigo_core_listener_result_t pdua_receive_packet(of_octets_t *data,
                                                  of_port_no_t port_no);
indigo_error_t pdua_port_dump_enable_set(of_port_no_t port_no, bool enabled);
indigo_error_t pdua_port_dump_enable_get(of_port_no_t port_no, bool *enabled);
#endif /* __PDUA_H__ */
