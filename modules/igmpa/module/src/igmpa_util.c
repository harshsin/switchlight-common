/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * Utility functions for IGMP agent implementation.
 */

#include <AIM/aim.h>
#include <indigo/indigo.h>
#include <arpa/inet.h>

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


