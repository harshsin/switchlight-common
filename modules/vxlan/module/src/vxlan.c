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

#include <indigo/of_state_manager.h>
#include "vxlan_int.h"
#include "vxlan_log.h"
#include "vxlan_gentable_protocol_identifier.h"
#include "vxlan_gentable_vni_vlan_mapping.h"
#include "vxlan/vxlan.h"
#include <arpa/inet.h>

 /* VXLAN protocol header:
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |G|R|R|R|I|R|R|C|               Reserved                        |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  * |                VXLAN Network Identifier (VNI) |   Reserved    |
  * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  * G = 1        Group Policy (VXLAN-GBP)
  * I = 1        VXLAN Network Identifier (VNI) present
  * C = 1        Remote checksum offload (RCO)
  */

typedef struct vxlan_hdr_s {
    uint32_t flags;
    uint32_t vni;
} vxlan_hdr_t;

#define VXLAN_HEADER_SIZE  (sizeof(vxlan_hdr_t))
#define VXLAN_VNI(vni)     ((vni) >> 8)

static bool vxlan_initialized = false;
indigo_error_t
vxlan_init(void)
{
    indigo_error_t rv;

    if (vxlan_initialized) return INDIGO_ERROR_NONE;

    rv = vxlan_gentable_protocol_identifier_init();
    if (rv != INDIGO_ERROR_NONE) {
        return rv;
    }

    rv = vxlan_gentable_vni_vlan_mapping_init();
    if (rv != INDIGO_ERROR_NONE) {
        return rv;
    }

    vxlan_initialized = true;

    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_finish(void)
{
    if (!vxlan_initialized) return INDIGO_ERROR_NONE;

    (void) vxlan_gentable_protocol_identifier_deinit();

    (void) vxlan_gentable_vni_vlan_mapping_deinit();

    vxlan_initialized = false;

    return INDIGO_ERROR_NONE;
}

static uint8_t*
vxlan_header_get(ppe_packet_t *ppep)
{
    uint8_t *start;
    uint32_t vxlan_udp_port, pkt_udp_dest_port;

    /* VXLAN packets are UDP packets and the destination port
       is a well-known UDP port (4789 per RFC7348). */
    start = ppe_header_get(ppep, PPE_HEADER_UDP);
    if (!start) {
        return NULL;
    }

    vxlan_udp_port = vxlan_protocol_identifier_udp_dst_port_get();
    ppe_field_get(ppep, PPE_FIELD_UDP_DST_PORT, &pkt_udp_dest_port);

    /* Check if the packet UDP dest port matches VXLAN UDP port. */
    if (vxlan_udp_port != pkt_udp_dest_port) {
        return NULL;
    }

    /* Since we know this is a VXLAN packet, find the start
       of the VXLAN header */
    return (start + SLSHARED_CONFIG_UDP_HEADER_SIZE);
}

uint8_t*
vxlan_payload_get(ppe_packet_t *ppep, uint32_t *length)
{
    uint8_t *vxlan_hdr = vxlan_header_get(ppep);
    if (!vxlan_hdr) {
        return NULL;
    }

    uint32_t udp_length;
    ppe_field_get(ppep, PPE_FIELD_UDP_LENGTH, &udp_length);
    *length = udp_length - SLSHARED_CONFIG_UDP_HEADER_SIZE - VXLAN_HEADER_SIZE;

    /* Since we know this is a VXLAN packet, find the start of the inner
       L2 payload. */
    return (vxlan_hdr + VXLAN_HEADER_SIZE);
}

indigo_error_t
vxlan_vni_get(ppe_packet_t *ppep, uint32_t *vni)
{
    uint8_t *start = vxlan_header_get(ppep);
    if (!start) {
        return INDIGO_ERROR_NOT_FOUND;
    }

    vxlan_hdr_t *vxlan_hdr = (vxlan_hdr_t *)(start);
    *vni = VXLAN_VNI(ntohl(vxlan_hdr->vni));

    return INDIGO_ERROR_NONE;
}
