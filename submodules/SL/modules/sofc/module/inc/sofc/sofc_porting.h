/**************************************************************************//**
 * 
 * @file
 * @brief sofc Porting Macros.
 * 
 * @addtogroup sofc-porting
 * @{
 * 
 *****************************************************************************/
#ifndef __SOFC_PORTING_H__
#define __SOFC_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if SOFC_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef SOFC_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define SOFC_MALLOC GLOBAL_MALLOC
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_MALLOC malloc
    #else
        #error The macro SOFC_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_FREE
    #if defined(GLOBAL_FREE)
        #define SOFC_FREE GLOBAL_FREE
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_FREE free
    #else
        #error The macro SOFC_FREE is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define SOFC_MEMSET GLOBAL_MEMSET
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_MEMSET memset
    #else
        #error The macro SOFC_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define SOFC_MEMCPY GLOBAL_MEMCPY
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_MEMCPY memcpy
    #else
        #error The macro SOFC_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define SOFC_STRNCPY GLOBAL_STRNCPY
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_STRNCPY strncpy
    #else
        #error The macro SOFC_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define SOFC_VSNPRINTF GLOBAL_VSNPRINTF
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_VSNPRINTF vsnprintf
    #else
        #error The macro SOFC_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define SOFC_SNPRINTF GLOBAL_SNPRINTF
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_SNPRINTF snprintf
    #else
        #error The macro SOFC_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef SOFC_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define SOFC_STRLEN GLOBAL_STRLEN
    #elif SOFC_CONFIG_PORTING_STDLIB == 1
        #define SOFC_STRLEN strlen
    #else
        #error The macro SOFC_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __SOFC_PORTING_H__ */
/* @} */
