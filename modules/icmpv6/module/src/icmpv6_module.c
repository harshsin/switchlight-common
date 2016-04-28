/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <icmpv6/icmpv6_config.h>

#include "icmpv6_log.h"

static int
datatypes_init__(void)
{
#define ICMPV6_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <icmpv6/icmpv6.x>
    return 0;
}

void __icmpv6_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

