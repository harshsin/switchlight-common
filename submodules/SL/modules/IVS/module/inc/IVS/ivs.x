/******************************************************************************
 *
 *  /module/inc/IVS/ivs.x
 *
 *  IVS X Macros
 *
 *****************************************************************************/
#include <IVS/ivs_config.h>



/* <auto.start.xmacro(ALL).define> */
#ifdef IVS_CXN_TYPE_ENTRY
IVS_CXN_TYPE_ENTRY(CONNECT)
IVS_CXN_TYPE_ENTRY(LISTEN)
#undef IVS_CXN_TYPE_ENTRY
#endif
/* <auto.end.xmacro(ALL).define> */


/* <auto.start.xenum(ALL).define> */
#ifdef IVS_ENUMERATION_ENTRY
IVS_ENUMERATION_ENTRY(ivs_cxn_role, "IVS Connection Roles")
IVS_ENUMERATION_ENTRY(ivs_cxn_state, "IVS Connection States")
IVS_ENUMERATION_ENTRY(ivs_cxn_type, "IVS Connection Type")
#undef IVS_ENUMERATION_ENTRY
#endif
/* <auto.end.xenum(ALL).define> */

