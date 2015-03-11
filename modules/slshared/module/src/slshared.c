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

#include <slshared/slshared.h>

indigo_error_t
slshared_fwd_packet_out(of_octets_t *octets, of_port_no_t in_port,
                        of_port_no_t out_port, int queue_id)
{
    of_packet_out_t    *obj;
    of_list_action_t   *list;
    indigo_error_t     rv;

    AIM_ASSERT(octets != NULL, "NULL octets");

    obj = of_packet_out_new(OF_VERSION_1_4);
    AIM_TRUE_OR_DIE(obj != NULL);

    of_packet_out_buffer_id_set(obj, -1);
    of_packet_out_in_port_set(obj, in_port);

    list = of_list_action_new(obj->version);
    AIM_TRUE_OR_DIE(list != NULL);

    if (queue_id != QUEUE_ID_INVALID) {
        of_action_set_queue_t queue_action;
        of_action_set_queue_init(&queue_action, obj->version, -1, 1);
        of_list_action_append_bind(list, &queue_action);
        of_action_set_queue_queue_id_set(&queue_action, queue_id);
    }

    of_action_output_t action;
    of_action_output_init(&action, obj->version, -1, 1);
    of_list_action_append_bind(list, &action);
    of_action_output_port_set(&action, out_port);

    AIM_TRUE_OR_DIE(of_packet_out_actions_set(obj, list) == 0);
    of_object_delete(list);

    if (of_packet_out_data_set(obj, octets) < 0) {
        AIM_DIE("Failed to set data");
    }

    rv = indigo_fwd_packet_out(obj);
    of_packet_out_delete(obj);
    return rv;
}
