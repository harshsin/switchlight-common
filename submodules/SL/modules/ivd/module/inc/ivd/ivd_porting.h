#ifndef __IVD_PORTING_H__
#define __IVD_PORTING_H__

/* <auto.start.portingmacro(ALL).define> */
#if IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef IVD_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define IVD_MEMSET GLOBAL_MEMSET
    #elif IVD_CONFIG_PORTING_STDLIB == 1
        #define IVD_MEMSET memset
    #else
        #error The macro IVD_MEMSET is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */

#endif /* __IVD_PORTING_H__ */
