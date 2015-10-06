/****************************************************************
 *
 *        Copyright 2015, Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

/*************************************************************//**
 *
 * @file
 * @brief pdua Porting Macros.
 *
 * @addtogroup pdua-porting
 * @{
 *
 ****************************************************************/
#ifndef __PDUA_PORTING_H__
#define __PDUA_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if PDUA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef PDUA_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define PDUA_MALLOC GLOBAL_MALLOC
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_MALLOC malloc
    #else
        #error The macro PDUA_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_FREE
    #if defined(GLOBAL_FREE)
        #define PDUA_FREE GLOBAL_FREE
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_FREE free
    #else
        #error The macro PDUA_FREE is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define PDUA_MEMSET GLOBAL_MEMSET
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_MEMSET memset
    #else
        #error The macro PDUA_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define PDUA_MEMCPY GLOBAL_MEMCPY
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_MEMCPY memcpy
    #else
        #error The macro PDUA_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define PDUA_STRNCPY GLOBAL_STRNCPY
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_STRNCPY strncpy
    #else
        #error The macro PDUA_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define PDUA_VSNPRINTF GLOBAL_VSNPRINTF
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_VSNPRINTF vsnprintf
    #else
        #error The macro PDUA_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define PDUA_SNPRINTF GLOBAL_SNPRINTF
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_SNPRINTF snprintf
    #else
        #error The macro PDUA_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef PDUA_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define PDUA_STRLEN GLOBAL_STRLEN
    #elif PDUA_CONFIG_PORTING_STDLIB == 1
        #define PDUA_STRLEN strlen
    #else
        #error The macro PDUA_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __PDUA_PORTING_H__ */
/* @} */
