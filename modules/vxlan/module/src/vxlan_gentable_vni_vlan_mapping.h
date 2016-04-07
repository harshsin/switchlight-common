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

#ifndef __VXLAN_GENTABLE_VNI_VLAN_MAPPING_H__
#define __VXLAN_GENTABLE_VNI_VLAN_MAPPING_H__

extern indigo_error_t vxlan_gentable_vni_vlan_mapping_init(void);

extern indigo_error_t vxlan_gentable_vni_vlan_mapping_deinit(void);

#if VXLAN_CONFIG_INCLUDE_UCLI == 1
#include <uCli/ucli.h>
extern void vxlan_gentable_vni_vlan_mapping_print(ucli_context_t *uc);
#endif /* VXLAN_CONFIG_INCLUDE_UCLI == 1 */

#endif /* __VXLAN_GENTABLE_VNI_VLAN_MAPPING_H__ */
