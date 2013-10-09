/**************************************************************************//**
 * 
 * @file
 * @brief sofc Configuration Header
 * 
 * @addtogroup sofc-config
 * @{
 * 
 *****************************************************************************/
#ifndef __SOFC_CONFIG_H__
#define __SOFC_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef SOFC_INCLUDE_CUSTOM_CONFIG
#include <sofc_custom_config.h>
#endif

/* <auto.start.cdefs(SOFC_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * SOFC_CONFIG_INCLUDE_LOGGING
 * 
 * Include or exclude logging. */


#ifndef SOFC_CONFIG_INCLUDE_LOGGING
#define SOFC_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * SOFC_CONFIG_LOG_OPTIONS_DEFAULT
 * 
 * Default enabled log options. */


#ifndef SOFC_CONFIG_LOG_OPTIONS_DEFAULT
#define SOFC_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * SOFC_CONFIG_LOG_BITS_DEFAULT
 * 
 * Default enabled log bits. */


#ifndef SOFC_CONFIG_LOG_BITS_DEFAULT
#define SOFC_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 * 
 * Default enabled custom log bits. */


#ifndef SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * SOFC_CONFIG_PORTING_STDLIB
 * 
 * Default all porting macros to use the C standard libraries. */


#ifndef SOFC_CONFIG_PORTING_STDLIB
#define SOFC_CONFIG_PORTING_STDLIB 1
#endif

/**
 * SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 * 
 * Include standard library headers for stdlib porting macros. */


#ifndef SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS SOFC_CONFIG_PORTING_STDLIB
#endif

/**
 * SOFC_CONFIG_INCLUDE_UCLI
 * 
 * Include generic uCli support. */


#ifndef SOFC_CONFIG_INCLUDE_UCLI
#define SOFC_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct sofc_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} sofc_config_settings_t;

/** Configuration settings table. */
/** sofc_config_settings table. */
extern sofc_config_settings_t sofc_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup. 
 */
const char* sofc_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream. 
 */
int sofc_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(SOFC_CONFIG_HEADER).header> */

#include "sofc_porting.h"

#endif /* __SOFC_CONFIG_H__ */
/* @} */
