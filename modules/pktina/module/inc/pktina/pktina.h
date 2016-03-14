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

 #ifndef __PKTINA_H__
 #define __PKTINA_H__

#include <indigo/error.h>
#include <indigo/of_state_manager.h>

/**
 * Pass the packet-in though all the agents and send it to indigo
 * only if no agent ownes the packet or if it is a debug packet-in.
 * @param packet_in Pointer to the packet in object
 *
 * The pktin agent takes responsibility for the object
 */

extern indigo_error_t pktina_process_of_packet_in(of_packet_in_t *packet_in);

 #endif /* __PKTINA_H__ */
