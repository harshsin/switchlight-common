/******************************************************************************
 *
 *  /module/src/ivs_util.c
 *
 *  IVS Utilities
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include "ivs_int.h" 

/* <auto.start.util(ALL).define> */
void*
ivs_zmalloc(int size)
{
    void* p;
    p = IVS_MALLOC(size); 
    if(p) { 
        IVS_MEMSET(p, 0, size); 
    }
    return p; 
}
int
ivs_strlcpy(char* dst, const char* src, int size)
{
    IVS_STRNCPY(dst, src, size);
    if (size > 0)
        dst[size-1] = 0;
    return strlen(src); 
}
/* <auto.end.util(ALL).define> */

