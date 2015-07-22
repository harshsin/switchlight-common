/**************************************************************************//**
 *
 * @file
 * @brief igmpa Porting Macros.
 *
 * @addtogroup igmpa-porting
 * @{
 *
 *****************************************************************************/
#ifndef __IGMPA_PORTING_H__
#define __IGMPA_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if IGMPA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef IGMPA_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define IGMPA_MALLOC GLOBAL_MALLOC
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_MALLOC malloc
    #else
        #error The macro IGMPA_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_FREE
    #if defined(GLOBAL_FREE)
        #define IGMPA_FREE GLOBAL_FREE
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_FREE free
    #else
        #error The macro IGMPA_FREE is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define IGMPA_MEMSET GLOBAL_MEMSET
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_MEMSET memset
    #else
        #error The macro IGMPA_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define IGMPA_MEMCPY GLOBAL_MEMCPY
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_MEMCPY memcpy
    #else
        #error The macro IGMPA_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define IGMPA_STRNCPY GLOBAL_STRNCPY
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_STRNCPY strncpy
    #else
        #error The macro IGMPA_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define IGMPA_VSNPRINTF GLOBAL_VSNPRINTF
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_VSNPRINTF vsnprintf
    #else
        #error The macro IGMPA_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define IGMPA_SNPRINTF GLOBAL_SNPRINTF
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_SNPRINTF snprintf
    #else
        #error The macro IGMPA_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef IGMPA_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define IGMPA_STRLEN GLOBAL_STRLEN
    #elif IGMPA_CONFIG_PORTING_STDLIB == 1
        #define IGMPA_STRLEN strlen
    #else
        #error The macro IGMPA_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __IGMPA_PORTING_H__ */
/* @} */
