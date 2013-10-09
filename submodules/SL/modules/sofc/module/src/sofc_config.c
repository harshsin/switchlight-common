/**************************************************************************//**
 * 
 * 
 * 
 *****************************************************************************/
#include <sofc/sofc_config.h>

/* <auto.start.cdefs(SOFC_CONFIG_HEADER).source> */
#define __sofc_config_STRINGIFY_NAME(_x) #_x
#define __sofc_config_STRINGIFY_VALUE(_x) __sofc_config_STRINGIFY_NAME(_x)
sofc_config_settings_t sofc_config_settings[] =
{
#ifdef SOFC_CONFIG_INCLUDE_LOGGING
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_INCLUDE_LOGGING), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_INCLUDE_LOGGING) },
#else
{ SOFC_CONFIG_INCLUDE_LOGGING(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_LOG_OPTIONS_DEFAULT
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_LOG_OPTIONS_DEFAULT), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ SOFC_CONFIG_LOG_OPTIONS_DEFAULT(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_LOG_BITS_DEFAULT
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_LOG_BITS_DEFAULT), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_LOG_BITS_DEFAULT) },
#else
{ SOFC_CONFIG_LOG_BITS_DEFAULT(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ SOFC_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_PORTING_STDLIB
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_PORTING_STDLIB), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_PORTING_STDLIB) },
#else
{ SOFC_CONFIG_PORTING_STDLIB(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef SOFC_CONFIG_INCLUDE_UCLI
    { __sofc_config_STRINGIFY_NAME(SOFC_CONFIG_INCLUDE_UCLI), __sofc_config_STRINGIFY_VALUE(SOFC_CONFIG_INCLUDE_UCLI) },
#else
{ SOFC_CONFIG_INCLUDE_UCLI(__sofc_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __sofc_config_STRINGIFY_VALUE
#undef __sofc_config_STRINGIFY_NAME

const char*
sofc_config_lookup(const char* setting)
{
    int i;
    for(i = 0; sofc_config_settings[i].name; i++) {
        if(strcmp(sofc_config_settings[i].name, setting)) {
            return sofc_config_settings[i].value;
        }
    }
    return NULL;
}

int
sofc_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; sofc_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", sofc_config_settings[i].name, sofc_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(SOFC_CONFIG_HEADER).source> */

