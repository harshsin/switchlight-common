/****************************************************************
 *
 *        Copyright 2015, Big Switch Networks, Inc.
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
 * @brief pdua Configuration Header
 *
 * @addtogroup pdua-config
 * @{
 *
 ****************************************************************/
#ifndef __PDUA_CONFIG_H__
#define __PDUA_CONFIG_H__

#include <slshared/slshared_config.h>

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef PDUA_INCLUDE_CUSTOM_CONFIG
#include <pdua_custom_config.h>
#endif

/* <auto.start.cdefs(PDUA_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * PDUA_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef PDUA_CONFIG_INCLUDE_LOGGING
#define PDUA_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * PDUA_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef PDUA_CONFIG_LOG_OPTIONS_DEFAULT
#define PDUA_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * PDUA_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef PDUA_CONFIG_LOG_BITS_DEFAULT
#define PDUA_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * PDUA_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef PDUA_CONFIG_PORTING_STDLIB
#define PDUA_CONFIG_PORTING_STDLIB 1
#endif

/**
 * PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS PDUA_CONFIG_PORTING_STDLIB
#endif

/**
 * PDUA_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef PDUA_CONFIG_INCLUDE_UCLI
#define PDUA_CONFIG_INCLUDE_UCLI 0
#endif

/**
 * PDUA_CONFIG_OF_PORTS_MAX
 *
 * Maximum number of OF ports. */


#ifndef PDUA_CONFIG_OF_PORTS_MAX
#define PDUA_CONFIG_OF_PORTS_MAX SLSHARED_CONFIG_OF_PORT_MAX
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct pdua_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} pdua_config_settings_t;

/** Configuration settings table. */
/** pdua_config_settings table. */
extern pdua_config_settings_t pdua_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* pdua_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int pdua_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(PDUA_CONFIG_HEADER).header> */

#include "pdua_porting.h"

#endif /* __PDUA_CONFIG_H__ */
/* @} */
