/******************************************************************************
 *
 *  /module/src/ivs_config.c
 *
 *  IVS Config Information
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include "ivs_int.h" 
#include <stdlib.h>
#include <string.h>



/* <auto.start.cdefs(IVS_CONFIG_HEADER).source> */
#define __ivs_config_STRINGIFY_NAME(_x) #_x
#define __ivs_config_STRINGIFY_VALUE(_x) __ivs_config_STRINGIFY_NAME(_x)
ivs_config_settings_t ivs_config_settings[] =
{
#ifdef IVS_CONFIG_INCLUDE_LOGGING
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_LOGGING), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_LOGGING) },
#else
{ IVS_CONFIG_INCLUDE_LOGGING(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_LOG_OPTIONS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_LOG_OPTIONS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ IVS_CONFIG_LOG_OPTIONS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_LOG_BITS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_LOG_BITS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_LOG_BITS_DEFAULT) },
#else
{ IVS_CONFIG_LOG_BITS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_PORTING_STDLIB
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_PORTING_STDLIB), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_PORTING_STDLIB) },
#else
{ IVS_CONFIG_PORTING_STDLIB(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT) },
#else
{ IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CONTROLLER_IP_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CONTROLLER_IP_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CONTROLLER_IP_DEFAULT) },
#else
{ IVS_CONFIG_CONTROLLER_IP_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CONTROLLER_PORT_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CONTROLLER_PORT_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CONTROLLER_PORT_DEFAULT) },
#else
{ IVS_CONFIG_CONTROLLER_PORT_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CONTROLLER_LISTEN_PORT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CONTROLLER_LISTEN_PORT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CONTROLLER_LISTEN_PORT) },
#else
{ IVS_CONFIG_CONTROLLER_LISTEN_PORT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT) },
#else
{ IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_PORT_MAX_PORTS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_PORT_MAX_PORTS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_PORT_MAX_PORTS_DEFAULT) },
#else
{ IVS_CONFIG_PORT_MAX_PORTS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT) },
#else
{ IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT) },
#else
{ IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT) },
#else
{ IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_DPID_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_DPID_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_DPID_DEFAULT) },
#else
{ IVS_CONFIG_DPID_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT) },
#else
{ IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CXN_RESET_ECHO_COUNT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CXN_RESET_ECHO_COUNT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CXN_RESET_ECHO_COUNT) },
#else
{ IVS_CONFIG_CXN_RESET_ECHO_COUNT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_NSS_PORT_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_NSS_PORT_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_NSS_PORT_DEFAULT) },
#else
{ IVS_CONFIG_NSS_PORT_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CONSOLE_PROMPT_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CONSOLE_PROMPT_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CONSOLE_PROMPT_DEFAULT) },
#else
{ IVS_CONFIG_CONSOLE_PROMPT_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_CONSOLE_CLI
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_CONSOLE_CLI), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_CONSOLE_CLI) },
#else
{ IVS_CONFIG_INCLUDE_CONSOLE_CLI(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_NETWORK_CLI
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_NETWORK_CLI), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_NETWORK_CLI) },
#else
{ IVS_CONFIG_INCLUDE_NETWORK_CLI(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT) },
#else
{ IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_WATCHDOG
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_WATCHDOG), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_WATCHDOG) },
#else
{ IVS_CONFIG_INCLUDE_WATCHDOG(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_WATCHDOG_SECONDS
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_WATCHDOG_SECONDS), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_WATCHDOG_SECONDS) },
#else
{ IVS_CONFIG_WATCHDOG_SECONDS(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_CXN_LOG
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_CXN_LOG), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_CXN_LOG) },
#else
{ IVS_CONFIG_INCLUDE_CXN_LOG(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_CXN_LOG_SIZE
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_CXN_LOG_SIZE), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_CXN_LOG_SIZE) },
#else
{ IVS_CONFIG_CXN_LOG_SIZE(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_STATUS_LOG
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_STATUS_LOG), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_STATUS_LOG) },
#else
{ IVS_CONFIG_INCLUDE_STATUS_LOG(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_STATUS_LOG_SIZE
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_STATUS_LOG_SIZE), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_STATUS_LOG_SIZE) },
#else
{ IVS_CONFIG_STATUS_LOG_SIZE(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_STATUS_LOG_PERIOD_S
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_STATUS_LOG_PERIOD_S), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_STATUS_LOG_PERIOD_S) },
#else
{ IVS_CONFIG_STATUS_LOG_PERIOD_S(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef IVS_CONFIG_INCLUDE_SIGTERM_HANDLER
    { __ivs_config_STRINGIFY_NAME(IVS_CONFIG_INCLUDE_SIGTERM_HANDLER), __ivs_config_STRINGIFY_VALUE(IVS_CONFIG_INCLUDE_SIGTERM_HANDLER) },
#else
{ IVS_CONFIG_INCLUDE_SIGTERM_HANDLER(__ivs_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __ivs_config_STRINGIFY_VALUE
#undef __ivs_config_STRINGIFY_NAME

const char*
ivs_config_lookup(const char* setting)
{
    int i;
    for(i = 0; ivs_config_settings[i].name; i++) {
        if(strcmp(ivs_config_settings[i].name, setting)) {
            return ivs_config_settings[i].value;
        }
    }
    return NULL;
}

int
ivs_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; ivs_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", ivs_config_settings[i].name, ivs_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(IVS_CONFIG_HEADER).source> */



