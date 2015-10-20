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

#ifndef __PDUA_H__
#define __PDUA_H__

#include <indigo/of_state_manager.h>
#include <AIM/aim.h>

/* <auto.start.enum(ALL).header> */
/** pdua_packet_state */
typedef enum pdua_packet_state_e {
    PDUA_PACKET_STATE_UNKNOWN,
    PDUA_PACKET_STATE_HIT,
    PDUA_PACKET_STATE_MISS,
    PDUA_PACKET_STATE_LAST = PDUA_PACKET_STATE_MISS,
    PDUA_PACKET_STATE_COUNT,
    PDUA_PACKET_STATE_INVALID = -1,
} pdua_packet_state_t;

/** Strings macro. */
#define PDUA_PACKET_STATE_STRINGS \
{\
    "UNKNOWN", \
    "HIT", \
    "MISS", \
}
/** Enum names. */
const char* pdua_packet_state_name(pdua_packet_state_t e);

/** Enum values. */
int pdua_packet_state_value(const char* str, pdua_packet_state_t* e, int substr);

/** Enum descriptions. */
const char* pdua_packet_state_desc(pdua_packet_state_t e);

/** validator */
#define PDUA_PACKET_STATE_VALID(_e) \
    ( (0 <= (_e)) && ((_e) <= PDUA_PACKET_STATE_MISS))

/** pdua_packet_state_map table. */
extern aim_map_si_t pdua_packet_state_map[];
/** pdua_packet_state_desc_map table. */
extern aim_map_si_t pdua_packet_state_desc_map[];
/* <auto.end.enum(ALL).header> */

/****************************
 **** PDUA external APIs ****
 ****************************/

int pdua_system_init(void);
void pdua_system_finish(void);
indigo_core_listener_result_t pdua_receive_packet(of_octets_t *data,
                                                  of_port_no_t port_no);
indigo_error_t pdua_port_dump_enable_set(of_port_no_t port_no, bool enabled);
indigo_error_t pdua_port_dump_enable_get(of_port_no_t port_no, bool *enabled);

typedef void
(*pdua_pkt_event_listener_callback_f)(of_port_no_t port_no,
                                      pdua_packet_state_t pkt_state);

indigo_error_t
pdua_pkt_event_listener_register(pdua_pkt_event_listener_callback_f fn);

void
pdua_pkt_event_listener_unregister(pdua_pkt_event_listener_callback_f fn);

bool pdua_port_is_rx_registered(of_port_no_t port_no);

#endif /* __PDUA_H__ */
