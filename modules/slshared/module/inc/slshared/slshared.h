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

#ifndef __SLSHARED_H__
#define __SLSHARED_H__

#include <indigo/indigo.h>
#include <indigo/of_state_manager.h>

#define QUEUE_ID_INVALID -1

#define VLAN_PCP_OFFSET    13
#define VLAN_PCP_MASK      0x7
#define VLAN_VID_MASK      0xFFF
#define VLAN_TCI(vid, pcp) ( (((pcp) & VLAN_PCP_MASK) << VLAN_PCP_OFFSET) | ((vid) & VLAN_VID_MASK) )
#define STRIP_OFPVID_PRESENT(tci) VLAN_VID(tci)
#define VLAN_VID(tci) ((tci) & 0xfff)
#define VLAN_PCP(tci) ((tci) >> 13)

/**
 * @brief Construct an of_packet_out_t and send the packet out.
 * This api is a wrapper around indigo_fwd_packet_out.
 * @param octets Buffer containing packet length and data.
 * @param in_port Input switch port.
 * @param out_port Output port.
 * @param queue_id QOS packet out queue; if set to QUEUE_ID_INVALID,
 * don't add OF_ACTION_SET_QUEUE to the action list.
 */
indigo_error_t slshared_fwd_packet_out(of_octets_t *octets, of_port_no_t in_port,
                                       of_port_no_t out_port, int queue_id);

#endif /* __SLSHARED__H__ */
