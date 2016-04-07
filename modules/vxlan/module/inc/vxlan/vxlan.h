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

#ifndef __VXLAN_H__
#define __VXLAN_H__

#include <indigo/error.h>

#define VXLAN_UDP_DST_PORT_UNDEFINED 0

/**
 * Initialize VXLAN agent.
 */
extern indigo_error_t vxlan_init(void);

/**
 * Uninitialize VXLAN agent.
 */
extern indigo_error_t vxlan_finish(void);

/**
 * Get VXLAN udp destination port configured.
 * Returns VXLAN_UDP_DST_PORT_UNDEFINED if not configured.
 */
extern int vxlan_gentable_protocol_identifier_udp_dst_port_get(void);

/**
 * Get vlan_vid for given vni.
 * Returns INDIGO_ERROR_NOT_FOUND if mapping is not configured.
 */
extern indigo_error_t vxlan_gentable_vni_vlan_mapping_get(uint32_t vni, uint16_t *vlan_vid);

#endif /* __VXLAN_H__ */
