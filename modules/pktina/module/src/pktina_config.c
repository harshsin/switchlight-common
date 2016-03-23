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

#include <pktina/pktina_config.h>

/* <auto.start.cdefs(PKTINA_CONFIG_HEADER).source> */
#define __pktina_config_STRINGIFY_NAME(_x) #_x
#define __pktina_config_STRINGIFY_VALUE(_x) __pktina_config_STRINGIFY_NAME(_x)
pktina_config_settings_t pktina_config_settings[] =
{
#ifdef PKTINA_CONFIG_INCLUDE_LOGGING
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_INCLUDE_LOGGING), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_INCLUDE_LOGGING) },
#else
{ PKTINA_CONFIG_INCLUDE_LOGGING(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_LOG_OPTIONS_DEFAULT
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_LOG_OPTIONS_DEFAULT), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ PKTINA_CONFIG_LOG_OPTIONS_DEFAULT(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_LOG_BITS_DEFAULT
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_LOG_BITS_DEFAULT), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_LOG_BITS_DEFAULT) },
#else
{ PKTINA_CONFIG_LOG_BITS_DEFAULT(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ PKTINA_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_PORTING_STDLIB
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_PORTING_STDLIB), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_PORTING_STDLIB) },
#else
{ PKTINA_CONFIG_PORTING_STDLIB(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ PKTINA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_INCLUDE_UCLI
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_INCLUDE_UCLI), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_INCLUDE_UCLI) },
#else
{ PKTINA_CONFIG_INCLUDE_UCLI(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef PKTINA_CONFIG_OF_PORTS_MAX
    { __pktina_config_STRINGIFY_NAME(PKTINA_CONFIG_OF_PORTS_MAX), __pktina_config_STRINGIFY_VALUE(PKTINA_CONFIG_OF_PORTS_MAX) },
#else
{ PKTINA_CONFIG_OF_PORTS_MAX(__pktina_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __pktina_config_STRINGIFY_VALUE
#undef __pktina_config_STRINGIFY_NAME

const char*
pktina_config_lookup(const char* setting)
{
    int i;
    for(i = 0; pktina_config_settings[i].name; i++) {
        if(strcmp(pktina_config_settings[i].name, setting)) {
            return pktina_config_settings[i].value;
        }
    }
    return NULL;
}

int
pktina_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; pktina_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", pktina_config_settings[i].name, pktina_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(PKTINA_CONFIG_HEADER).source> */

