/**************************************************************************//**
 *
 * @file
 * @brief collecta Porting Macros.
 *
 * @addtogroup collecta-porting
 * @{
 *
 *****************************************************************************/
#ifndef __COLLECTA_PORTING_H__
#define __COLLECTA_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if COLLECTA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef COLLECTA_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define COLLECTA_MALLOC GLOBAL_MALLOC
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_MALLOC malloc
    #else
        #error The macro COLLECTA_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_FREE
    #if defined(GLOBAL_FREE)
        #define COLLECTA_FREE GLOBAL_FREE
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_FREE free
    #else
        #error The macro COLLECTA_FREE is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define COLLECTA_MEMSET GLOBAL_MEMSET
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_MEMSET memset
    #else
        #error The macro COLLECTA_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define COLLECTA_MEMCPY GLOBAL_MEMCPY
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_MEMCPY memcpy
    #else
        #error The macro COLLECTA_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define COLLECTA_STRNCPY GLOBAL_STRNCPY
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_STRNCPY strncpy
    #else
        #error The macro COLLECTA_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define COLLECTA_VSNPRINTF GLOBAL_VSNPRINTF
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_VSNPRINTF vsnprintf
    #else
        #error The macro COLLECTA_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define COLLECTA_SNPRINTF GLOBAL_SNPRINTF
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_SNPRINTF snprintf
    #else
        #error The macro COLLECTA_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef COLLECTA_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define COLLECTA_STRLEN GLOBAL_STRLEN
    #elif COLLECTA_CONFIG_PORTING_STDLIB == 1
        #define COLLECTA_STRLEN strlen
    #else
        #error The macro COLLECTA_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __COLLECTA_PORTING_H__ */
/* @} */
