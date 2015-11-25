/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * PIM expectation gentable data structures and prototypes.
 */

#ifndef __PIM_EXPECT_TABLE_H__
#define __PIM_EXPECT_TABLE_H__

#include <BigHash/bighash.h>
#include <timer_wheel/timer_wheel.h>
#include "igmpa_int.h"


typedef struct pim_expect_key_s {
    char name[IGMP_NAME_LEN];  /* name of igmp_rx_port_group */
    uint16_t vlan_vid;
} pim_expect_key_t;

/* pim expectation gentable entry */
typedef struct pim_expect_entry_s {
    bighash_entry_t hash_entry;
    timer_wheel_entry_t timer_entry;
    pim_expect_key_t key;
    /* entry has no associated values */
    /* stats */
    indigo_time_t time_last_hit;  /* to compute idle_time */
    uint64_t rx_packets;
    uint64_t tx_packets;  /* idle notifications sent */
} pim_expect_entry_t;


pim_expect_entry_t *igmpa_pim_expect_lookup(char name[], uint16_t vlan_vid);
void igmpa_pim_expect_reschedule(pim_expect_entry_t *pim_expect_entry,
                                 uint64_t new_deadline);
void igmpa_pim_expect_stats_clear(void);
void igmpa_pim_expect_stats_show(aim_pvs_t *pvs);
void igmpa_pim_expect_table_init(void);
void igmpa_pim_expect_table_finish(void);


#endif /* __PIM_EXPECT_TABLE_H__ */
