/****************************************************************
 *
 *        Copyright 2016, Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

/*************************************************************//**
 *
 * @file
 * @brief macblaster Configuration Header
 *
 * @addtogroup macblaster-config
 * @{
 *
 ****************************************************************/
#ifndef __MACBLASTER_CONFIG_H__
#define __MACBLASTER_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef MACBLASTER_INCLUDE_CUSTOM_CONFIG
#include <macblaster_custom_config.h>
#endif

/* <auto.start.cdefs(MACBLASTER_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * MACBLASTER_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef MACBLASTER_CONFIG_INCLUDE_LOGGING
#define MACBLASTER_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT
#define MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * MACBLASTER_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef MACBLASTER_CONFIG_LOG_BITS_DEFAULT
#define MACBLASTER_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * MACBLASTER_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef MACBLASTER_CONFIG_PORTING_STDLIB
#define MACBLASTER_CONFIG_PORTING_STDLIB 1
#endif

/**
 * MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS MACBLASTER_CONFIG_PORTING_STDLIB
#endif

/**
 * MACBLASTER_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef MACBLASTER_CONFIG_INCLUDE_UCLI
#define MACBLASTER_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct macblaster_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} macblaster_config_settings_t;

/** Configuration settings table. */
/** macblaster_config_settings table. */
extern macblaster_config_settings_t macblaster_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* macblaster_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int macblaster_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(MACBLASTER_CONFIG_HEADER).header> */

#include "macblaster_porting.h"

#endif /* __MACBLASTER_CONFIG_H__ */
/* @} */
