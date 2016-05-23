/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <collecta/collecta_config.h>

/* <auto.start.cdefs(COLLECTA_CONFIG_HEADER).source> */
#define __collecta_config_STRINGIFY_NAME(_x) #_x
#define __collecta_config_STRINGIFY_VALUE(_x) __collecta_config_STRINGIFY_NAME(_x)
collecta_config_settings_t collecta_config_settings[] =
{
#ifdef COLLECTA_CONFIG_INCLUDE_LOGGING
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_INCLUDE_LOGGING), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_INCLUDE_LOGGING) },
#else
{ COLLECTA_CONFIG_INCLUDE_LOGGING(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ COLLECTA_CONFIG_LOG_OPTIONS_DEFAULT(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_LOG_BITS_DEFAULT
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_LOG_BITS_DEFAULT), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_LOG_BITS_DEFAULT) },
#else
{ COLLECTA_CONFIG_LOG_BITS_DEFAULT(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ COLLECTA_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_PORTING_STDLIB
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_PORTING_STDLIB), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_PORTING_STDLIB) },
#else
{ COLLECTA_CONFIG_PORTING_STDLIB(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef COLLECTA_CONFIG_INCLUDE_UCLI
    { __collecta_config_STRINGIFY_NAME(COLLECTA_CONFIG_INCLUDE_UCLI), __collecta_config_STRINGIFY_VALUE(COLLECTA_CONFIG_INCLUDE_UCLI) },
#else
{ COLLECTA_CONFIG_INCLUDE_UCLI(__collecta_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __collecta_config_STRINGIFY_VALUE
#undef __collecta_config_STRINGIFY_NAME

const char*
collecta_config_lookup(const char* setting)
{
    int i;
    for(i = 0; collecta_config_settings[i].name; i++) {
        if(strcmp(collecta_config_settings[i].name, setting)) {
            return collecta_config_settings[i].value;
        }
    }
    return NULL;
}

int
collecta_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; collecta_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", collecta_config_settings[i].name, collecta_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(COLLECTA_CONFIG_HEADER).source> */

