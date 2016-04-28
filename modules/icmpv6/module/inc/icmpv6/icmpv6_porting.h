/**************************************************************************//**
 *
 * @file
 * @brief icmpv6 Porting Macros.
 *
 * @addtogroup icmpv6-porting
 * @{
 *
 *****************************************************************************/
#ifndef __ICMPV6_PORTING_H__
#define __ICMPV6_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ICMPV6_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ICMPV6_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ICMPV6_MALLOC GLOBAL_MALLOC
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_MALLOC malloc
    #else
        #error The macro ICMPV6_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_FREE
    #if defined(GLOBAL_FREE)
        #define ICMPV6_FREE GLOBAL_FREE
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_FREE free
    #else
        #error The macro ICMPV6_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ICMPV6_MEMSET GLOBAL_MEMSET
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_MEMSET memset
    #else
        #error The macro ICMPV6_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ICMPV6_MEMCPY GLOBAL_MEMCPY
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_MEMCPY memcpy
    #else
        #error The macro ICMPV6_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define ICMPV6_STRNCPY GLOBAL_STRNCPY
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_STRNCPY strncpy
    #else
        #error The macro ICMPV6_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ICMPV6_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_VSNPRINTF vsnprintf
    #else
        #error The macro ICMPV6_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ICMPV6_SNPRINTF GLOBAL_SNPRINTF
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_SNPRINTF snprintf
    #else
        #error The macro ICMPV6_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ICMPV6_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ICMPV6_STRLEN GLOBAL_STRLEN
    #elif ICMPV6_CONFIG_PORTING_STDLIB == 1
        #define ICMPV6_STRLEN strlen
    #else
        #error The macro ICMPV6_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ICMPV6_PORTING_H__ */
/* @} */
