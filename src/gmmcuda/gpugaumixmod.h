// Host side code for GPU enabled GMM EM algorithm.
// See gpugaumixmod_kernel.h for GPU-side code (kernels).
//
// Andrew Harp (andrew.harp@gmail.com)
// http://andrewharp.com/gmmcuda
#include <ctime>
// Where the GPU goodness lives.
// As an include may not be the best way to integrate the file, but it works.
#include "gpugaumixmod_kernel.h"

// Some helper functions I wrote.
#include "nr_cuda_util.h"

// Print verbose debug information.
#define DEBUGME 0

// Display timing information.
#define TIME 0
        
///////////////////////////////////////////////////////////////////////////

struct GpuGaumixmod {
  // GPU memory pointers, corresponding to struct members in gaumixmod.h
  float* d_frac;
  float* d_lndets;
  float* d_resp;
  float* d_data;
  float* d_means;
  float* d_sig;
  float* d_ll_chunks;
  
  VecSing foo;
  
  Int n_tries;
  Int nn;                 // num data
  Int kk;                 // num clusters
  Int mm;                 // data dimensionality
  MatSing data;           // the actual data
  
  Mat3DSing means;        // Cluster centroids
  Mat3DSing resp;         // Data assignment weights?  Add to 1?
  
  MatSing frac;           // Percentage of weight assigned to each cluster.
  MatSing lndets;         // log determinants?
  Mat3DSing sig;          // vector of covariance matrices for clusters
  
  MatSing ll_chunks;      // log likelihood of data given model.
  VecSing loglikes;
  
///////////////////////////////////////////////////////////////////////////
  
  GpuGaumixmod(MatSing &ddata, Mat3DSing &mmeans) :
            n_tries(mmeans.dim1()),
            nn(ddata.nrows()),
            kk(mmeans.dim2()),
            mm(ddata.ncols()),
            data(ddata),
            foo(ddata.nrows()),
            means(n_tries, kk, mm),
            resp(n_tries, nn, kk),
            frac(n_tries, kk),
            lndets(n_tries, kk),
            sig(n_tries, kk, mm * mm),
            ll_chunks(n_tries, NUM_CHUNKS),
            loglikes(n_tries),
            
            d_frac(NULL),
            d_lndets(NULL),
            d_resp(NULL),
            d_data(NULL),
            d_means(NULL),
            d_sig(NULL),
            d_ll_chunks(NULL) {
    if (DEBUGME) {
      printf("Constructing GpuGaumixmod! %d %d %d\n", n_tries, kk, mm);
    }
    
    Int i, j, k, ntry;
    for (i=0; i < means.dim1(); ++i) {
      for (j=0; j < means.dim2(); ++j) {
        for (k=0; k < means.dim3(); ++k) {
          means[i][j][k] = mmeans[i][j][k];
        }
      }
    }
        
    for (ntry=0; ntry<n_tries; ++ntry) {
      for (i=0; i < NUM_CHUNKS; ++i) {
        ll_chunks[ntry][i] = 0.0f;
      }
      
      // For every cluster.
      for (k=0;k<kk;k++) {

        // Every cluster has an equal prior probability.
        frac[ntry][k] = 1./kk;

        // Step through and create the sigma matrix.
        for (i=0;i<mm;i++) {
          for (j=0;j<mm;j++) {
            sig[ntry][k][(i*mm) + j] = 0.;
          }
          sig[ntry][k][(i*mm) + i] = 1.0e-10;
        }
      }
    }
    
    if (DEBUGME) {
      printSelf();
    }
  }

///////////////////////////////////////////////////////////////////////////
            
  void printSelf() {
    Int i, j, k, ntry, m;
    
    printf("Number of tries: %d\n", n_tries);
    
    for (ntry = 0; ntry < n_tries; ++ntry) {
      printf("====== Try num: %d ======\n", ntry);
      
      printf("log likelihood: \n");
      for (i=0; i<NUM_CHUNKS; ++i) {
        printf("%0.2f ", ll_chunks[ntry][i]);
      }
      printf("\n");

      // For every cluster
      for (k=0; k<kk; ++k) {
        printf("  == Cluster %d\n", k);
        printf("Fraction: %0.2f\n", frac[ntry][k]);
        printf("Lndet: %0.2f\n", lndets[ntry][k]);

        printf("Mean: ");
        for (m=0; m < mm; ++m) {
          printf("%0.2f ", means[ntry][k][m]);
        }
        printf("\n");

        // For every cluster.
        printf("Sigma:\n", kk);
        // Step through and create the sigma matrix.
        for (i=0; i<mm; i++) {
          for (j=0; j<mm; j++) {
            printf("%.10f ", sig[ntry][k][(i*mm) + j]);
          }
          printf("\n");
        }
        printf("\n");
      }
      
      printf("Data:\n", kk);
      for (int n=0; n<nn; ++n) {
        for (k=0; k<kk; ++k) {
          printf("%0.2f ", resp[ntry][n][k]);
        }
        printf("\n");
      }
      
      printf("====== End Try num: %d ======\n", ntry);
    }
  }
  
///////////////////////////////////////////////////////////////////////////

  // Runs the Gpu version of the estimation step.
  VecSing estep() {
    if (d_frac == NULL) {
      throw("GMM must be copied to GPU before running EM steps on GPU!");
    }
		
    if (DEBUGME) {
      copyFromGpu();
      CUDASAFE(cudaThreadSynchronize());
      printSelf();
    }
    
    //
    {
      dim3 gridDim(kk, n_tries, 1);
      dim3 blockDim(BLOCK_SIZE, 1, 1);

      clock_t start(clock());
      if (TIME) printf("e1: ");
      estep_kernel<<<gridDim, blockDim, 0>>>(
              d_resp, d_frac, d_data, d_means, d_sig, d_lndets,
              kk, mm, nn);
      CUDASAFE(cudaThreadSynchronize());
      
      if (TIME) {
        clock_t finish(clock());
        printf("%0.4fs\n", (finish-start)/1000.0);
      }
      
      int err = cudaGetLastError();
      if (err != 0) throw("Kernel problem!");
    }
    
    if (DEBUGME) {
      printf("FINISHED ESTEP 1!!!!\n");
      copyFromGpu();
      CUDASAFE(cudaThreadSynchronize());
      printSelf();
    }
    
    //
    {
      dim3 gridDim(NUM_CHUNKS, n_tries, 1); // totally arbitrary
      dim3 blockDim(BLOCK_SIZE, 1, 1);
      
      clock_t start(clock());
      if (TIME) printf("e2: ");
      
      estep_normalize_kernel<<<gridDim, blockDim, 0>>>(
              d_resp, d_frac, d_data, d_means, d_sig, d_lndets, d_ll_chunks,
              kk, mm, nn);
      
      CUDASAFE(cudaThreadSynchronize());
      
      if (TIME) {
        clock_t finish(clock());
        printf("%0.4fs\n", (finish-start)/1000.0);
      }
      
      int err = cudaGetLastError();
      if (err != 0) throw("Kernel problem!");
    }
    
    copyFromDevice(ll_chunks, d_ll_chunks);
    CUDASAFE(cudaThreadSynchronize());
 
    for (int i_try=0; i_try < n_tries; ++i_try) {
      float sum = 0.0f;
      for (int i_chunk=0; i_chunk < NUM_CHUNKS; ++i_chunk) {
        sum += ll_chunks[i_try][i_chunk];
        ll_chunks[i_try][i_chunk] = 0.0f;
      }
      //printf("Loglike(%d): %0.2f\n", i_try, sum);
      loglikes[i_try] = sum;
    }
    
    return loglikes;
  }

///////////////////////////////////////////////////////////////////////////

  // Runs the Gpu version of the maximization step.
  void mstep() {
    if (d_frac == NULL) {
      throw("GMM must be copied to GPU before running EM steps on GPU!");
    }
    
    {
      dim3 gridDim(kk * n_tries, mm, 1);
      dim3 blockDim(BLOCK_SIZE, 1, 1);

      clock_t start(clock());
      if (TIME) printf("m1: ");
      
      mstep_kernel<<<gridDim, blockDim>>>(
              d_resp, d_frac, d_data, d_means, d_sig,
              kk, mm, nn);
      CUDASAFE(cudaThreadSynchronize());
      
      if (TIME) {
        clock_t finish(clock());
        printf("%0.4fs\n", (finish-start)/1000.0);
      }
    
      int err = cudaGetLastError();
      if (err != 0) throw("Kernel problem!");
    }
    
    {
      dim3 gridDim(kk * n_tries, mm * mm, 1);
      dim3 blockDim(BLOCK_SIZE, 1, 1);

      clock_t start(clock());
      if (TIME) printf("m2: ");
      
      mstep_sigma_kernel<<<gridDim, blockDim>>>(
              d_resp, d_frac, d_data, d_means, d_sig,
              kk, mm, nn);
      CUDASAFE(cudaThreadSynchronize());

      if (TIME) {
        clock_t finish(clock());
        printf("%0.4fs\n", (finish-start)/1000.0);
      }
      
      int err = cudaGetLastError();
      if (err != 0) throw("Kernel problem!");
    }
  }

///////////////////////////////////////////////////////////////////////////

  // Gotta clean up our mess.
  ~GpuGaumixmod() {
    if (DEBUGME) {
      printf("Deleting GpuGaumixmod!\n");
    }

    removeFromGPU();
  }

///////////////////////////////////////////////////////////////////////////

  // Copies the GMM data from this struct to the object.
  void copyToGpu() {
    if (DEBUGME) {
      printf("Copying: CPU >> GPU (to graphics device)\n");
    }

    d_frac   = copyToDevice(frac, d_frac);
    d_lndets = copyToDevice(lndets, d_lndets);

    d_resp   = preallocate(resp, d_resp);

    // Data never changes, so once it's over there don't bother updating it.
    if (d_data == NULL) {
      d_data   = copyToDevice(data, d_data, true);
      d_ll_chunks = copyToDevice(ll_chunks, d_ll_chunks);
    }

    d_means  = copyToDevice(means, d_means);

    d_sig    = copyToDevice(sig, d_sig);
    
    CUDASAFE(cudaThreadSynchronize());
  }

///////////////////////////////////////////////////////////////////////////

  // Copies the GMM data from the cpu back into this struct.
  void copyFromGpu() {
    if (DEBUGME) {
      printf("Copying: GPU >> CPU (from graphics device)\n");
    }
    
    copyFromDevice(frac, d_frac);
    copyFromDevice(lndets, d_lndets);

    copyFromDevice(resp, d_resp, true);
    //copyFromDevice(ll_chunks, d_ll_chunks);
    //Data never changes!
    //copyFromDevice(gmm->data, d_data, true);

    copyFromDevice(means, d_means);
    copyFromDevice(sig, d_sig);
    
    CUDASAFE(cudaThreadSynchronize());
  }

///////////////////////////////////////////////////////////////////////////

  // When the GMM goes away, we need to make our GPU stored data (if it
  // exists) go away as well.
  void removeFromGPU() {
    if (d_frac != NULL) {
      if (DEBUGME) {
        printf("Freeing memory on GPU.\n");
      }

      CUDASAFE(cudaFree(d_frac));
      CUDASAFE(cudaFree(d_lndets));
      CUDASAFE(cudaFree(d_resp));
      CUDASAFE(cudaFree(d_data));
      CUDASAFE(cudaFree(d_means));
      CUDASAFE(cudaFree(d_sig));
      CUDASAFE(cudaFree(d_ll_chunks));
      
      CUDASAFE(cudaThreadSynchronize());
    } else if(DEBUGME) {
      printf("Not freeing memory on GPU.\n");
    }

    d_ll_chunks = NULL;
    d_frac = NULL;
    d_lndets = NULL;
    d_resp = NULL;
    d_data = NULL;
    d_means = NULL;
    d_sig = NULL;
  } 
};