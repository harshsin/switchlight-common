/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <collecta/collecta_config.h>

#include "collecta_log.h"

static int
datatypes_init__(void)
{
#define COLLECTA_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <collecta/collecta.x>
    return 0;
}

void __collecta_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

