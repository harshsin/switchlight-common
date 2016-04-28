/**************************************************************************//**
 *
 * @file
 * @brief icmpv6 Configuration Header
 *
 * @addtogroup icmpv6-config
 * @{
 *
 *****************************************************************************/
#ifndef __ICMPV6_CONFIG_H__
#define __ICMPV6_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef ICMPV6_INCLUDE_CUSTOM_CONFIG
#include <icmpv6_custom_config.h>
#endif

/* <auto.start.cdefs(ICMPV6_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * ICMPV6_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef ICMPV6_CONFIG_INCLUDE_LOGGING
#define ICMPV6_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT
#define ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * ICMPV6_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef ICMPV6_CONFIG_LOG_BITS_DEFAULT
#define ICMPV6_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * ICMPV6_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef ICMPV6_CONFIG_PORTING_STDLIB
#define ICMPV6_CONFIG_PORTING_STDLIB 1
#endif

/**
 * ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS ICMPV6_CONFIG_PORTING_STDLIB
#endif

/**
 * ICMPV6_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef ICMPV6_CONFIG_INCLUDE_UCLI
#define ICMPV6_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct icmpv6_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} icmpv6_config_settings_t;

/** Configuration settings table. */
/** icmpv6_config_settings table. */
extern icmpv6_config_settings_t icmpv6_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* icmpv6_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int icmpv6_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(ICMPV6_CONFIG_HEADER).header> */

#include "icmpv6_porting.h"

#endif /* __ICMPV6_CONFIG_H__ */
/* @} */
