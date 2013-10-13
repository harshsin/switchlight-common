/******************************************************************************
 *
 *  /module/src/ivs_enums.c
 *
 *  IVS Enum Definitions
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include "ivs_int.h" 

#include <AIM/aim_map.h>


#include <stdlib.h>
#include <string.h>


/* <auto.start.enum(ALL).source> */
aim_map_si_t ivs_cxn_role_map[] =
{
    { "UNKNOWN", IVS_CXN_ROLE_UNKNOWN },
    { "MASTER", IVS_CXN_ROLE_MASTER },
    { "SLAVE", IVS_CXN_ROLE_SLAVE },
    { "EQUAL", IVS_CXN_ROLE_EQUAL },
    { NULL, 0 }
};

aim_map_si_t ivs_cxn_role_desc_map[] =
{
    { "None", IVS_CXN_ROLE_UNKNOWN },
    { "None", IVS_CXN_ROLE_MASTER },
    { "None", IVS_CXN_ROLE_SLAVE },
    { "None", IVS_CXN_ROLE_EQUAL },
    { NULL, 0 }
};

const char*
ivs_cxn_role_name(ivs_cxn_role_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_role_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_role'";
    }
}

int
ivs_cxn_role_value(const char* str, ivs_cxn_role_t* e, int substr)
{
    int i; 
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, ivs_cxn_role_map, 0)) {
        /* Enum Found */
        *e = i; 
        return 0; 
    }
    else {
        return -1; 
    }
}

const char*
ivs_cxn_role_desc(ivs_cxn_role_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_role_desc_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_role'";
    }
}

int
ivs_cxn_role_valid(ivs_cxn_role_t e)
{
    return aim_map_si_i(NULL, e, ivs_cxn_role_map, 0) ? 1 : 0;
}


aim_map_si_t ivs_cxn_state_map[] =
{
    { "DISCONNECTED", IVS_CXN_STATE_DISCONNECTED },
    { "CONNECTING", IVS_CXN_STATE_CONNECTING },
    { "CONNECTED", IVS_CXN_STATE_CONNECTED },
    { "CLOSING", IVS_CXN_STATE_CLOSING },
    { NULL, 0 }
};

aim_map_si_t ivs_cxn_state_desc_map[] =
{
    { "None", IVS_CXN_STATE_DISCONNECTED },
    { "None", IVS_CXN_STATE_CONNECTING },
    { "None", IVS_CXN_STATE_CONNECTED },
    { "None", IVS_CXN_STATE_CLOSING },
    { NULL, 0 }
};

const char*
ivs_cxn_state_name(ivs_cxn_state_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_state_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_state'";
    }
}

int
ivs_cxn_state_value(const char* str, ivs_cxn_state_t* e, int substr)
{
    int i; 
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, ivs_cxn_state_map, 0)) {
        /* Enum Found */
        *e = i; 
        return 0; 
    }
    else {
        return -1; 
    }
}

const char*
ivs_cxn_state_desc(ivs_cxn_state_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_state_desc_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_state'";
    }
}

int
ivs_cxn_state_valid(ivs_cxn_state_t e)
{
    return aim_map_si_i(NULL, e, ivs_cxn_state_map, 0) ? 1 : 0;
}


aim_map_si_t ivs_cxn_type_map[] =
{
    { "CONNECT", IVS_CXN_TYPE_CONNECT },
    { "LISTEN", IVS_CXN_TYPE_LISTEN },
    { NULL, 0 }
};

aim_map_si_t ivs_cxn_type_desc_map[] =
{
    { "None", IVS_CXN_TYPE_CONNECT },
    { "None", IVS_CXN_TYPE_LISTEN },
    { NULL, 0 }
};

const char*
ivs_cxn_type_name(ivs_cxn_type_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_type_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_type'";
    }
}

int
ivs_cxn_type_value(const char* str, ivs_cxn_type_t* e, int substr)
{
    int i; 
    AIM_REFERENCE(substr);
    if(aim_map_si_s(&i, str, ivs_cxn_type_map, 0)) {
        /* Enum Found */
        *e = i; 
        return 0; 
    }
    else {
        return -1; 
    }
}

const char*
ivs_cxn_type_desc(ivs_cxn_type_t e)
{
    const char* name; 
    if(aim_map_si_i(&name, e, ivs_cxn_type_desc_map, 0)) { 
        return name; 
    }
    else { 
        return "-invalid value for enum type 'ivs_cxn_type'";
    }
}

/* <auto.end.enum(ALL).source> */
