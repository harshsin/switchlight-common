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

#include <macblaster/macblaster_config.h>
#include <macblaster/macblaster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

#include <indigo/of_state_manager.h>
#include <SocketManager/socketmanager.h>

static ind_soc_config_t soc_cfg;

int aim_main(int argc, char* argv[])
{
    ind_soc_init(&soc_cfg);
    AIM_TRUE_OR_DIE(macblaster_init() == INDIGO_ERROR_NONE);
    macblaster_finish();
    ind_soc_finish();
    return 0;
}

void
indigo_cxn_pause(indigo_cxn_id_t cxn_id)
{
}

void
indigo_cxn_resume(indigo_cxn_id_t cxn_id)
{
}

indigo_error_t
indigo_core_message_listener_register(indigo_core_message_listener_f fn)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_core_message_listener_unregister(indigo_core_message_listener_f fn)
{
}

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *obj)
{
	return INDIGO_ERROR_NONE;
}

void
indigo_cxn_send_bsn_error(indigo_cxn_id_t cxn_id, of_object_t *orig,
                          char *err_txt)
{
}
