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
static inline indigo_error_t
slshared_fwd_packet_out(of_octets_t *octets, of_port_no_t in_port,
                        of_port_no_t out_port, int queue_id)
{
    of_packet_out_t    *obj;
    of_list_action_t   *list;
    of_action_output_t *action;
    indigo_error_t     rv;

    AIM_ASSERT(octets != NULL, "NULL octets");

    obj = of_packet_out_new(OF_VERSION_1_3);
    AIM_TRUE_OR_DIE(obj != NULL);

    list = of_list_action_new(obj->version);
    AIM_TRUE_OR_DIE(list != NULL);

    if (queue_id != QUEUE_ID_INVALID) {
        of_action_set_queue_t *queue_action = of_action_set_queue_new(obj->version);
        AIM_TRUE_OR_DIE(queue_action != NULL);

        of_action_set_queue_queue_id_set(queue_action, queue_id);
        of_list_append(list, queue_action);
        of_object_delete(queue_action);
    }

    action = of_action_output_new(obj->version);
    AIM_TRUE_OR_DIE(action != NULL);

    of_packet_out_buffer_id_set(obj, -1);
    of_packet_out_in_port_set(obj, in_port);
    of_action_output_port_set(action, out_port);
    of_list_append(list, action);
    of_object_delete(action);
    rv = of_packet_out_actions_set(obj, list);
    AIM_ASSERT(rv == 0);
    of_object_delete(list);

    if (of_packet_out_data_set(obj, octets) < 0) {
        AIM_DIE("Failed to set data");
    }

    rv = indigo_fwd_packet_out(obj);
    of_packet_out_delete(obj);
    return rv;
}

#endif /* __SLSHARED__H__ */
