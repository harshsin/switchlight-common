/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sl/sl_config.h>

#include "sl_int.h"

/* <auto.start.enum(ALL).source> */
aim_map_si_t sl_shell_service_type_map[] =
{
    { "RESERVED", SL_SHELL_SERVICE_TYPE_RESERVED },
    { "SHELL", SL_SHELL_SERVICE_TYPE_SHELL },
    { "CLI", SL_SHELL_SERVICE_TYPE_CLI },
    { NULL, 0 }
};

aim_map_si_t sl_shell_service_type_desc_map[] =
{
    { "None", SL_SHELL_SERVICE_TYPE_RESERVED },
    { "None", SL_SHELL_SERVICE_TYPE_SHELL },
    { "None", SL_SHELL_SERVICE_TYPE_CLI },
    { NULL, 0 }
};

const char*
sl_shell_service_type_name(sl_shell_service_type_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, sl_shell_service_type_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'sl_shell_service_type'";
    }
}

int
sl_shell_service_type_value(const char* str, sl_shell_service_type_t* e, int substr)
{
    int i;
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, sl_shell_service_type_map, 0)) {
        /* Enum Found */
        *e = i;
        return 0;
    }
    else {
        return -1;
    }
}

const char*
sl_shell_service_type_desc(sl_shell_service_type_t e)
{
    const char* name;
    if(aim_map_si_i(&name, e, sl_shell_service_type_desc_map, 0)) {
        return name;
    }
    else {
        return "-invalid value for enum type 'sl_shell_service_type'";
    }
}

/* <auto.end.enum(ALL).source> */

