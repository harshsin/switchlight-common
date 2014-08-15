/******************************************************************************
 *
 *  /module/inc/portmanager.h
 *
 *  PortManager Public Interface
 *
 *****************************************************************************/


#ifndef __PORTMANAGER_H__
#define __PORTMANAGER_H__


#include <indigo/port_manager.h>
#include <indigo/forwarding.h>
#include <indigo/of_state_manager.h>


/**
 * Configuration structure for the configuration manager
 * @param periodic_event_ms Time out in ms for periodic event checking
 * @param flags Currently ignored
 */

typedef struct ind_port_config_s {
  unsigned of_version; /**< OF protocol version to use; see LOXI */
  unsigned max_ports; /**< Maximum number of OpenFlow ports */
} ind_port_config_t;

extern indigo_error_t ind_port_mac_addr_set(of_port_no_t port_no,
                                            of_mac_addr_t *mac_addr);
extern indigo_error_t ind_port_mac_addr_get(of_port_no_t port_no,
                                            of_mac_addr_t *mac_addr);

extern indigo_error_t ind_port_base_mac_addr_set(of_mac_addr_t *base_mac);

/**
 * Initialize the port manager
 * @param config The port manager specific config data
 * @returns An error code
 */

extern indigo_error_t ind_port_init(ind_port_config_t *config);

/**
 * Enable set/get for the port manager
 */

extern indigo_error_t ind_port_enable_set(int enable);
extern indigo_error_t ind_port_enable_get(int *enable);

/**
 * Disable/dealloc call for the port manager
 */

extern indigo_error_t ind_port_finish(void);

extern unsigned ind_port_packet_in_is_enabled(of_port_no_t of_port_num);

#endif /* __PORTMANAGER_H__ */
