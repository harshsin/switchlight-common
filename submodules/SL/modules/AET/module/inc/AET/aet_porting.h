/**************************************************************************//**
 * 
 * @file
 * @brief AET Porting Macros.
 * 
 * @addtogroup aet_porting
 * @{
 * 
 *****************************************************************************/
#ifndef __AET_PORTING_H__
#define __AET_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if AET_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef AET_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define AET_MALLOC GLOBAL_MALLOC
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_MALLOC malloc
    #else
        #error The macro AET_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef AET_FREE
    #if defined(GLOBAL_FREE)
        #define AET_FREE GLOBAL_FREE
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_FREE free
    #else
        #error The macro AET_FREE is required but cannot be defined.
    #endif
#endif

#ifndef AET_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define AET_MEMSET GLOBAL_MEMSET
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_MEMSET memset
    #else
        #error The macro AET_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef AET_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define AET_MEMCPY GLOBAL_MEMCPY
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_MEMCPY memcpy
    #else
        #error The macro AET_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef AET_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define AET_STRNCPY GLOBAL_STRNCPY
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_STRNCPY strncpy
    #else
        #error The macro AET_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef AET_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define AET_VSNPRINTF GLOBAL_VSNPRINTF
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_VSNPRINTF vsnprintf
    #else
        #error The macro AET_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef AET_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define AET_SNPRINTF GLOBAL_SNPRINTF
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_SNPRINTF snprintf
    #else
        #error The macro AET_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef AET_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define AET_STRLEN GLOBAL_STRLEN
    #elif AET_CONFIG_PORTING_STDLIB == 1
        #define AET_STRLEN strlen
    #else
        #error The macro AET_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __AET_PORTING_H__ */
/* @} */
