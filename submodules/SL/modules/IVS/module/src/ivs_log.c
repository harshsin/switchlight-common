/******************************************************************************
 *
 *  /module/src/ivs_log.c
 *
 *  IVS Logger
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include "ivs_int.h" 
#include "ivs_log.h"


AIM_LOG_STRUCT_DEFINE(
                      IVS_CONFIG_LOG_OPTIONS_DEFAULT,
                      IVS_CONFIG_LOG_BITS_DEFAULT,
                      NULL,     /* Custom Log Map */
                      IVS_CONFIG_LOG_CUSTOM_BITS_DEFAULT
                      );
