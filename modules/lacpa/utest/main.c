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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

uint8_t mac[6] = {0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00};
uint8_t mac2[6] = {0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00};
uint8_t src_mac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
uint8_t src_mac2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t data1[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x0a, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x5, 0x0};
uint8_t data2[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x06, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x5, 0x0};
uint8_t data3[54] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x83, 0x16, 0xF5, 0x10, 0x88, 0x09, 0x01, 0x01, 0x01, 0x014, 0x80, 0x00, 0x00, 0x0e, 0x83, 0x16, 0xf5, 0x00, 0x00, 0xe, 0x80, 0x00, 0x00, 0x16, 0x6, 0x00, 0x00, 0x00, 0x2, 0x14, 0x80, 0x00, 0x00, 0x13, 0xc4, 0x12, 0x0f, 0x00, 0x00, 0x0d, 0x80, 0x00, 0x00, 0x19, 0x1d, 0x0};

lacpa_port_t *port1, *port2;

/*
 * Stub function's to avoid compilation failure in lacpa/utest module
 */
extern void
indigo_cxn_send_controller_message (indigo_cxn_id_t cxn_id, of_object_t *obj)
{
	printf("lacpa module: Send a REPLY to the controller\n");
}

extern void
indigo_cxn_send_async_message (of_object_t *obj)
{
	printf("lacpa module: Send an ASYNC msg to the controller\n");
}

extern indigo_error_t
indigo_cxn_get_async_version (of_version_t *version)
{
	*version = OF_VERSION_1_3;
	return INDIGO_ERROR_NONE;
}

extern indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *of_packet_out)
{
	printf("lacpa module: Send a packet out the port\n");
	return INDIGO_ERROR_NONE;
}

extern void
lacpa_send_utest (lacpa_port_t *port, uint8_t *data, uint32_t bytes)
{
    if (!port) return;

    if (port->actor.port_no == 10) {
        lacpa_receive_utest(port2, data, bytes);
    } else if (port->actor.port_no == 20){
        lacpa_receive_utest(port1, data, bytes);
    }
}

int
aim_main(int argc, char* argv[])
{
    lacpa_system_t lacp_system;
    lacpa_info_t info1, info2;
    memset(&info1, 0, sizeof(lacpa_info_t));
    memset(&info2, 0, sizeof(lacpa_info_t));

    if (!lacpa_is_system_initialized()) {
		lacpa_init_system(&lacp_system);
    }

    port1 = lacpa_find_port(&lacp_system, 10);
    port2 = lacpa_find_port(&lacp_system, 20);
    if (!port1 || !port2) {
        printf("FATAL ERROR - PORT ALLOCATION FAILED");
        return 0;
    }
    memcpy(&port1->src_mac.addr, src_mac, 6);
    memcpy(&port2->src_mac.addr, src_mac2, 6);

    info1.sys_priority = 32768;
    memcpy(&info1.sys_mac.addr, mac, 6);
    info1.port_priority = 32768;
    info1.port_num = 25;
    info1.key = 13;
    info1.port_no = 10;
    lacpa_init_port(&lacp_system, &info1, TRUE);

    info2.sys_priority = 32768;
    memcpy(&info2.sys_mac.addr, mac2, 6);
    info2.port_priority = 32768;
    info2.port_num = 0x16;
    info2.key = 0xe;
    info2.port_no = 20;
    lacpa_init_port(&lacp_system, &info2, TRUE);

	//Code to test recv of init() msg from controller with different params
    info2.sys_priority = 25000;
	info2.key = 0xf;
	printf("Resending Port init() msg\n"); 
    lacpa_init_port(&lacp_system, &info2, TRUE);

    //lacpa_config_show(&aim_pvs_stdout);
    return 0;
}

