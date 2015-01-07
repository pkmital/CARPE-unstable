// Matlab interface and host (CPU-side) code for doing GMM estimation.
// GPU-side code is in gmm_kernel.cu.
//
// Based on Mex Code from lecture slides here:
// http://www.nr.com/CS395T/lectures2008/11-GaussianMixtureModelsAndEM.pdf
//
// Modified for GPU support by Andrew Harp (andrew.harp@gmail.com)
// http://andrewharp.com/gmmcuda

#include "mex.h"
#include "nr3matlab.h"
#include "gpugaumixmod.h"
 
GpuGaumixmod *gpugmm = NULL;

/*
 * Convert a Fortran array(W,X,Y) to a C++ array[W][X][Y]
 */
void Fortran_to_C(Mat3DSing& dest, Doub* src) {
  Int d1 = dest.dim1();
  Int d2 = dest.dim2();
  Int d3 = dest.dim3();
  for (Int i = 0; i < d1; i++) {
    for (Int j = 0; j < d2; j++) {
      for (Int k = 0; k < d3; k++) {
        dest[i][j][k] = (Sing)(*src++);
      }
    }
  }
}

/*
 * Convert a C++ array[W][X][Y] to a Fortran array(W,X,Y)
 */
void C_to_Fortran(Doub* dest, Mat3DSing& src) {
  for (Int i = 0; i < src.dim1(); i++) {
    for (Int j = 0; j < src.dim2(); j++) {
      for (Int k = 0; k < src.dim3(); k++) {
        *(dest++) = src[i][j][k];
      }
    }
  }
}

/* Matlab usage: 
  gpugmm('construct',data, means)
  
  // Copy data to GPU.
  gpugmm('togpu');
  
  // Run computation on GPU.
  loglike = gpugmm('gstep',nsteps)
  
  // Copy resulting model back for analysis/display.
  gpugmm('fromgpu');
  
  [mean sig] = gpugmm(k)
  resp = gpugmm('response')
  gpugmm('delete')
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  int i, j, nn, kk, mm;
  int the_result = 0; 
  if (gpugmm) {
    nn=gpugmm->nn; 
    kk=gpugmm->kk; 
    mm=gpugmm->mm;
  
    for (int i_try = 0; i_try < gpugmm->n_tries; ++i_try) {
      if (gpugmm->loglikes[i_try] > gpugmm->loglikes[the_result]) {
        the_result = i_try;
      }
    }

    //printf("!! Found best loglike(%d): %0.2f\n",  best_result, gpugmm->loglike[best_result][0]);
  }
    
  if (gpugmm && (nrhs == 1 || nrhs == 2) && mxT(prhs[0]) == mxT<Doub>()) { // [mean sig] = gpugmm(k)
    Int k = Int( mxScalar<Doub>(prhs[0]) );
    
    // If they are requesting a result specifically, let them have it.
    if (nrhs > 1) {
      the_result = Int( mxScalar<Doub>(prhs[1]) );
    }
    
    if (nlhs > 0) {
      VecDoub mean(mm, plhs[0]);
      for (i=0;i<mm;i++) {
        mean[i] = gpugmm->means[the_result][k-1][i]; // k argument comes in 1-based
      }
    }
    
    if (nlhs > 1) {
      MatDoub sig(mm, mm, plhs[1]);
      for (i=0;i<mm;i++) {
        for (j=0;j<mm;j++) {
          sig[i][j] = gpugmm->sig[the_result][k-1][(i*mm)+j];
          //printf("%0.2f ", gpugmm->sig[best_result][k-1][(i*mm)+j]);
        }
        //printf("\n");
      }
    }
  } else if (nrhs == 1 && mxScalar<char>(prhs[0]) == 't') {        // gpugmm('togpu')
    gpugmm->copyToGpu();
  } else if (nrhs == 1 && mxScalar<char>(prhs[0]) == 'f') {        // gpugmm('fromgpu')
    gpugmm->copyFromGpu();
  } else if (nrhs == 1 && mxScalar<char>(prhs[0]) == 'd') {        // gpugmm('delete')
    if (gpugmm != NULL) {
      delete gpugmm;
    } else {
      printf("GMM was already NULL, can't delete!\n");
    }
    gpugmm = NULL;
  } else if (gpugmm && (nrhs == 1 || nrhs == 2) && mxScalar<char>(prhs[0]) == 'r') { // gpugmm('response')
    // If they are requesting a result specifically, let them have it.
    if (nrhs > 1) {
      the_result = Int( mxScalar<Doub>(prhs[1]) );
    }
    
    if (nlhs > 0) {
      MatDoub resp(nn, kk, plhs[0]);
      for (i=0;i<nn;i++) for (j=0;j<kk;j++) resp[i][j] = gpugmm->resp[the_result][i][j];
    }
  } else if (gpugmm && nrhs == 2 && mxT(prhs[1]) == mxT<Doub>() && (mxScalar<char>(prhs[0]) == 's' || mxScalar<char>(prhs[0]) == 'g')) { // deltaloglike = gpugmm('step',nsteps)
    if (DEBUGME) {
      printf("GPU method.\n");
    }
    
    Int nstep = Int(mxScalar<Doub>(prhs[1]));
    
    VecSing tmp(gpugmm->n_tries);
    for (i=0;i<nstep;i++) {
      tmp = gpugmm->estep();
      gpugmm->mstep();
    }
    
    if (nlhs > 0) {
      VecSing loglikes(gpugmm->n_tries, plhs[0]);
      for (i = 0; i < gpugmm->n_tries; ++i) {
        loglikes[i] = tmp[i];
      }
    }
  } else if (nrhs == 3 && mxT(prhs[0]) == mxT<char>() && mxScalar<char>(prhs[0]) == 'c') { // gpugmm('construct', data, means)
    if (gpugmm) {
      delete gpugmm;
    }
    
    MatSing data;
    
    int num_dims = mxGetNumberOfDimensions(prhs[2]);
    const Int *dimension_array = mxGetDimensions(prhs[2]);

    Mat3DSing means(num_dims > 2 ? dimension_array[2] : 1, 
                    dimension_array[1], 
                    dimension_array[0]);
        
    if (mxT(prhs[1]) == mxT<Doub>()) {
      if (DEBUGME) {
        printf("Constructing from doubles!\n");
      }
      
      MatDoub data_d(prhs[1]);
      data.resize(data_d.nrows(), data_d.ncols());
      for (int i = 0; i < data_d.nrows(); ++i)
        for (int j = 0; j < data_d.ncols(); ++j)
          data[i][j] = data_d[i][j];
      
      Doub* mean_d = (Doub *)mxGetData(prhs[2]);
      Fortran_to_C(means, mean_d);
      
      //means = tmp_means;
    } else {
      // TODO: make this work for singles...
      throw("Pass doubles for now!");
    }
    
    if (means.dim3() != data.ncols()) throw("wrong dims in gpugmm 1");
    if (means.dim2() >= data.nrows()) throw("wrong dims in gpugmm 2");
    // user probably didn't transpose
    
    gpugmm = new GpuGaumixmod(data, means);
  } else {
    throw("bad call to gpugmm");
  }
  return;
}