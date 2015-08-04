/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * general query expectation gentable data structures and prototypes.
 */

#ifndef __GQ_EXPECT_TABLE_H__
#define __GQ_EXPECT_TABLE_H__

#include <BigHash/bighash.h>
#include <timer_wheel/timer_wheel.h>
#include "igmpa_int.h"


typedef struct gq_expect_key_s {
    char name[IGMP_NAME_LEN];  /* name of igmp_rx_port_group */
    uint16_t vlan_vid;
} gq_expect_key_t;

/* general query expectation gentable entry */
typedef struct gq_expect_entry_s {
    bighash_entry_t hash_entry;
    timer_wheel_entry_t timer_entry;
    gq_expect_key_t key;
    /* entry has no associated values */
    /* stats */
    indigo_time_t time_last_hit;  /* to compute idle_time */
    uint64_t rx_packets;
} gq_expect_entry_t;


gq_expect_entry_t *igmpa_gq_expect_lookup(char name[], uint16_t vlan_vid);
void igmpa_gq_expect_reschedule(gq_expect_entry_t *gq_expect_entry,
                                uint64_t new_deadline);
void igmpa_gq_expect_stats_show(aim_pvs_t *pvs);
void igmpa_gq_expect_table_init(void);
void igmpa_gq_expect_table_finish(void);


#endif /* __GQ_EXPECT_TABLE_H__ */
