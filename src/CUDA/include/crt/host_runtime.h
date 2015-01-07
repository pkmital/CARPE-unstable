/*
 * Copyright 1993-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:   
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and 
 * international Copyright laws.  Users and possessors of this source code 
 * are hereby granted a nonexclusive, royalty-free license to use this code 
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOURCE CODE.  
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

#if !defined(__CUDA_INTERNAL_COMPILATION__)

#define __CUDA_INTERNAL_COMPILATION__
#define __glob_pref_var(var) \
        __global_##var
#define __global_var(var) \
        (*__glob_pref_var(var))
#define __shadow_pref_var(var) \
        __shadow_##var
#define __shadow_var(var) \
        __shadow_pref_var(var)
#define __pad__(f) \
        f
#define __text__
#define __dv(v)

#if defined(__APPLE__)

#define __extern_weak__ \
        __weak_import__,

#elif defined(__GNUC__)

#define __extern_weak__

#endif /* __APPLE__ */

#if defined(__cplusplus)

#define __device_stub_name(c, cpp) \
        cpp
#define __cppref__ \
        &

#else /* __cplusplus */

#define __device_stub_name(c, cpp) \
        c
#define __cppref__

typedef char bool;

#endif /* __cplusplus */

#include "cuda_runtime_api.h"
#include "storage_class.h"

#else /* !__CUDA_INTERNAL_COMPILATION__ */

#include "host_defines.h"

#define __cudaRegisterBinary()                                                 \
        __cudaFatCubinHandle = __cudaRegisterFatBinary((void*)__cudaFatCubin); \
        atexit(__cudaUnregisterBinaryUtil)
#define __cudaRegisterVariable(var, ext, size, constant, global) \
        __cudaRegisterVar(__cudaFatCubinHandle, (char*)&__host##var, (char*)__device##var, __name##var, ext, size, constant, global)
#define __cudaRegisterGlobalTexture(tex, dim, norm, ext) \
        __cudaRegisterTexture(__cudaFatCubinHandle, (const struct textureReference*)&tex, __tex_var(tex), #tex, dim, norm, ext)
#define __cudaRegisterUnsizedShared(var) \
        __cudaRegisterShared(__cudaFatCubinHandle, (void**)__device_var(var))
#define __cudaRegisterSharedVariable(var, size, align, sc) \
        __cudaRegisterSharedVar(__cudaFatCubinHandle, (void**)__device_var(var), size, align, sc)
#define __cudaRegisterEntry(funptr, fun, thread_limit) \
        __cudaRegisterFunction(__cudaFatCubinHandle, (const char*)funptr, (char*)__device_fun(fun), #fun, thread_limit, __ids)

#define __cudaInitArgBlock(arg) \
        char __[256];           \
        *(char**)&arg = __
#define __cudaSetupArg(arg, offset) \
        if (cudaSetupArgument((void*)(char*)&arg, sizeof(arg), (size_t)&offset->arg - (size_t)offset) != cudaSuccess) \
          return
#define __cudaLaunch(fun) \
        { volatile static char *__f; __f = fun; (void)cudaLaunch(fun); }

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

extern void** CUDARTAPI __cudaRegisterFatBinary(
  void *fatCubin
);

extern void CUDARTAPI __cudaUnregisterFatBinary(
  void **fatCubinHandle
);

extern void CUDARTAPI __cudaRegisterVar(
        void **fatCubinHandle,
        char  *hostVar,
        char  *deviceAddress,
  const char  *deviceName,
        int    ext,
        int    size,
        int    constant,
        int    global
);

extern void CUDARTAPI __cudaRegisterTexture(
        void                    **fatCubinHandle,
  const struct textureReference  *hostVar,
  const void                    **deviceAddress,
  const char                     *deviceName,
        int                       dim,       
        int                       norm,      
        int                       ext        
);

extern void CUDARTAPI __cudaRegisterShared(
  void **fatCubinHandle,
  void **devicePtr
);

extern void CUDARTAPI __cudaRegisterSharedVar(
  void   **fatCubinHandle,
  void   **devicePtr,
  size_t   size,
  size_t   alignment,
  int      storage
);

extern void CUDARTAPI __cudaRegisterFunction(
        void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize
);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#if defined(__GNUC__) && defined(__cplusplus)

extern int atexit(void(*)(void)) throw();

#else /* __GNUC__ && __cplusplus */

extern int atexit(void(*)(void));

#endif /* __GNUC__ && __cplusplus */

static void **__cudaFatCubinHandle;

static void __cudaUnregisterBinaryUtil(void)
{
  __cudaUnregisterFatBinary(__cudaFatCubinHandle);
}

#if defined(__device_emulation)

#define __device_fun(fun) \
        __device_wrapper_##fun
#define __device_var(var) \
        (char*)&var
#define __tex_var(var) \
        &__texture_var(var)
#define __cudaFatCubin \
        0

#if defined(__multi_core__)

#define __ids \
        (uint3*)0, (uint3*)0, &blockDim, &gridDim, &warpSize

#else /* __multi_core__ */

#define __ids \
        &threadIdx, &blockIdx, &blockDim, &gridDim, &warpSize

#endif /* __multi_core__ */

#else /* __device_emulation */

#define __device_fun(fun) \
        #fun
#define __device_var(var) \
        #var
#define __tex_var(var) \
        0
#define __cudaFatCubin \
        (&__fatDeviceText)
#define __ids \
        (uint3*)0, (uint3*)0, (dim3*)0, (dim3*)0, (int*)0

#include "common_functions.h"

#endif /* __device_emulation */

/* UTILITY MACROS */
#define __device__global_var(var) \
        __device_var(var)
#define __name__global_var(var) \
        #var
#define __host__global_var(var) \
        __glob_pref_var(var)
#define __device__shadow_var(var) \
        __device_var(var)
#define __name__shadow_var(var) \
        #var
#define __host__shadow_var(var) \
        __shadow_pref_var(var)

#endif /* !__CUDA_INTERNAL_COMPILATION__ */
