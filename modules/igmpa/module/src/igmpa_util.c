/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * Utility functions for IGMP agent implementation.
 */

#include <AIM/aim.h>
#include <indigo/indigo.h>
#include <arpa/inet.h>
#include <PPE/ppe.h>
#include <slshared/slshared.h>

#include "igmpa_int.h"


indigo_error_t 
igmpa_parse_name_tlv(of_object_t *tlv, char *dst_name)
{
    of_octets_t name;
    of_bsn_tlv_name_value_get(tlv, &name);

    if (name.bytes >= IGMP_NAME_LEN) {
        AIM_LOG_ERROR("%s: name length %d exceeds maximum %d",
                      __FUNCTION__, name.bytes, IGMP_NAME_LEN);
        return INDIGO_ERROR_PARAM;
    }

    if (strnlen((char *)name.data, name.bytes) != name.bytes) {
        AIM_LOG_ERROR("%s: name length %d does not match expected %d",
                      __FUNCTION__, strnlen((char *)name.data, name.bytes),
                      name.bytes);
        return INDIGO_ERROR_PARAM;
    }

    IGMPA_MEMCPY(dst_name, name.data, name.bytes);
    dst_name[name.bytes] = 0;
    return INDIGO_ERROR_NONE;
}


/*
 * compute one's complement sum of the 'len' bytes pointed to by 'data'.
 * stolen from ppe_util.c 
 */
uint16_t
igmpa_sum16(uint8_t* data, int len)
{
    uint32_t sum = 0;
    uint16_t* sdata = (uint16_t*)data;
    int olen = len;

    while(len > 1) {
        sum += ntohs(*(sdata++));
        if(sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        len -= 2;
    }

    if(len) {
        uint16_t b = data[olen-1];
        sum += ntohs(b<<8);
    }

    return (sum & 0xFFFF) + (sum>>16);
}


int
igmpa_send_igmp_packet(igmpa_pkt_params_t *params)
{
    ppe_packet_t ppep;
    const int L2_len = 14;
    const int tag_len = 4;
    const int L3_len = 20;
    const int igmp_len = 8;
    const int pkt_len = L2_len + tag_len + L3_len + igmp_len;
    uint8_t pkt_bytes[pkt_len];
    uint32_t igmp_checksum;
    of_octets_t octets = { pkt_bytes, sizeof(pkt_bytes) };
    indigo_error_t rv;

    IGMPA_MEMSET(pkt_bytes, 0, sizeof(pkt_bytes));

    /* build and send report */
    ppe_packet_init(&ppep, pkt_bytes, pkt_len);
    /* set ethertypes before parsing */
    /* FIXME use ppe_packet_format_set? */
    pkt_bytes[12] = 0x81;
    pkt_bytes[13] = 0x00;
    pkt_bytes[16] = 0x08;
    pkt_bytes[17] = 0x00;
    if (ppe_parse(&ppep) < 0) {
        AIM_DIE("ppe_parse failed sending IGMP packet");
    }

    /* ethernet */
    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_SRC_MAC, 
                           params->eth_src) < 0) {
        AIM_DIE("failed to set PPE_FIELD_ETHERNET_SRC_MAC");
    }
    if (ppe_wide_field_set(&ppep, PPE_FIELD_ETHERNET_DST_MAC,
                           params->eth_dst) < 0) {
        AIM_DIE("failed to set PPE_FIELD_ETHERNET_DST_MAC");
    }

    /* tag */
    if (ppe_field_set(&ppep, PPE_FIELD_8021Q_VLAN,
                      params->vlan_vid) < 0) {
        AIM_DIE("failed to set PPE_FIELD_8021Q_VLAN");
    }

    /* ipv4 */
    /* FIXME add router alert? */
    if (ppe_build_ipv4_header(&ppep, params->ipv4_src, params->ipv4_dst,
                              L3_len + igmp_len,
                              2 /* IGMP */, 1 /* TTL */) < 0) {
        AIM_DIE("failed to build ipv4 header");
    }
    /* igmp */
    if (ppe_field_set(&ppep, PPE_FIELD_IGMP_TYPE, params->igmp_type) < 0) {
        AIM_DIE("failed to set PPE_FIELD_IGMP_TYPE");
    }
    if (ppe_field_set(&ppep, PPE_FIELD_IGMP_GROUP_ADDRESS,
                      params->igmp_group_addr) < 0) {
        AIM_DIE("failed to set PPE_FIELD_IGMP_GROUP_ADDRESS");
    }

    /* compute and set IGMP checksum */
    igmp_checksum = igmpa_sum16(ppe_header_get(&ppep, PPE_HEADER_IGMP),
                                igmp_len);
    if (ppe_field_set(&ppep, PPE_FIELD_IGMP_CHECKSUM,
                      (0xffff - igmp_checksum)) < 0) {
        AIM_DIE("failed to set PPE_FIELD_IGMP_CHECKSUM");
    }

    if (ppe_packet_update(&ppep) < 0) {
        AIM_DIE("ppe_packet_update failed for IGMP report");
    }

    /* send packet */
    rv = slshared_fwd_packet_out(&octets, OF_PORT_DEST_CONTROLLER,
                                 params->output_port_no, QUEUE_ID_INVALID);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_INTERNAL("Failed to send IGMP report: %s",
                         indigo_strerror(rv));
        return -1;
    }

    return 0;
}
