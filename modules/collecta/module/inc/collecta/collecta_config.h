/**************************************************************************//**
 *
 * @file
 * @brief collecta Configuration Header
 *
 * @addtogroup collecta-config
 * @{
 *
 *****************************************************************************/
#ifndef __COLLECTA_CONFIG_H__
#define __COLLECTA_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef COLLECTA_INCLUDE_CUSTOM_CONFIG
#include <collecta_custom_config.h>
#endif

/* <auto.start.cdefs(COLLECTA_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * COLLECTA_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef COLLECTA_CONFIG_INCLUDE_LOGGING
#define COLLECTA_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT
#define COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * COLLECTA_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef COLLECTA_CONFIG_LOG_BITS_DEFAULT
#define COLLECTA_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * COLLECTA_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef COLLECTA_CONFIG_PORTING_STDLIB
#define COLLECTA_CONFIG_PORTING_STDLIB 1
#endif

/**
 * COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS COLLECTA_CONFIG_PORTING_STDLIB
#endif

/**
 * COLLECTA_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef COLLECTA_CONFIG_INCLUDE_UCLI
#define COLLECTA_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct collecta_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} collecta_config_settings_t;

/** Configuration settings table. */
/** collecta_config_settings table. */
extern collecta_config_settings_t collecta_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* collecta_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int collecta_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(COLLECTA_CONFIG_HEADER).header> */

#include "collecta_porting.h"

#endif /* __COLLECTA_CONFIG_H__ */
/* @} */
