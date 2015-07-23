/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * general query tx gentable data structures and prototypes.
 */

#ifndef __GQ_TX_TABLE_H__
#define __GQ_TX_TABLE_H__

#include <timer_wheel/timer_wheel.h>
#include "igmpa_int.h"


typedef struct gq_tx_key_s {
    of_port_no_t port_no;  /* name resolved to port */
    uint16_t vlan_vid;
} gq_tx_key_t;

typedef struct gq_tx_value_s {
    uint16_t vlan_vid_tx;
    uint32_t ipv4_src;
    of_mac_addr_t eth_src;
} gq_tx_value_t;

/* report tx gentable entry */
typedef struct gq_tx_entry_s {
    timer_wheel_entry_t timer_entry;
    gq_tx_key_t key;
    gq_tx_value_t value;
    /* stats */
    indigo_time_t time_last_hit;  /* to compute idle_time */
    uint64_t tx_packets;
} gq_tx_entry_t;


void gq_tx_stats_show(aim_pvs_t *pvs);
void gq_tx_table_init(void);
void gq_tx_table_finish(void);


#endif /* __GQ_TX_TABLE_H__ */
