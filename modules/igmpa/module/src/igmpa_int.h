/**************************************************************************//**
 *
 * igmpa Internal Header
 *
 *****************************************************************************/
#ifndef __IGMPA_INT_H__
#define __IGMPA_INT_H__

#include <igmpa/igmpa_config.h>
#include "igmpa_log.h"


/* maximum length of name TLVs */
#define IGMP_NAME_LEN 64


/* utility macros */
#define IS_IPV4_MULTICAST_ADDR(x) ((x & 0xf0000000) == 0xe0000000)

/* utility functions */
indigo_error_t parse_name_tlv(of_object_t *tlv, char *dst_name);
uint16_t sum16(uint8_t* data, int len);


void igmpa_stats_show(aim_pvs_t *pvs);

#endif /* __IGMPA_INT_H__ */
