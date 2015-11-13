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

#include <indigo/indigo.h>
#include <indigo/of_state_manager.h>
#include <AIM/aim.h>

#ifndef __IGMPA_H__
#define __IGMPA_H__

indigo_error_t igmpa_init(void);
void igmpa_finish(void);

/* sends a IGMP or PIM packet directly to the IGMP agent */
indigo_core_listener_result_t
igmpa_receive_pkt(ppe_packet_t *ppep, of_port_no_t in_port);

/* returns gentable ids for bundle comparator;
 * tx_port_group entries should be installed first,
 * before gq_tx/report_tx entries */
uint16_t igmpa_tx_port_group_table_id_get(void);

void igmpa_stats_show(aim_pvs_t *pvs);

#endif /* __IGMPA_H__ */
