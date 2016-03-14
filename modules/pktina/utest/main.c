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

#include <pktina/pktina_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

#include <indigo/of_state_manager.h>
#include <indigo/port_manager.h>
#include <SocketManager/socketmanager.h>

int aim_main(int argc, char* argv[])
{
    printf("pktina Utest Is Empty\n");
    pktina_config_show(&aim_pvs_stdout);
    return 0;
}


void
indigo_cxn_send_bsn_error(indigo_cxn_id_t cxn_id,
                          of_object_t *orig,
                          char *err_txt)
{
}

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *packet_out)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_modify(of_port_mod_t *port_mod)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_queue_config_get(of_queue_get_config_request_t *request,
                             of_queue_get_config_reply_t **reply_ptr)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_forwarding_features_get(of_features_reply_t *features)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_interface_list(indigo_port_info_t **list)
{
    return INDIGO_ERROR_NOT_SUPPORTED;
}

void
indigo_port_interface_list_destroy(indigo_port_info_t *list)
{
}

void
indigo_fwd_pipeline_stats_get(of_desc_str_t **pipelines,
                              int *num_pipelines)
{
}

void
indigo_fwd_pipeline_get(of_desc_str_t pipeline)
{
}

indigo_error_t
indigo_fwd_pipeline_set(of_desc_str_t pipeline)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_experimenter(of_experimenter_t *experimenter,
                         indigo_cxn_id_t cxn_id)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_features_get(of_features_reply_t *features)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_experimenter(of_experimenter_t *experimenter,
                        indigo_cxn_id_t cxn_id)
{
    return INDIGO_ERROR_NONE;
}
