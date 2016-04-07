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
 * @brief vxlan Porting Macros.
 *
 * @addtogroup vxlan-porting
 * @{
 *
 ****************************************************************/
#ifndef __VXLAN_PORTING_H__
#define __VXLAN_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if VXLAN_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef VXLAN_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define VXLAN_MALLOC GLOBAL_MALLOC
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_MALLOC malloc
    #else
        #error The macro VXLAN_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_FREE
    #if defined(GLOBAL_FREE)
        #define VXLAN_FREE GLOBAL_FREE
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_FREE free
    #else
        #error The macro VXLAN_FREE is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define VXLAN_MEMSET GLOBAL_MEMSET
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_MEMSET memset
    #else
        #error The macro VXLAN_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define VXLAN_MEMCPY GLOBAL_MEMCPY
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_MEMCPY memcpy
    #else
        #error The macro VXLAN_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define VXLAN_STRNCPY GLOBAL_STRNCPY
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_STRNCPY strncpy
    #else
        #error The macro VXLAN_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define VXLAN_VSNPRINTF GLOBAL_VSNPRINTF
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_VSNPRINTF vsnprintf
    #else
        #error The macro VXLAN_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define VXLAN_SNPRINTF GLOBAL_SNPRINTF
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_SNPRINTF snprintf
    #else
        #error The macro VXLAN_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef VXLAN_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define VXLAN_STRLEN GLOBAL_STRLEN
    #elif VXLAN_CONFIG_PORTING_STDLIB == 1
        #define VXLAN_STRLEN strlen
    #else
        #error The macro VXLAN_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __VXLAN_PORTING_H__ */
/* @} */
