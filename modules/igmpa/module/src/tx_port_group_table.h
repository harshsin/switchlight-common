/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * tx port group gentable data structures and prototypes.
 */

#ifndef __TX_PORT_GROUP_TABLE_H__
#define __TX_PORT_GROUP_TABLE_H__

#include <BigHash/bighash.h>
#include "igmpa_int.h"


typedef struct tx_port_group_key_s {
    char name[IGMP_NAME_LEN];
} tx_port_group_key_t;

typedef struct tx_port_group_value_s {
    of_port_no_t port_no;
} tx_port_group_value_t;

/* tx port group gentable entry */
typedef struct tx_port_group_entry_s {
    bighash_entry_t hash_entry;
    tx_port_group_key_t key;
    tx_port_group_value_t value;
} tx_port_group_entry_t;


of_port_no_t tx_port_group_lookup(uint16_t table_id, of_object_t *key);
void tx_port_group_stats_show(aim_pvs_t *pvs);
void tx_port_group_table_init(void);
void tx_port_group_table_finish(void);


#endif /* __TX_PORT_GROUP_TABLE_H__ */
