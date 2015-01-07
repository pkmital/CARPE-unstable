/*
 * Copyright (c) ICG. All rights reserved.
 *
 * Institute for Computer Graphics and Vision
 * Graz University of Technology / Austria
 *
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the above copyright notices for more information.
 *
 *
 * Project     : vmgpu
 * Module      : FlowLib
 * Class       : 
 * Language    : C++/CUDA
 * Description : General Definitions for the FlowLib
 *
 * Author     : Manuel Werlberger
 * EMail      : werlberger@icg.tutgraz.at
 *
 */

#ifndef FLOWLIBDEFS_H
#define FLOWLIBDEFS_H

#include "FlowLibConstants.h"
#include "vmlib_check_cuda_error.h"

#include "cudatemplates/size.hpp"
#include "cudatemplates/hostmemoryheap.hpp"
#include "cudatemplates/devicememorypitched.hpp"
#include "cudatemplates/devicememoryreference.hpp"
// #include "cudatemplates/devicememorylinear.hpp"

// Some second-order prior defines
#define SO_ANISOTROPIC 1           // nevermind
#define SO_DANIELSSON 2            // (u_xx+u_yy, u_xx-u_yy, u_xy)^T
#define SO_QUAD_VARIATION 3        // (u_xx, sqrt(2)*u_xy, u_yy)^T
#define SO_FROB_HESSIAN 4          // (u_xx, u_xy, u_yx, u_yy)^T
// which algorithm do we choose?
#define SO_ALGORITHM SO_DANIELSSON

// Should the edge image be Gauss-smoothed (default), or
// TV-L1 denoised, before g is calculated?
//#define SO_EDGE_IMAGE_HL1

// If this variable is defined, the Huber-L1 algorithm is
// used to calculate the flow and the second order algorithm
// is only invoked on the finest scale.
//#define SO_INIT_WITH_HL1

// If you would like to use more sophisticated rescale filters for building the image
// pyramid and for prolongating the dual-variables and flow-field within the
// coarse-to-fine warping scheme, define this variable.
//#define FLOWLIB_CTF_SOPHISTICATED_RESCALING

// An early stopping criteria is evaluated based on the changes of u and v within a
// single warp
//#define FLOWLIB_EARLY_STOPPING

// Define if gradients Ix/Iy are noramlized with I_grad_sqr or not
//#define FLOWLIB_GRAD_NORMALIZATION

//-----------------------------------------------------------------------------
/* CUDA ERROR HANDLING (CHECK FOR CUDA ERRORS)
 */
#define FLOWLIB_CHECK_CUDA_ERROR()  VMLIB_CHECK_CUDA_ERROR()

//-----------------------------------------------------------------------------
/* Shared lib macros for windows dlls
*/
#ifdef WIN32
#pragma warning( disable : 4251 ) // disable the warning about exported template code from stl

   #ifdef FLOWLIB_USE_STATIC
      #define FLOWLIB_DLLAPI
   #else
      #ifdef FLOWLIB_DLL_EXPORTS
         #define FLOWLIB_DLLAPI __declspec(dllexport)
      #else
         #define FLOWLIB_DLLAPI __declspec(dllimport)
      #endif
   #endif
#else
   #define FLOWLIB_DLLAPI
#endif

//-----------------------------------------------------------------------------
/* Operator Definitions if C++0x standard is not supported
*/
#ifdef NO_NEW_CXX_STD
#define CUDA_USE_OLD_SIZE
namespace Cuda {

  template <class Type, unsigned Dim>
  Size<Dim> operator/(const Size<Dim> &v1, Type s)
  {
    Size<Dim> v;
  
    for(unsigned i = Dim; i--;)
      v[i] = static_cast<size_t>(v1[i] / s);
    
    return v;
  }

  template <class Type, unsigned Dim>
  Size<Dim> operator*(const Size<Dim> &v1, Type s)
  {
    Size<Dim> v;
  
    for(unsigned i = Dim; i--;)
      v[i] = static_cast<size_t>(v1[i] * s);
    
    return v;
  }
} // namespace Cuda
#endif // DNOC++0x

//-----------------------------------------------------------------------------
/* Helpers and Definitions for the FlowLib
*/
namespace CudaFlowLib {
  /*
    Gives back correct and optimal image sizes that fit the current BLOCK_SIZE.
  */
  static inline Cuda::Size<2> calculateOptimalCudaSize(const Cuda::Size<2>& size)
  {
    Cuda::Size<2> newSize = size;
    newSize[0] = ((newSize[0] % BLOCK_SIZE != 0) ? (newSize[0] / BLOCK_SIZE + 1) :
                  (newSize[0] / BLOCK_SIZE))*BLOCK_SIZE;
    newSize[1] = ((newSize[1] % BLOCK_SIZE != 0) ? (newSize[1] / BLOCK_SIZE + 1) :
                  (newSize[1] / BLOCK_SIZE))*BLOCK_SIZE;
    return newSize;
  }

  /** An enum that defines the filter method for str.-tex. descomp. */
  enum FilterMethod_t {
    NO_STR_TEX_DECOMP, /**< No Structure-Texture decomposition. */
    STR_TEX_LAPLACE, /**< Laplace filtering. */
    STR_TEX_ROF /**< TV ROF denoising. */
  };  
  /**
     Models for flow calculations. Sorry for the awful code due to the ifdefs...
  */
  enum Model {
    HL1, /**< Huber-L1 FPD OF using either 2- or 3-frame calculations. */
    HL1NLS, /**< Huber-L1 FPD with non-local search OF using either 2- or 3-frame calculations. -- currently only 2 frame version implemented!*/
    CHL1NLS, /**< Huber-L1 FPD with non-local search using 2-frame color images for calculations. */
//    SOL1,  /**< SOL1 optical flow using 2 frames. */
		HL1FEATURES  /**< Huber-L1 FPD OF with additional constraints (PTAM). */
  };

} // namespace CudaFlowLib


//-----------------------------------------------------------------------------
/** Flow Parameters structure holds all 'generic' parameters for flow calculations
 */
struct FLOWLIB_DLLAPI FlowParameters
{
  FlowParameters() :
    num_images(2), lambda(30.0f), gauss_tv_epsilon(0.01f), num_iter(30), num_warps(5), 
    num_scales(FLOWLIB_MAX_PYRAMIDAL_LEVELS), scale_factor(0.5f),     
    str_tex_filter_method(CudaFlowLib::STR_TEX_ROF), str_tex_weight(0.8f), str_tex_smoothing(10.0f),
    diffusion_tensor(true), diffusion_sigma(.8f), diffusion_alpha(10.f), diffusion_q(.8f), 
    images_swapped(false)
    {
    }

  unsigned int num_images; /**< Usually 2 or 3 frames used for flow calculations. */
  float lambda; /**< Amount of weighting for optical flow constraint against regularization. \n Default: 50.0 */
  float theta; /**< Pushes the approximation of u/v towards real u/v in the optimization scheme. \n Default: 0.1 */
  float gauss_tv_epsilon; /**< Parameter for gauss TV that modells the strength of the 'gaussification'. \n Default: 0.01 */

  unsigned int num_iter; /**< Number of iterations per warping step. \n Default: 10 */
  unsigned int num_warps; /**< Number of warps. \n Default: 5 */

  size_t num_scales; /**< Number of used scales. \n Default: MAX_PYRAMIDAL_LEVELS -- auotmatically determined due to scale factor.  \n This variable effects initialization of input data!*/
  float scale_factor; /**< Scale factor from one level to the next. \n Default: 0.5  \n This variable effects initialization of input data!*/

  CudaFlowLib::FilterMethod_t str_tex_filter_method; /**< Filter method for generating str.-tex. decomposed image. \n Default: STR_TEX_LAPLACE (fastest)  \n This variable effects initialization of input data!*/
  float str_tex_weight; /**< Proportion of structure towards texture. \n Default: 0.8 */ 
  float str_tex_smoothing; /**< Amount of denoising for building structure texture decomposed image. \n Default: 1.0 (for ROF use 10.0!) */

  bool diffusion_tensor; /**< Flag if diffusion tensor is calculated and used as g-weighting. \n Default: TRUE */
  float diffusion_sigma; /**< Amount of smoothing before calculating edges. \n Default: 2.0 */
  float diffusion_alpha; /**< Strength of diffusion tensor. \n Default: 5.0 */
  float diffusion_q; /**< paramter todo. \n Default: 0.5 */

  bool images_swapped; /**< Uses textures to rescale buffers. \n Default: FALSE \n This variable effects initialization of input data!*/

};

#endif // FLOWLIBDEFS_H
