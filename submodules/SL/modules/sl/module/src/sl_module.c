/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sl/sl_config.h>
#include "sl_int.h"
#include "sl_log.h"

static int
datatypes_init__(void)
{
#define SL_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <sl/sl.x>
    return 0;
}


void __sl_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
    sl_bsn_shell_handler_register(); 
}

