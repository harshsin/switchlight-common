/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <collecta/collecta_config.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include <Configuration/configuration.h>

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

uint64_t datapath_id = 0L;
static uint64_t dpid_stage = 0L;

static indigo_error_t
collecta_cfg_stage(cJSON *root) {
    char *dpid_str = NULL;
    int err = ind_cfg_lookup_string(root, "of_datapath_id", &dpid_str);
    if (err >= 0) {
        dpid_stage = strtoull(dpid_str, NULL, 16);
    }
    return INDIGO_ERROR_NONE;
}

static void
collecta_cfg_commit(void)
{
    datapath_id = dpid_stage;
}

const struct ind_cfg_ops collecta_cfg_ops = {
    .stage = collecta_cfg_stage,
    .commit = collecta_cfg_commit,
};
