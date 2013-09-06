/**************************************************************************//**
 *
 * @file
 * @brief sl Porting Macros.
 *
 * @addtogroup sl-porting
 * @{
 *
 *****************************************************************************/
#ifndef __SL_PORTING_H__
#define __SL_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if SL_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef SL_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define SL_MALLOC GLOBAL_MALLOC
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_MALLOC malloc
    #else
        #error The macro SL_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef SL_FREE
    #if defined(GLOBAL_FREE)
        #define SL_FREE GLOBAL_FREE
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_FREE free
    #else
        #error The macro SL_FREE is required but cannot be defined.
    #endif
#endif

#ifndef SL_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define SL_MEMSET GLOBAL_MEMSET
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_MEMSET memset
    #else
        #error The macro SL_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef SL_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define SL_MEMCPY GLOBAL_MEMCPY
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_MEMCPY memcpy
    #else
        #error The macro SL_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef SL_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define SL_STRNCPY GLOBAL_STRNCPY
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_STRNCPY strncpy
    #else
        #error The macro SL_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef SL_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define SL_VSNPRINTF GLOBAL_VSNPRINTF
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_VSNPRINTF vsnprintf
    #else
        #error The macro SL_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef SL_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define SL_SNPRINTF GLOBAL_SNPRINTF
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_SNPRINTF snprintf
    #else
        #error The macro SL_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef SL_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define SL_STRLEN GLOBAL_STRLEN
    #elif SL_CONFIG_PORTING_STDLIB == 1
        #define SL_STRLEN strlen
    #else
        #error The macro SL_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __SL_PORTING_H__ */
/* @} */
