/****************************************************************
 *
 *        Copyright 2016, Big Switch Networks, Inc.
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
 * @brief macblaster Porting Macros.
 *
 * @addtogroup macblaster-porting
 * @{
 *
 ****************************************************************/
#ifndef __MACBLASTER_PORTING_H__
#define __MACBLASTER_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if MACBLASTER_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef MACBLASTER_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define MACBLASTER_MALLOC GLOBAL_MALLOC
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_MALLOC malloc
    #else
        #error The macro MACBLASTER_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_FREE
    #if defined(GLOBAL_FREE)
        #define MACBLASTER_FREE GLOBAL_FREE
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_FREE free
    #else
        #error The macro MACBLASTER_FREE is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define MACBLASTER_MEMSET GLOBAL_MEMSET
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_MEMSET memset
    #else
        #error The macro MACBLASTER_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define MACBLASTER_MEMCPY GLOBAL_MEMCPY
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_MEMCPY memcpy
    #else
        #error The macro MACBLASTER_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define MACBLASTER_STRNCPY GLOBAL_STRNCPY
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_STRNCPY strncpy
    #else
        #error The macro MACBLASTER_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define MACBLASTER_VSNPRINTF GLOBAL_VSNPRINTF
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_VSNPRINTF vsnprintf
    #else
        #error The macro MACBLASTER_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define MACBLASTER_SNPRINTF GLOBAL_SNPRINTF
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_SNPRINTF snprintf
    #else
        #error The macro MACBLASTER_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef MACBLASTER_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define MACBLASTER_STRLEN GLOBAL_STRLEN
    #elif MACBLASTER_CONFIG_PORTING_STDLIB == 1
        #define MACBLASTER_STRLEN strlen
    #else
        #error The macro MACBLASTER_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __MACBLASTER_PORTING_H__ */
/* @} */
