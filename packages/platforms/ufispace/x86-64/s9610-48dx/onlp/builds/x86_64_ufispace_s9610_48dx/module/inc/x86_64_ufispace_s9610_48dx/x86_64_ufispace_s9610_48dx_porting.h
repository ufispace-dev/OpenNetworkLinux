/********************************************************//**
 *
 * @file
 * @brief x86_64_UfiSpace_s9610_48dx Porting Macros.
 *
 * @addtogroup x86_64_UfiSpace_s9610_48dx-porting
 * @{
 *
 ***********************************************************/
#ifndef __X86_64_UFISPACE_S9610_48DX_PORTING_H__
#define __X86_64_UFISPACE_S9610_48DX_PORTING_H__

/* <auto.start.portingmacro(ALL).define> */
#if X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define X86_64_UFISPACE_S9610_48DX_MALLOC GLOBAL_MALLOC
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_MALLOC malloc
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_FREE
    #if defined(GLOBAL_FREE)
        #define X86_64_UFISPACE_S9610_48DX_FREE GLOBAL_FREE
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_FREE free
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_FREE is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define X86_64_UFISPACE_S9610_48DX_MEMSET GLOBAL_MEMSET
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_MEMSET memset
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define X86_64_UFISPACE_S9610_48DX_MEMCPY GLOBAL_MEMCPY
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_MEMCPY memcpy
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define X86_64_UFISPACE_S9610_48DX_VSNPRINTF GLOBAL_VSNPRINTF
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_VSNPRINTF vsnprintf
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define X86_64_UFISPACE_S9610_48DX_SNPRINTF GLOBAL_SNPRINTF
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_SNPRINTF snprintf
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_UFISPACE_S9610_48DX_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define X86_64_UFISPACE_S9610_48DX_STRLEN GLOBAL_STRLEN
    #elif X86_64_UFISPACE_S9610_48DX_CONFIG_PORTING_STDLIB == 1
        #define X86_64_UFISPACE_S9610_48DX_STRLEN strlen
    #else
        #error The macro X86_64_UFISPACE_S9610_48DX_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __X86_64_UFISPACE_S9610_48DX_PORTING_H__ */
/* @} */