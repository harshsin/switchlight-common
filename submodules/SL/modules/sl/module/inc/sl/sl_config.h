/**************************************************************************//**
 *
 * @file
 * @brief sl Configuration Header
 *
 * @addtogroup sl-config
 * @{
 *
 *****************************************************************************/
#ifndef __SL_CONFIG_H__
#define __SL_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef SL_INCLUDE_CUSTOM_CONFIG
#include <sl_custom_config.h>
#endif

/* <auto.start.cdefs(SL_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * SL_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef SL_CONFIG_INCLUDE_LOGGING
#define SL_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * SL_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef SL_CONFIG_LOG_OPTIONS_DEFAULT
#define SL_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * SL_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef SL_CONFIG_LOG_BITS_DEFAULT
#define SL_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * SL_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef SL_CONFIG_PORTING_STDLIB
#define SL_CONFIG_PORTING_STDLIB 1
#endif

/**
 * SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS SL_CONFIG_PORTING_STDLIB
#endif

/**
 * SL_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef SL_CONFIG_INCLUDE_UCLI
#define SL_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct sl_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} sl_config_settings_t;

/** Configuration settings table. */
/** sl_config_settings table. */
extern sl_config_settings_t sl_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* sl_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int sl_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(SL_CONFIG_HEADER).header> */

#include "sl_porting.h"

#endif /* __SL_CONFIG_H__ */
/* @} */
