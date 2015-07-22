/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <igmpa/igmpa_config.h>

/* <auto.start.cdefs(IGMPA_CONFIG_HEADER).source> */
#define __igmpa_config_STRINGIFY_NAME(_x) #_x
#define __igmpa_config_STRINGIFY_VALUE(_x) __igmpa_config_STRINGIFY_NAME(_x)
igmpa_config_settings_t igmpa_config_settings[] =
{
#ifdef IGMPA_CONFIG_INCLUDE_LOGGING
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_INCLUDE_LOGGING), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_INCLUDE_LOGGING) },
#else
{ IGMPA_CONFIG_INCLUDE_LOGGING(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_LOG_OPTIONS_DEFAULT
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_LOG_OPTIONS_DEFAULT), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ IGMPA_CONFIG_LOG_OPTIONS_DEFAULT(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_LOG_BITS_DEFAULT
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_LOG_BITS_DEFAULT), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_LOG_BITS_DEFAULT) },
#else
{ IGMPA_CONFIG_LOG_BITS_DEFAULT(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ IGMPA_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_PORTING_STDLIB
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_PORTING_STDLIB), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_PORTING_STDLIB) },
#else
{ IGMPA_CONFIG_PORTING_STDLIB(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ IGMPA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IGMPA_CONFIG_INCLUDE_UCLI
    { __igmpa_config_STRINGIFY_NAME(IGMPA_CONFIG_INCLUDE_UCLI), __igmpa_config_STRINGIFY_VALUE(IGMPA_CONFIG_INCLUDE_UCLI) },
#else
{ IGMPA_CONFIG_INCLUDE_UCLI(__igmpa_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __igmpa_config_STRINGIFY_VALUE
#undef __igmpa_config_STRINGIFY_NAME

const char*
igmpa_config_lookup(const char* setting)
{
    int i;
    for(i = 0; igmpa_config_settings[i].name; i++) {
        if(strcmp(igmpa_config_settings[i].name, setting)) {
            return igmpa_config_settings[i].value;
        }
    }
    return NULL;
}

int
igmpa_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; igmpa_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", igmpa_config_settings[i].name, igmpa_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(IGMPA_CONFIG_HEADER).source> */

