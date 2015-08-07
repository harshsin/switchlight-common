/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * report tx gentable data structures and prototypes.
 */

#ifndef __REPORT_TX_TABLE_H__
#define __REPORT_TX_TABLE_H__

#include <timer_wheel/timer_wheel.h>
#include "igmpa_int.h"


typedef struct report_tx_key_s {
    char tx_port_group_name[IGMP_NAME_LEN];
    uint16_t vlan_vid;
    uint32_t ipv4;
} report_tx_key_t;

typedef struct report_tx_value_s {
    uint16_t vlan_vid_tx;
    uint32_t ipv4_src;
    of_mac_addr_t eth_src;
} report_tx_value_t;

/* report tx gentable entry */
typedef struct report_tx_entry_s {
    timer_wheel_entry_t timer_entry;
    report_tx_key_t key;
    report_tx_value_t value;
    /* stats */
    indigo_time_t time_last_hit;  /* to compute idle_time */
    uint64_t tx_packets;
} report_tx_entry_t;


void igmpa_report_tx_stats_show(aim_pvs_t *pvs);
void igmpa_report_tx_table_init(void);
void igmpa_report_tx_table_finish(void);


#endif /* __REPORT_TX_TABLE_H__ */
