/**************************************************************************//**
 *
 * @file
 * @brief IVS Configuration Definitions. 
 *
 *
 * @addtogroup ivs-config
 * @{
 *
 *****************************************************************************/
#ifndef __IVS_CONFIG_H__
#define __IVS_CONFIG_H__


#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef IVS_INCLUDE_CUSTOM_CONFIG
#include <ivs_custom_config.h>
#endif




/* <auto.start.cdefs(IVS_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * IVS_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef IVS_CONFIG_INCLUDE_LOGGING
#define IVS_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * IVS_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef IVS_CONFIG_LOG_OPTIONS_DEFAULT
#define IVS_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * IVS_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log options. */


#ifndef IVS_CONFIG_LOG_BITS_DEFAULT
#define IVS_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log options. */


#ifndef IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * IVS_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef IVS_CONFIG_PORTING_STDLIB
#define IVS_CONFIG_PORTING_STDLIB 1
#endif

/**
 * IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS IVS_CONFIG_PORTING_STDLIB
#endif

/**
 * IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT
 *
 * Include the default controller at startup if no controllers were specified. */


#ifndef IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT
#define IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT 1
#endif

/**
 * IVS_CONFIG_CONTROLLER_IP_DEFAULT
 *
 * Default controller IP for initial connections. */


#ifndef IVS_CONFIG_CONTROLLER_IP_DEFAULT
#define IVS_CONFIG_CONTROLLER_IP_DEFAULT "127.0.0.1"
#endif

/**
 * IVS_CONFIG_CONTROLLER_PORT_DEFAULT
 *
 * Default controller port for initial connections. */


#ifndef IVS_CONFIG_CONTROLLER_PORT_DEFAULT
#define IVS_CONFIG_CONTROLLER_PORT_DEFAULT 6633
#endif

/**
 * IVS_CONFIG_CONTROLLER_LISTEN_PORT
 *
 * Default port to listen for controller connections. */


#ifndef IVS_CONFIG_CONTROLLER_LISTEN_PORT
#define IVS_CONFIG_CONTROLLER_LISTEN_PORT 7000
#endif

/**
 * IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT
 *
 * Initialize with default veth interfaces. */


#ifndef IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT
#define IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT 1
#endif

/**
 * IVS_CONFIG_PORT_MAX_PORTS_DEFAULT
 *
 * Default maximum port count. */


#ifndef IVS_CONFIG_PORT_MAX_PORTS_DEFAULT
#define IVS_CONFIG_PORT_MAX_PORTS_DEFAULT 24
#endif

/**
 * IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT
 *
 * Default value for the maximum number of flow table entries. */


#ifndef IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT
#define IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT 16*1024
#endif

/**
 * IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT
 *
 * Default setting for whether to expire flows in state manager. */


#ifndef IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT
#define IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT 1
#endif

/**
 * IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT
 *
 * Default period for core statistics checks in milliseconds. */


#ifndef IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT
#define IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT 900
#endif

/**
 * IVS_CONFIG_DPID_DEFAULT
 *
 * Default datapath id. */


#ifndef IVS_CONFIG_DPID_DEFAULT
#define IVS_CONFIG_DPID_DEFAULT 1
#endif

/**
 * IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT
 *
 * Default value for the connection echo period in milliseconds. */


#ifndef IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT
#define IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT 10000
#endif

/**
 * IVS_CONFIG_CXN_RESET_ECHO_COUNT
 *
 * Default value for a connection's reset echo count. */


#ifndef IVS_CONFIG_CXN_RESET_ECHO_COUNT
#define IVS_CONFIG_CXN_RESET_ECHO_COUNT 3
#endif

/**
 * IVS_CONFIG_NSS_PORT_DEFAULT
 *
 * Default port for network cli connections. */


#ifndef IVS_CONFIG_NSS_PORT_DEFAULT
#define IVS_CONFIG_NSS_PORT_DEFAULT 4454
#endif

/**
 * IVS_CONFIG_CONSOLE_PROMPT_DEFAULT
 *
 * Default base prompt for the console cli if enabled. */


#ifndef IVS_CONFIG_CONSOLE_PROMPT_DEFAULT
#define IVS_CONFIG_CONSOLE_PROMPT_DEFAULT "ivs"
#endif

/**
 * IVS_CONFIG_INCLUDE_CONSOLE_CLI
 *
 * Include console CLI. */


#ifndef IVS_CONFIG_INCLUDE_CONSOLE_CLI
#define IVS_CONFIG_INCLUDE_CONSOLE_CLI 1
#endif

/**
 * IVS_CONFIG_INCLUDE_NETWORK_CLI
 *
 * Include network CLI. */


#ifndef IVS_CONFIG_INCLUDE_NETWORK_CLI
#define IVS_CONFIG_INCLUDE_NETWORK_CLI 1
#endif

/**
 * IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT
 *
 * For async threaded pkt-in queue handling, max queue length */


#ifndef IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT
#define IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT 128
#endif

/**
 * IVS_CONFIG_INCLUDE_WATCHDOG
 *
 * Include watchdog support. */


#ifndef IVS_CONFIG_INCLUDE_WATCHDOG
#define IVS_CONFIG_INCLUDE_WATCHDOG 1
#endif

/**
 * IVS_CONFIG_WATCHDOG_SECONDS
 *
 * Configure the default value for the watchdog timer (in seconds). */


#ifndef IVS_CONFIG_WATCHDOG_SECONDS
#define IVS_CONFIG_WATCHDOG_SECONDS 0
#endif

/**
 * IVS_CONFIG_INCLUDE_CXN_LOG
 *
 * Include a connection log ring buffer. */


#ifndef IVS_CONFIG_INCLUDE_CXN_LOG
#define IVS_CONFIG_INCLUDE_CXN_LOG 0
#endif

/**
 * IVS_CONFIG_CXN_LOG_SIZE
 *
 * The number of connection log entries to maintain. */


#ifndef IVS_CONFIG_CXN_LOG_SIZE
#define IVS_CONFIG_CXN_LOG_SIZE 0
#endif

/**
 * IVS_CONFIG_INCLUDE_STATUS_LOG
 *
 * Include a status log ring buffer. */


#ifndef IVS_CONFIG_INCLUDE_STATUS_LOG
#define IVS_CONFIG_INCLUDE_STATUS_LOG 1
#endif

/**
 * IVS_CONFIG_STATUS_LOG_SIZE
 *
 * The number of status log entries to maintain. */


#ifndef IVS_CONFIG_STATUS_LOG_SIZE
#define IVS_CONFIG_STATUS_LOG_SIZE 60
#endif

/**
 * IVS_CONFIG_STATUS_LOG_PERIOD_S
 *
 * Time between successive status logs, in seconds. */


#ifndef IVS_CONFIG_STATUS_LOG_PERIOD_S
#define IVS_CONFIG_STATUS_LOG_PERIOD_S 5
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct ivs_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} ivs_config_settings_t;

/** Configuration settings table. */
/** ivs_config_settings table. */
extern ivs_config_settings_t ivs_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* ivs_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int ivs_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(IVS_CONFIG_HEADER).header> */



#include "ivs_porting.h"
#endif /* __IVS_CONFIG_H__ */
/*@}*/
