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

#endif /* !__CUDA_INTERNAL_COMPILATION__ */

#include "host_defines.h"

#define __no_sc__
#define __empty_array(s) \
        s
#define __unsized_shared_data(name, type_post) \
        __unsized##name __unsized##type_post
#define __sized_shared_data(name, type) \
        __sized##name type
#define __sized__shared_var(name, s, type) \
        name

/*TEXTURE_TYPE*/
typedef const void *__texture_type__;

#if defined(__CUDABE__) /* cudabe compiler */

#define __pad__(f)
#define __text__ \
        __attribute__((__texture__))
#define ___device__(sc) \
        static
#define __in__(cdecl, decl) \
        __shared__ cdecl
#define __in_type__(cdecl, decl) \
        cdecl
#define __texture_var(name) \
        name
#define __shared_var(name, s, type) \
        name
#define __cuda_host_device_name(name) \
        name
#define __val_param(name) \
        __val_param##name
#define __copy_param(local_decl, param) \
        local_decl = param
#define __unsized_array_size \
        []
#define __unsized__shared_var(name, s, type) \
        name
#define __unsized__empty_array(s) \
        s
#define __var_used__ \
        __attribute__((__used__))

#undef __cdecl
#define __cdecl
#undef __w64
#define __w64

#elif defined(__CUDACC__) /* cudafe compiler */

#define __loc_sc__(loc, sc) \
        sc loc
#define __pad__(f)
#define __text__
#define ___device__(sc) \
        sc __device__
#define __in__(cdecl, decl) \
        decl
#define __in_type__(cdecl, decl) \
        decl
#define __texture_var(name) \
        name
#define __shared_var(name, s, type) \
        name
#define __cuda_host_device_name(name) \
        name
#define __val_param(name) \
        name
#define __copy_param(local_decl, param)
#define __unsized_array_size \
        []
#define __unsized__shared_var(name, s, type) \
        name
#define __unsized__empty_array(s) \
        s

#else /* host compiler (cl, gcc, open64, ...) */

#if defined (__MULTI_CORE__) || defined(__multi_core__)

struct uint3;
extern struct uint3* CUDARTAPI __cudaGetBlockIdxPtr(void);
extern        void*  CUDARTAPI __cudaGetSharedMem(void*);

#endif /* __MULTI_CORE__ || __multi_core__ */

#if defined (__multi_core__)

#if defined(__GNUC__)

#if defined(__cplusplus)

extern void *alloca(size_t) throw();

#else /* __cplusplus */

extern void *alloca(size_t);

#endif /* __cplusplus */

#define __cuda_alloc(s) \
        alloca(s)

#else /* __GNUC__ */

extern void *_alloca(size_t);

#define __cuda_alloc(s) \
        _alloca(s)

#endif  /* __GNUC__ */

#endif /* __multi_core__ */

#if defined (__MULTI_CORE__)

#define ___device__(sc) \
        static
#define __pad__(f) \
        f
#define __text__
#define __cudaGet_blockIdx() \
        (*__cudaGetBlockIdxPtr())
#define __shared_var(name, s, type) \
        (s type __cudaGetSharedMem((void*)(&(name))))
#define __var_used__ \
        __attribute__((__used__))
#define __storage_auto__shared__ \
        auto

#undef __cdecl
#define __cdecl
#undef __w64
#define __w64

#else /* __MULTI_CORE__ */

#define ___device__(sc) \
        static __device__
#define __shared_var(name, s, type) \
        name

#if defined(__APPLE__) || defined(__ICC)

#define __STORAGE__ \
        __attribute__((__weak__))

#elif defined(__GNUC__)

#define __STORAGE__ \
        __attribute__((__common__))

#elif defined(__cplusplus)

#define __STORAGE__ \
        __declspec(selectany)

#pragma warning(disable: 4099 4190)

#else /* __APPLE__ || __ICC */

#define __STORAGE__

#endif /* __APPLE__ || __ICC */

#endif /* __MULTI_CORE__ */

#define __in__(cdecl, decl) \
        decl
#define __in_type__(cdecl, decl) \
        decl
#define __texture_var(name) \
        __texture_##name
#define __cuda_host_device_name(name) \
        __cuda_host_device_##name
#define __val_param(name) \
        name
#define __copy_param(local_decl, param)
#define __unsized_array_size
#define __unsized__shared_var(name, s, type) \
        (*name)
#define __unsized__empty_array(s)

/* this is compiled with a host compiler for device emulation */
#define __device_emulation

#endif /* __CUDABE__ */

#include "builtin_types.h"
#include "device_launch_parameters.h"
#include "storage_class.h"
