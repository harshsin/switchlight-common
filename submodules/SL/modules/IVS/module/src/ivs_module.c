/**************************************************************************//**
 *
 *
 *
 *
 *****************************************************************************/
#include <IVS/ivs_config.h>
#include "ivs_log.h"
#include <AIM/aim_datatypes.h>
#include <IVS/ivs.h>

/*
 * Maptable for Indigo enumerations. 
 * These should be elsewhere. 
 */
static aim_datatype_map_t indigo_error_map__[] = 
    { 
        { "none", INDIGO_ERROR_NONE }, 
        { "pending", INDIGO_ERROR_PENDING },
        { "resource", INDIGO_ERROR_RESOURCE },
        { "param", INDIGO_ERROR_PARAM }, 
        { "version", INDIGO_ERROR_VERSION }, 
        { "range", INDIGO_ERROR_RANGE },
        { "compat", INDIGO_ERROR_COMPAT }, 
        { "parse", INDIGO_ERROR_PARSE },        
        { "init", INDIGO_ERROR_INIT }, 
        { "exists", INDIGO_ERROR_EXISTS }, 
        { "not found", INDIGO_ERROR_NOT_FOUND }, 
        { "not supported", INDIGO_ERROR_NOT_SUPPORTED }, 
        { "time out", INDIGO_ERROR_TIME_OUT }, 
        { "protocol", INDIGO_ERROR_PROTOCOL }, 
        { "connection", INDIGO_ERROR_CONNECTION }, 
        { "not ready", INDIGO_ERROR_NOT_READY }, 
        { "unknown", INDIGO_ERROR_UNKNOWN }, 
        { NULL, 0 }
    }; 


int
ivs_datatypes_init(void)
{
    /** Register datatype handlers for all of our enumeration types */
#define IVS_ENUMERATION_ENTRY(_enum_name, _desc)                   \
    AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc, \
                              IVS_INTERNAL);
#include <IVS/ivs.x>

    AIM_DATATYPE_MAP_REGISTER(ind_error, indigo_error_map__, 
                              "Indigo Error", IVS_INTERNAL); 
    return 0; 
}


void
__ivs_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER(); 
    ivs_datatypes_init(); 
}
