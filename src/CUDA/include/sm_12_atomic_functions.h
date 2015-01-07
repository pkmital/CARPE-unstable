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

#if !defined(__SM_12_ATOMIC_FUNCTIONS_H__)
#define __SM_12_ATOMIC_FUNCTIONS_H__

#if defined(__cplusplus) && defined(__CUDACC__)

#if !defined(CUDA_NO_SM_12_ATOMIC_INTRINSICS)

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "host_defines.h"

extern "C"
{

/*DEVICE_BUILTIN*/
extern __device__ unsigned long long int __ullAtomicAdd(unsigned long long int *address, unsigned long long int val);
/*DEVICE_BUILTIN*/
extern __device__ unsigned long long int __ullAtomicExch(unsigned long long int *address, unsigned long long int val);
/*DEVICE_BUILTIN*/
extern __device__ unsigned long long int __ullAtomicCAS(unsigned long long int *address, unsigned long long int compare, unsigned long long int val);

/*DEVICE_BUILTIN*/
extern __device__ int                    __any(int cond);
/*DEVICE_BUILTIN*/
extern __device__ int                    __all(int cond);

}

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

static __inline__ __device__ unsigned long long int atomicAdd(unsigned long long int *address, unsigned long long int val)
{
  return __ullAtomicAdd(address, val);
}

static __inline__ __device__ unsigned long long int atomicExch(unsigned long long int *address, unsigned long long int val)
{
  return __ullAtomicExch(address, val);
}

static __inline__ __device__ unsigned long long int atomicCAS(unsigned long long int *address, unsigned long long int compare, unsigned long long int val)
{
  return __ullAtomicCAS(address, compare, val);
}

static __inline__ __device__ bool any(bool cond)
{
  return (bool)__any((int)cond);
}

static __inline__ __device__ bool all(bool cond)
{
  return (bool)__all((int)cond);
}

#endif /* !CUDA_NO_SM_12_ATOMIC_INTRINSICS */

#elif !defined(__CUDACC__)

#include "crt/func_macro.h"

#if !defined(__CUDABE__)

#if defined(__MULTI_CORE__)

#define __ullAtomicAdd(address, val) \
        __builtin___ullAtomicAdd(address, val)
#define __ullAtomicExch(address, val) \
        __builtin___ullAtomicExch(address, val)
#define __ullAtomicCAS(address, compare, val) \
        __builtin___ullAtomicCAS(address, compare, val)

#else /* __MULTI_CORE__ */

extern void CUDARTAPI __cudaMutexOperation(int lock);

#define __cudaAtomicOperation(code) \
        __cudaMutexOperation(1);    \
        code                        \
        __cudaMutexOperation(0);

__device_func__(unsigned long long int __ullAtomicAdd(unsigned long long int *address, unsigned long long int val))
{
  unsigned long long int old;

  __cudaAtomicOperation(
    old = *address;
    *address = old + val;
  )
  return old;
}

__device_func__(unsigned long long int __ullAtomicExch(unsigned long long int *address, unsigned long long int val))
{
  unsigned long long int old;

  __cudaAtomicOperation(
    old = *address;
    *address = val;
  )
  return old;
}

__device_func__(unsigned long long int __ullAtomicCAS(unsigned long long int *address, unsigned long long int compare, unsigned long long int val))
{
  unsigned long long int old;

  __cudaAtomicOperation(
    old = *address;
    *address = old == compare ? val : old;
  )
  return old;
}

#undef __cudaAtomicOperation

#endif /* __MULTI_CORE__ */

__device_func__(int __any(int cond))
{
  return cond;
}

__device_func__(int __all(int cond))
{
  return cond;
}

#endif /* !__CUDABE__ */

#endif /* __cplusplus && __CUDACC__ */

#endif /* !__SM_12_ATOMIC_FUNCTIONS_H__ */
