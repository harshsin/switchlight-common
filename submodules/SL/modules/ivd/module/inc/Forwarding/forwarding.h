/******************************************************************************
 *
 *  /module/inc/forwarding.h
 *
 *  Forwarding Public Interface
 *
 *****************************************************************************/


#ifndef __FORWARDING_H__
#define __FORWARDING_H__


#include <indigo/port_manager.h>
#include <indigo/forwarding.h>
#include <indigo/error.h>
#include <indigo/of_state_manager.h>


typedef struct {
  unsigned of_version;
  unsigned max_flows;
} ind_fwd_config_t;

extern indigo_error_t ind_fwd_init(ind_fwd_config_t *config);

/**
 * Enable set/get for forwarding
 */

extern indigo_error_t ind_fwd_enable_set(int enable);
extern indigo_error_t ind_fwd_enable_get(int *enable);

/**
 * Disable/dealloc call for the forwarding module
 */

extern indigo_error_t ind_fwd_finish(void);

/**
 * Stats for packet in
 *
 * These are shared so that the port manager can get stats related
 * to OF_PORT_DEST_CONTROLLER
 */

extern uint64_t ind_fwd_packet_in_packets;
extern uint64_t ind_fwd_packet_in_bytes;
extern uint64_t ind_fwd_packet_out_packets;
extern uint64_t ind_fwd_packet_out_bytes;

#endif /* __FORWARDING_H__ */
