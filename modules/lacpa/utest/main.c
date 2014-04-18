/****************************************************************
 *
 *        Copyright 2013, Big Switch Networks, Inc.
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

#include <lacpa/lacpa_config.h>
#include <lacpa/lacpa.h>
#include <lacpa/lacpa.h>
#include <lacpa_int.h>
#include <SocketManager/socketmanager.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

uint8_t mac[6] = {0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00};
uint8_t mac2[6] = {0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00};
uint8_t data1[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x0a, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x5, 0x0};
uint8_t data2[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x06, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x5, 0x0};
uint8_t data3[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x6, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x1d, 0x0};

lacpa_port_t *port1, *port2;
static ind_soc_config_t soc_cfg;

indigo_error_t
lacp_create_send_packet_in (of_port_no_t in_port, of_octets_t *of_octets) 
{
    of_match_t     match;
    of_packet_in_t *of_packet_in;

    if (!of_octets) return INDIGO_ERROR_UNKNOWN;

    if ((of_packet_in = of_packet_in_new(OF_VERSION_1_3)) == NULL) {
        return INDIGO_ERROR_RESOURCE;
    }

    of_packet_in_total_len_set(of_packet_in, of_octets->bytes);

    match.version = OF_VERSION_1_3;
    match.fields.in_port = in_port;
    OF_MATCH_MASK_IN_PORT_EXACT_SET(&match);
    if ((of_packet_in_match_set(of_packet_in, &match)) != OF_ERROR_NONE) {
        printf("Failed to write match to packet-in message\n");
        of_packet_in_delete(of_packet_in);
        return INDIGO_ERROR_UNKNOWN;
    }

    if ((of_packet_in_data_set(of_packet_in, of_octets)) != OF_ERROR_NONE) {
        printf("Failed to write packet data to packet-in message\n");
        of_packet_in_delete(of_packet_in);
        return INDIGO_ERROR_UNKNOWN;
    }

    if (lacpa_packet_in_handler(of_packet_in) == INDIGO_CORE_LISTENER_RESULT_DROP) {
        printf("Listener dropped packet-in\n");
    } else {
        printf("Listener passed packet-in\n");
    }

    of_packet_in_delete(of_packet_in);
    return INDIGO_ERROR_NONE;
}

/*
 * Stub function's to avoid compilation failure in lacpa/utest module
 */
void
indigo_cxn_send_controller_message (indigo_cxn_id_t cxn_id, of_object_t *obj)
{
    printf("lacpa module: Send a REPLY to the controller\n");
}

void
indigo_cxn_send_async_message (of_object_t *obj)
{
    printf("lacpa module: Send an ASYNC msg to the controller\n");
}

indigo_error_t
indigo_cxn_get_async_version (of_version_t *version)
{
    *version = OF_VERSION_1_3;
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *of_packet_out)
{
    of_port_no_t     port_no = 0;
    of_octets_t      of_octets;
    of_list_action_t action;
    of_action_t      act;
    int              rv;

    if (!of_packet_out) return INDIGO_ERROR_NONE;

    of_packet_out_actions_bind(of_packet_out, &action);
    OF_LIST_ACTION_ITER(&action, &act, rv) {
        of_action_output_port_get(&act.output, &port_no);
    }

    of_packet_out_data_get(of_packet_out, &of_octets);

    printf("lacpa module: Send a packet out the port: %d\n", port_no);
    if (port_no == 10) {
        lacp_create_send_packet_in(20, &of_octets);
    } else if (port_no == 20){
        lacp_create_send_packet_in(10, &of_octets);
    }

    return INDIGO_ERROR_NONE;
}

int
aim_main(int argc, char* argv[])
{
    lacpa_info_t info1, info2;
    memset(&info1, 0, sizeof(lacpa_info_t));
    memset(&info2, 0, sizeof(lacpa_info_t));

    ind_soc_init(&soc_cfg);

    if (!lacpa_is_initialized()) {
        lacpa_init();
    }

    port1 = lacpa_find_port(10);
    port2 = lacpa_find_port(20);
    if (!port1 || !port2) {
        printf("ERROR - PORT ALLOCATION FAILED");
        return 0;
    }

    info1.sys_priority = 32768;
    memcpy(&info1.sys_mac.addr, mac, 6);
    info1.port_priority = 32768;
    info1.port_num = 25;
    info1.key = 13;
    info1.port_no = 10;
    lacpa_init_port(&info1, true);

    info2.sys_priority = 32768;
    memcpy(&info2.sys_mac.addr, mac2, 6);
    info2.port_priority = 32768;
    info2.port_num = 0x16;
    info2.key = 0xe;
    info2.port_no = 20;
    lacpa_init_port(&info2, true);

    /*
     * Make sure the lacp protocol converges
     */
    assert(port1->is_converged == true);
    assert(port2->is_converged == true);

    /*
     * Code to test recv of init() msg from controller with different params
     */
    info2.sys_priority = 25000;
    info2.key = 0xf;
    printf("Resending Port init() msg\n"); 
    lacpa_init_port(&info2, true);

    assert(port1->is_converged == true);
    assert(port2->is_converged == true);
   
    lacpa_finish(); 
    return 0;
}

