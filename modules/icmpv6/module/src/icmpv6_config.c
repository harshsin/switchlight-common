/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <icmpv6/icmpv6_config.h>

/* <auto.start.cdefs(ICMPV6_CONFIG_HEADER).source> */
#define __icmpv6_config_STRINGIFY_NAME(_x) #_x
#define __icmpv6_config_STRINGIFY_VALUE(_x) __icmpv6_config_STRINGIFY_NAME(_x)
icmpv6_config_settings_t icmpv6_config_settings[] =
{
#ifdef ICMPV6_CONFIG_INCLUDE_LOGGING
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_INCLUDE_LOGGING), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_INCLUDE_LOGGING) },
#else
{ ICMPV6_CONFIG_INCLUDE_LOGGING(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ ICMPV6_CONFIG_LOG_OPTIONS_DEFAULT(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_LOG_BITS_DEFAULT
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_LOG_BITS_DEFAULT), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_LOG_BITS_DEFAULT) },
#else
{ ICMPV6_CONFIG_LOG_BITS_DEFAULT(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ ICMPV6_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_PORTING_STDLIB
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_PORTING_STDLIB), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_PORTING_STDLIB) },
#else
{ ICMPV6_CONFIG_PORTING_STDLIB(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ICMPV6_CONFIG_INCLUDE_UCLI
    { __icmpv6_config_STRINGIFY_NAME(ICMPV6_CONFIG_INCLUDE_UCLI), __icmpv6_config_STRINGIFY_VALUE(ICMPV6_CONFIG_INCLUDE_UCLI) },
#else
{ ICMPV6_CONFIG_INCLUDE_UCLI(__icmpv6_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __icmpv6_config_STRINGIFY_VALUE
#undef __icmpv6_config_STRINGIFY_NAME

const char*
icmpv6_config_lookup(const char* setting)
{
    int i;
    for(i = 0; icmpv6_config_settings[i].name; i++) {
        if(strcmp(icmpv6_config_settings[i].name, setting)) {
            return icmpv6_config_settings[i].value;
        }
    }
    return NULL;
}

int
icmpv6_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; icmpv6_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", icmpv6_config_settings[i].name, icmpv6_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(ICMPV6_CONFIG_HEADER).source> */

