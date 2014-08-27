#include <ivd/ivd_config.h>

/* <auto.start.cdefs(IVD_CONFIG_HEADER).source> */
#define __ivd_config_STRINGIFY_NAME(_x) #_x
#define __ivd_config_STRINGIFY_VALUE(_x) __ivd_config_STRINGIFY_NAME(_x)
ivd_config_settings_t ivd_config_settings[] =
{
#ifdef IVD_CONFIG_LOG_OPTIONS_DEFAULT
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_LOG_OPTIONS_DEFAULT), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ IVD_CONFIG_LOG_OPTIONS_DEFAULT(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVD_CONFIG_LOG_BITS_DEFAULT
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_LOG_BITS_DEFAULT), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_LOG_BITS_DEFAULT) },
#else
{ IVD_CONFIG_LOG_BITS_DEFAULT(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVD_CONFIG_PORTING_STDLIB
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_PORTING_STDLIB), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_PORTING_STDLIB) },
#else
{ IVD_CONFIG_PORTING_STDLIB(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVD_CONFIG_ZTN_JSON
    { __ivd_config_STRINGIFY_NAME(IVD_CONFIG_ZTN_JSON), __ivd_config_STRINGIFY_VALUE(IVD_CONFIG_ZTN_JSON) },
#else
{ IVD_CONFIG_ZTN_JSON(__ivd_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __ivd_config_STRINGIFY_VALUE
#undef __ivd_config_STRINGIFY_NAME

const char*
ivd_config_lookup(const char* setting)
{
    int i;
    for(i = 0; ivd_config_settings[i].name; i++) {
        if(strcmp(ivd_config_settings[i].name, setting)) {
            return ivd_config_settings[i].value;
        }
    }
    return NULL;
}

int
ivd_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; ivd_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", ivd_config_settings[i].name, ivd_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(IVD_CONFIG_HEADER).source> */
