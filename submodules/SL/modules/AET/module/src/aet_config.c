/**************************************************************************//**
 * 
 * 
 * 
 *****************************************************************************/
#include <AET/aet_config.h>

/* <auto.start.cdefs(AET_CONFIG_HEADER).source> */
#define __aet_config_STRINGIFY_NAME(_x) #_x
#define __aet_config_STRINGIFY_VALUE(_x) __aet_config_STRINGIFY_NAME(_x)
aet_config_settings_t aet_config_settings[] =
{
#ifdef AET_CONFIG_INCLUDE_LOGGING
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_INCLUDE_LOGGING), __aet_config_STRINGIFY_VALUE(AET_CONFIG_INCLUDE_LOGGING) },
#else
{ AET_CONFIG_INCLUDE_LOGGING(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_LOG_OPTIONS_DEFAULT
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_LOG_OPTIONS_DEFAULT), __aet_config_STRINGIFY_VALUE(AET_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ AET_CONFIG_LOG_OPTIONS_DEFAULT(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_LOG_BITS_DEFAULT
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_LOG_BITS_DEFAULT), __aet_config_STRINGIFY_VALUE(AET_CONFIG_LOG_BITS_DEFAULT) },
#else
{ AET_CONFIG_LOG_BITS_DEFAULT(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __aet_config_STRINGIFY_VALUE(AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ AET_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_PORTING_STDLIB
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_PORTING_STDLIB), __aet_config_STRINGIFY_VALUE(AET_CONFIG_PORTING_STDLIB) },
#else
{ AET_CONFIG_PORTING_STDLIB(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __aet_config_STRINGIFY_VALUE(AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_INCLUDE_UCLI
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_INCLUDE_UCLI), __aet_config_STRINGIFY_VALUE(AET_CONFIG_INCLUDE_UCLI) },
#else
{ AET_CONFIG_INCLUDE_UCLI(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_NO_PORT_STATUS_SUPPORT
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_NO_PORT_STATUS_SUPPORT), __aet_config_STRINGIFY_VALUE(AET_CONFIG_NO_PORT_STATUS_SUPPORT) },
#else
{ AET_CONFIG_NO_PORT_STATUS_SUPPORT(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef AET_CONFIG_PACKET_IN_QUEUE_DEPTH
    { __aet_config_STRINGIFY_NAME(AET_CONFIG_PACKET_IN_QUEUE_DEPTH), __aet_config_STRINGIFY_VALUE(AET_CONFIG_PACKET_IN_QUEUE_DEPTH) },
#else
{ AET_CONFIG_PACKET_IN_QUEUE_DEPTH(__aet_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __aet_config_STRINGIFY_VALUE
#undef __aet_config_STRINGIFY_NAME

const char*
aet_config_lookup(const char* setting)
{
    int i;
    for(i = 0; aet_config_settings[i].name; i++) {
        if(strcmp(aet_config_settings[i].name, setting)) {
            return aet_config_settings[i].value;
        }
    }
    return NULL;
}

int
aet_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; aet_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", aet_config_settings[i].name, aet_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(AET_CONFIG_HEADER).source> */

