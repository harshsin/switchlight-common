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

#ifndef DHCPR_VRF_H_
#define DHCPR_VRF_H_

#include <indigo/error.h>

indigo_error_t dhcpr_vrf_init();
void dhcpr_vrf_finish();

 /*
  * Get vrf from vlan and mac values
  * ret: 0; if successful
  */
int dhcpr_vrf_find(uint32_t *vrf, uint32_t vlan, uint8_t *mac_addr);

/* Print dhcp vrf table */
void dhcpr_vrf_table_print(aim_pvs_t *apvs);

#endif /* DHCPR_VRF_H_ */
