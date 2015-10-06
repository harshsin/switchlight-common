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

#include <pdua/pdua_config.h>

/* <auto.start.cdefs(PDUA_CONFIG_HEADER).source> */
#define __pdua_config_STRINGIFY_NAME(_x) #_x
#define __pdua_config_STRINGIFY_VALUE(_x) __pdua_config_STRINGIFY_NAME(_x)
pdua_config_settings_t pdua_config_settings[] =
{
#ifdef PDUA_CONFIG_INCLUDE_LOGGING
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_INCLUDE_LOGGING), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_INCLUDE_LOGGING) },
#else
{ PDUA_CONFIG_INCLUDE_LOGGING(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_LOG_OPTIONS_DEFAULT
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_LOG_OPTIONS_DEFAULT), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ PDUA_CONFIG_LOG_OPTIONS_DEFAULT(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_LOG_BITS_DEFAULT
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_LOG_BITS_DEFAULT), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_LOG_BITS_DEFAULT) },
#else
{ PDUA_CONFIG_LOG_BITS_DEFAULT(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ PDUA_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_PORTING_STDLIB
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_PORTING_STDLIB), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_PORTING_STDLIB) },
#else
{ PDUA_CONFIG_PORTING_STDLIB(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_INCLUDE_UCLI
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_INCLUDE_UCLI), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_INCLUDE_UCLI) },
#else
{ PDUA_CONFIG_INCLUDE_UCLI(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PDUA_CONFIG_OF_PORTS_MAX
    { __pdua_config_STRINGIFY_NAME(PDUA_CONFIG_OF_PORTS_MAX), __pdua_config_STRINGIFY_VALUE(PDUA_CONFIG_OF_PORTS_MAX) },
#else
{ PDUA_CONFIG_OF_PORTS_MAX(__pdua_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __pdua_config_STRINGIFY_VALUE
#undef __pdua_config_STRINGIFY_NAME

const char*
pdua_config_lookup(const char* setting)
{
    int i;
    for(i = 0; pdua_config_settings[i].name; i++) {
        if(strcmp(pdua_config_settings[i].name, setting)) {
            return pdua_config_settings[i].value;
        }
    }
    return NULL;
}

int
pdua_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; pdua_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", pdua_config_settings[i].name, pdua_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(PDUA_CONFIG_HEADER).source> */

