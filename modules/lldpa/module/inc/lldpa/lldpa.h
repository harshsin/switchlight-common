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

#ifndef __LLDPA_H__
#define __LLDPA_H__

#include <indigo/indigo.h>
#include <PPE/ppe.h>

/* <--auto.start.enum(ALL).header> */
/* <auto.end.enum(ALL).header> */

/****************************
 **** LLDPA external APIs ****
 ****************************/

int lldpa_system_init();
void lldpa_system_finish();
indigo_error_t lldpa_receive_packet(of_octets_t *data, of_port_no_t port_no);

#endif /* __LLDPA_H__ */
