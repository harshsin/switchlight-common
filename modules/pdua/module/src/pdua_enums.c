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

#include <pdua/pdua_config.h>
#include <pdua/pdua.h>

/* <auto.start.enum(ALL).source> */
aim_map_si_t pdua_packet_state_map[] =
{
    { "UNKNOWN", PDUA_PACKET_STATE_UNKNOWN },
    { "HIT", PDUA_PACKET_STATE_HIT },
    { "MISS", PDUA_PACKET_STATE_MISS },
    { NULL, 0 }
};

aim_map_si_t pdua_packet_state_desc_map[] =
{
    { "None", PDUA_PACKET_STATE_UNKNOWN },
    { "None", PDUA_PACKET_STATE_HIT },
    { "None", PDUA_PACKET_STATE_MISS },
    { NULL, 0 }
};

const char*
pdua_packet_state_name(pdua_packet_state_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, pdua_packet_state_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'pdua_packet_state'";
    }
}

int
pdua_packet_state_value(const char* str, pdua_packet_state_t* e, int substr)
{
    int i;
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, pdua_packet_state_map, 0)) {
        /* Enum Found */
        *e = i;
        return 0;
    }
    else {
        return -1;
    }
}

const char*
pdua_packet_state_desc(pdua_packet_state_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, pdua_packet_state_desc_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'pdua_packet_state'";
    }
}

/* <auto.end.enum(ALL).source> */

