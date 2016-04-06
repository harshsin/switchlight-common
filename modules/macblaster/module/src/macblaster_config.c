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

#include <macblaster/macblaster_config.h>

/* <auto.start.cdefs(MACBLASTER_CONFIG_HEADER).source> */
#define __macblaster_config_STRINGIFY_NAME(_x) #_x
#define __macblaster_config_STRINGIFY_VALUE(_x) __macblaster_config_STRINGIFY_NAME(_x)
macblaster_config_settings_t macblaster_config_settings[] =
{
#ifdef MACBLASTER_CONFIG_INCLUDE_LOGGING
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_INCLUDE_LOGGING), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_INCLUDE_LOGGING) },
#else
{ MACBLASTER_CONFIG_INCLUDE_LOGGING(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_LOG_BITS_DEFAULT
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_LOG_BITS_DEFAULT), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_LOG_BITS_DEFAULT) },
#else
{ MACBLASTER_CONFIG_LOG_BITS_DEFAULT(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_PORTING_STDLIB
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_PORTING_STDLIB), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_PORTING_STDLIB) },
#else
{ MACBLASTER_CONFIG_PORTING_STDLIB(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_INCLUDE_UCLI
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_INCLUDE_UCLI), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_INCLUDE_UCLI) },
#else
{ MACBLASTER_CONFIG_INCLUDE_UCLI(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef MACBLASTER_CONFIG_OF_PORTS_MAX
    { __macblaster_config_STRINGIFY_NAME(MACBLASTER_CONFIG_OF_PORTS_MAX), __macblaster_config_STRINGIFY_VALUE(MACBLASTER_CONFIG_OF_PORTS_MAX) },
#else
{ MACBLASTER_CONFIG_OF_PORTS_MAX(__macblaster_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __macblaster_config_STRINGIFY_VALUE
#undef __macblaster_config_STRINGIFY_NAME

const char*
macblaster_config_lookup(const char* setting)
{
    int i;
    for(i = 0; macblaster_config_settings[i].name; i++) {
        if(strcmp(macblaster_config_settings[i].name, setting)) {
            return macblaster_config_settings[i].value;
        }
    }
    return NULL;
}

int
macblaster_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; macblaster_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", macblaster_config_settings[i].name, macblaster_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(MACBLASTER_CONFIG_HEADER).source> */

