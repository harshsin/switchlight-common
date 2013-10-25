/******************************************************************************
 *
 *  /module/src/ivs_int.h
 *
 *  IVS Internal Header
 *
 *****************************************************************************/
#ifndef __IVS_INT_H__
#define __IVS_INT_H__


#include <IVS/ivs_config.h>

#include <IVS/ivs.h> 

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
int ivs_status_log_show(ivs_t* ivs, aim_pvs_t* pvs); 
int ivs_status_log_enable(ivs_t* ivs);
#endif

#endif /* __IVS_INT_H__ */
