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
indigo_error_t igmpa_parse_name_tlv(of_object_t *tlv, char *dst_name);
indigo_error_t igmpa_refkey_name_get(of_object_t* refkey, char *name);
uint16_t igmpa_sum16(uint8_t* data, int len);

typedef struct igmpa_pkt_params_s {
    uint8_t *eth_src;
    uint8_t *eth_dst;
    uint16_t vlan_vid;
    uint32_t ipv4_src;
    uint32_t ipv4_dst;
    uint8_t igmp_type;
    uint8_t igmp_max_resp_time;
    uint32_t igmp_group_addr;
    of_port_no_t output_port_no;
} igmpa_pkt_params_t;
int igmpa_send_igmp_packet(igmpa_pkt_params_t *params);

void igmpa_stats_clear(void);
void igmpa_stats_show(aim_pvs_t *pvs);

#endif /* __IGMPA_INT_H__ */
