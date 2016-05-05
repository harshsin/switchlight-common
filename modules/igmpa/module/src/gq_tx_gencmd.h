/*
 * Copyright 2016, Big Switch Networks, Inc.
 *
 * general query tx generic command prototypes.
 */

#ifndef __GQ_TX_GENCMD_H__
#define __GQ_TX_GENCMD_H__


void igmpa_gq_tx_gencmd_stats_clear(void);
void igmpa_gq_tx_gencmd_stats_show(aim_pvs_t *pvs);
void igmpa_gq_tx_gencmd_init(void);
void igmpa_gq_tx_gencmd_finish(void);


#endif /* __GQ_TX_GENCMD_H__ */
