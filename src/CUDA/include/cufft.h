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

/*
* cufft.h  
* Public header file for the NVIDIA Cuda FFT library (CUFFT)  
*/ 
#ifndef _CUFFT_H_
#define _CUFFT_H_

#include <stdio.h>
#include "cuComplex.h"

#ifndef CUFFTAPI
#ifdef _WIN32
#define CUFFTAPI __stdcall
#else
#define CUFFTAPI 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// CUFFT API function return values 
typedef enum cufftResult_t {
  CUFFT_SUCCESS        = 0x0,
  CUFFT_INVALID_PLAN   = 0x1,
  CUFFT_ALLOC_FAILED   = 0x2,
  CUFFT_INVALID_TYPE   = 0x3,
  CUFFT_INVALID_VALUE  = 0x4,
  CUFFT_INTERNAL_ERROR = 0x5,
  CUFFT_EXEC_FAILED    = 0x6,
  CUFFT_SETUP_FAILED   = 0x7,
  CUFFT_INVALID_SIZE   = 0x8
} cufftResult;
    
// CUFFT defines and supports the following data types

// cufftHandle is a handle type used to store and access CUFFT plans.
typedef unsigned int cufftHandle;

// cufftReal is a single-precision, floating-point real data type.
typedef float cufftReal;

// cufftComplex is a single-precision, floating-point complex data type that 
// consists of interleaved real and imaginary components.
// typedef float cufftComplex[2];
typedef cuComplex cufftComplex;

// CUFFT transform directions 
#define CUFFT_FORWARD -1 // Forward FFT
#define CUFFT_INVERSE  1 // Inverse FFT

// CUFFT supports the following transform types 
typedef enum cufftType_t {
  CUFFT_R2C = 0x2a, // Real to Complex (interleaved)
  CUFFT_C2R = 0x2c, // Complex (interleaved) to Real
  CUFFT_C2C = 0x29  // Complex to Complex, interleaved
} cufftType;

cufftResult CUFFTAPI cufftPlan1d(cufftHandle *plan, 
                                 int nx, 
                                 cufftType type, 
                                 int batch);

cufftResult CUFFTAPI cufftPlan2d(cufftHandle *plan, 
                                 int nx, int ny,
                                 cufftType type);

cufftResult CUFFTAPI cufftPlan3d(cufftHandle *plan, 
                                 int nx, int ny, int nz, 
                                 cufftType type);

cufftResult CUFFTAPI cufftDestroy(cufftHandle plan);

cufftResult CUFFTAPI cufftExecC2C(cufftHandle plan, 
                                  cufftComplex *idata,
                                  cufftComplex *odata,
                                  int direction);

cufftResult CUFFTAPI cufftExecR2C(cufftHandle plan, 
                                  cufftReal *idata,
                                  cufftComplex *odata);

cufftResult CUFFTAPI cufftExecC2R(cufftHandle plan, 
                                  cufftComplex *idata,
                                  cufftReal *odata);

#ifdef __cplusplus
}
#endif

#endif /* _CUFFT_H_ */
