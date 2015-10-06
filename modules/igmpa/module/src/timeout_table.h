/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * timeout gentable data structures and prototypes.
 */

#ifndef __TIMEOUT_TABLE_H__
#define __TIMEOUT_TABLE_H__

#include "igmpa_int.h"


typedef struct timeout_key_s {
    char name[IGMP_NAME_LEN];
} timeout_key_t;

typedef struct timeout_value_s {
    uint32_t timeout;
} timeout_value_t;

/* timeout gentable entry */
typedef struct timeout_entry_s {
    timeout_key_t key;
    timeout_value_t value;
} timeout_entry_t;


extern uint32_t igmpa_gq_expect_timeout;
extern uint32_t igmpa_report_expect_timeout;
extern uint32_t igmpa_gq_tx_timeout;
extern uint32_t igmpa_report_tx_timeout;
extern uint32_t igmpa_pim_expect_timeout;


void igmpa_timeout_stats_clear(void);
void igmpa_timeout_stats_show(aim_pvs_t *pvs);
void igmpa_timeout_table_init(void);
void igmpa_timeout_table_finish(void);


#endif /* __TIMEOUT_TABLE_H__ */
