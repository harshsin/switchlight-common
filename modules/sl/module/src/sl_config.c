/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sl/sl_config.h>

/* <auto.start.cdefs(SL_CONFIG_HEADER).source> */
#define __sl_config_STRINGIFY_NAME(_x) #_x
#define __sl_config_STRINGIFY_VALUE(_x) __sl_config_STRINGIFY_NAME(_x)
sl_config_settings_t sl_config_settings[] =
{
#ifdef SL_CONFIG_INCLUDE_LOGGING
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_INCLUDE_LOGGING), __sl_config_STRINGIFY_VALUE(SL_CONFIG_INCLUDE_LOGGING) },
#else
{ SL_CONFIG_INCLUDE_LOGGING(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_LOG_OPTIONS_DEFAULT
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_LOG_OPTIONS_DEFAULT), __sl_config_STRINGIFY_VALUE(SL_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ SL_CONFIG_LOG_OPTIONS_DEFAULT(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_LOG_BITS_DEFAULT
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_LOG_BITS_DEFAULT), __sl_config_STRINGIFY_VALUE(SL_CONFIG_LOG_BITS_DEFAULT) },
#else
{ SL_CONFIG_LOG_BITS_DEFAULT(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __sl_config_STRINGIFY_VALUE(SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ SL_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_PORTING_STDLIB
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_PORTING_STDLIB), __sl_config_STRINGIFY_VALUE(SL_CONFIG_PORTING_STDLIB) },
#else
{ SL_CONFIG_PORTING_STDLIB(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __sl_config_STRINGIFY_VALUE(SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SL_CONFIG_INCLUDE_UCLI
    { __sl_config_STRINGIFY_NAME(SL_CONFIG_INCLUDE_UCLI), __sl_config_STRINGIFY_VALUE(SL_CONFIG_INCLUDE_UCLI) },
#else
{ SL_CONFIG_INCLUDE_UCLI(__sl_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __sl_config_STRINGIFY_VALUE
#undef __sl_config_STRINGIFY_NAME

const char*
sl_config_lookup(const char* setting)
{
    int i;
    for(i = 0; sl_config_settings[i].name; i++) {
        if(strcmp(sl_config_settings[i].name, setting)) {
            return sl_config_settings[i].value;
        }
    }
    return NULL;
}

int
sl_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; sl_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", sl_config_settings[i].name, sl_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(SL_CONFIG_HEADER).source> */

