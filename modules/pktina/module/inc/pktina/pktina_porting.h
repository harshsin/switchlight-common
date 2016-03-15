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
 * @brief pktina Porting Macros.
 *
 * @addtogroup pktina-porting
 * @{
 *
 ****************************************************************/
#ifndef __PKTINA_PORTING_H__
#define __PKTINA_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if PKTINA_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef PKTINA_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define PKTINA_MALLOC GLOBAL_MALLOC
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_MALLOC malloc
    #else
        #error The macro PKTINA_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_FREE
    #if defined(GLOBAL_FREE)
        #define PKTINA_FREE GLOBAL_FREE
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_FREE free
    #else
        #error The macro PKTINA_FREE is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define PKTINA_MEMSET GLOBAL_MEMSET
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_MEMSET memset
    #else
        #error The macro PKTINA_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define PKTINA_MEMCPY GLOBAL_MEMCPY
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_MEMCPY memcpy
    #else
        #error The macro PKTINA_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define PKTINA_STRNCPY GLOBAL_STRNCPY
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_STRNCPY strncpy
    #else
        #error The macro PKTINA_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define PKTINA_VSNPRINTF GLOBAL_VSNPRINTF
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_VSNPRINTF vsnprintf
    #else
        #error The macro PKTINA_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define PKTINA_SNPRINTF GLOBAL_SNPRINTF
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_SNPRINTF snprintf
    #else
        #error The macro PKTINA_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef PKTINA_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define PKTINA_STRLEN GLOBAL_STRLEN
    #elif PKTINA_CONFIG_PORTING_STDLIB == 1
        #define PKTINA_STRLEN strlen
    #else
        #error The macro PKTINA_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __PKTINA_PORTING_H__ */
/* @} */
