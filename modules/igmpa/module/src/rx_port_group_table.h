/*
 * Copyright 2015, Big Switch Networks, Inc.
 *
 * rx port group gentable data structures and prototypes.
 */

#ifndef __RX_PORT_GROUP_TABLE_H__
#define __RX_PORT_GROUP_TABLE_H__

#include <BigHash/bighash.h>
#include "igmpa_int.h"


typedef struct rx_port_group_key_s {
    of_port_no_t port_no;    
} rx_port_group_key_t;

typedef struct rx_port_group_value_s {
    char name[IGMP_NAME_LEN];
} rx_port_group_value_t;

/* rx port group gentable entry */
typedef struct rx_port_group_entry_s {
    bighash_entry_t hash_entry;
    rx_port_group_key_t key;
    rx_port_group_value_t value;
} rx_port_group_entry_t;


rx_port_group_entry_t *igmpa_rx_port_group_lookup(of_port_no_t port_no);
void igmpa_rx_port_group_stats_show(aim_pvs_t *pvs);
void igmpa_rx_port_group_table_init(void);
void igmpa_rx_port_group_table_finish(void);


#endif /* __RX_PORT_GROUP_TABLE_H__ */
