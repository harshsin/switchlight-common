/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <igmpa/igmpa_config.h>

#include "igmpa_log.h"

static int
datatypes_init__(void)
{
#define IGMPA_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <igmpa/igmpa.x>
    return 0;
}

void __igmpa_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

