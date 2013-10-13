/**************************************************************************//**
 *
 * @file
 * @brief IVS Porting Definitions. 
 *
 *
 * @addtogroup ivs-porting
 * @{
 *
 *****************************************************************************/
#ifndef __IVS_PORTING_H__
#define __IVS_PORTING_H__


#include <IVS/ivs_config.h>


/* <auto.start.portingmacro(ALL).define> */
#if IVS_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef IVS_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define IVS_MALLOC GLOBAL_MALLOC
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_MALLOC malloc
    #else
        #error The macro IVS_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef IVS_FREE
    #if defined(GLOBAL_FREE)
        #define IVS_FREE GLOBAL_FREE
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_FREE free
    #else
        #error The macro IVS_FREE is required but cannot be defined.
    #endif
#endif

#ifndef IVS_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define IVS_MEMSET GLOBAL_MEMSET
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_MEMSET memset
    #else
        #error The macro IVS_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef IVS_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define IVS_MEMCPY GLOBAL_MEMCPY
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_MEMCPY memcpy
    #else
        #error The macro IVS_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef IVS_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define IVS_STRNCPY GLOBAL_STRNCPY
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_STRNCPY strncpy
    #else
        #error The macro IVS_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef IVS_STRCMP
    #if defined(GLOBAL_STRCMP)
        #define IVS_STRCMP GLOBAL_STRCMP
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_STRCMP strcmp
    #else
        #error The macro IVS_STRCMP is required but cannot be defined.
    #endif
#endif

#ifndef IVS_STRNCMP
    #if defined(GLOBAL_STRNCMP)
        #define IVS_STRNCMP GLOBAL_STRNCMP
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_STRNCMP strncmp
    #else
        #error The macro IVS_STRNCMP is required but cannot be defined.
    #endif
#endif

#ifndef IVS_STRCPY
    #if defined(GLOBAL_STRCPY)
        #define IVS_STRCPY GLOBAL_STRCPY
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_STRCPY strcpy
    #else
        #error The macro IVS_STRCPY is required but cannot be defined.
    #endif
#endif

#ifndef IVS_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define IVS_STRLEN GLOBAL_STRLEN
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_STRLEN strlen
    #else
        #error The macro IVS_STRLEN is required but cannot be defined.
    #endif
#endif

#ifndef IVS_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define IVS_VSNPRINTF GLOBAL_VSNPRINTF
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_VSNPRINTF vsnprintf
    #else
        #error The macro IVS_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef IVS_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define IVS_SNPRINTF GLOBAL_SNPRINTF
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_SNPRINTF snprintf
    #else
        #error The macro IVS_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef IVS_PRINTF
    #if defined(GLOBAL_PRINTF)
        #define IVS_PRINTF GLOBAL_PRINTF
    #elif IVS_CONFIG_PORTING_STDLIB == 1
        #define IVS_PRINTF printf
    #else
        #error The macro IVS_PRINTF is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */

#endif /* __IVS_PORTING_H__ */
/*@}*/
