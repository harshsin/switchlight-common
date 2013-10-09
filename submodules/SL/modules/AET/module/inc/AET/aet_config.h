/**************************************************************************//**
 * 
 * @file
 * @brief AET Configuration Header
 * 
 * @addtogroup aet-config
 * @{
 * 
 *****************************************************************************/
#ifndef __AET_CONFIG_H__
#define __AET_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef AET_INCLUDE_CUSTOM_CONFIG
#include <aet_custom_config.h>
#endif

/* <auto.start.cdefs(AET_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * AET_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef AET_CONFIG_INCLUDE_LOGGING
#define AET_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * AET_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef AET_CONFIG_LOG_OPTIONS_DEFAULT
#define AET_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * AET_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef AET_CONFIG_LOG_BITS_DEFAULT
#define AET_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * AET_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef AET_CONFIG_PORTING_STDLIB
#define AET_CONFIG_PORTING_STDLIB 1
#endif

/**
 * AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS AET_CONFIG_PORTING_STDLIB
#endif

/**
 * AET_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef AET_CONFIG_INCLUDE_UCLI
#define AET_CONFIG_INCLUDE_UCLI 0
#endif

/**
 * AET_CONFIG_NO_PORT_STATUS_SUPPORT
 *
 * If 1, do not allocate support for port status updates. */


#ifndef AET_CONFIG_NO_PORT_STATUS_SUPPORT
#define AET_CONFIG_NO_PORT_STATUS_SUPPORT 0
#endif

/**
 * AET_CONFIG_PACKET_IN_QUEUE_DEPTH
 *
 * Depth of packet-in message queue. */


#ifndef AET_CONFIG_PACKET_IN_QUEUE_DEPTH
#define AET_CONFIG_PACKET_IN_QUEUE_DEPTH 256
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct aet_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} aet_config_settings_t;

/** Configuration settings table. */
/** aet_config_settings table. */
extern aet_config_settings_t aet_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* aet_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int aet_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(AET_CONFIG_HEADER).header> */

#include "aet_porting.h"

#endif /* __AET_CONFIG_H__ */
/* @} */
