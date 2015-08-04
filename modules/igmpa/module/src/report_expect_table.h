/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * report expectation gentable data structures and prototypes.
 */

#ifndef __REPORT_EXPECT_TABLE_H__
#define __REPORT_EXPECT_TABLE_H__

#include <BigHash/bighash.h>
#include <timer_wheel/timer_wheel.h>
#include "igmpa_int.h"


typedef struct report_expect_key_s {
    char name[IGMP_NAME_LEN];  /* name of igmp_rx_port_group */
    uint16_t vlan_vid;
    uint32_t ipv4;  /* multicast address */
} report_expect_key_t;

/* report expectation gentable entry */
typedef struct report_expect_entry_s {
    bighash_entry_t hash_entry;
    timer_wheel_entry_t timer_entry;
    report_expect_key_t key;
    /* stats */
    indigo_time_t time_last_hit;  /* to compute idle_time */
    uint64_t rx_packets;
} report_expect_entry_t;


report_expect_entry_t *
igmpa_report_expect_lookup(char name[], uint16_t vlan_vid, uint32_t ipv4);
void igmpa_report_expect_reschedule(report_expect_entry_t *report_expect_entry,
                                    uint64_t new_deadline);
void igmpa_report_expect_stats_show(aim_pvs_t *pvs);
void igmpa_report_expect_table_init(void);
void igmpa_report_expect_table_finish(void);


#endif /* __REPORT_EXPECT_TABLE_H__ */
