// Some utility code to make my life interfacing Matlab and CUDA easier.
//
// Andrew Harp (andrew.harp@gmail.com)
// http://andrewharp.com/gmmcuda

typedef float Sing;
template <> inline mxClassID mxT<Sing>() {return mxSINGLE_CLASS;}

typedef const NRMat3d<Sing> Mat3DSing_I;
typedef NRMat3d<Sing> Mat3DSing, Mat3DSing_O, Mat3DSing_IO;

typedef const NRvector<Sing> VecSing_I;
typedef NRvector<Sing> VecSing, VecSing_O, VecSing_IO;

typedef const NRmatrix<Sing> MatSing_I;
typedef NRmatrix<Sing> MatSing, MatSing_O, MatSing_IO;

//struct preGpuGaumixmod {
//  static Int mmstat;
//  struct Mat_mm :
//    MatSing {
//      Mat_mm() : MatSing(mmstat, mmstat) {}
//    };
//    
//    preGpuGaumixmod(Int mm) {mmstat = mm;}
//};
//Int preGpuGaumixmod::mmstat = -1;

///////////////////////////////////////////////////////////////////////////

#define  CUDASAFE( call) do {                                             \
    int err = call;                                                       \
    if(0 != err) {                                                        \
        cudaError_t terr = cudaGetLastError();                            \
        printf("\nError: %s\n", cudaGetErrorString( terr));               \
        throw("CUDA error!");                                             \
    } } while (0)
 
///////////////////////////////////////////////////////////////////////////
      
void double2float(double *input_double, float *output_float, int Ntot) {
  for (int i = 0; i < Ntot; i++) {
    output_float[i] = (float) input_double[i];
  }
}
      
///////////////////////////////////////////////////////////////////////////

void float2double(float *input_float, double *output_double, int Ntot) {
  for (int i = 0; i < Ntot; i++) {
    output_double[i] = (double) input_float[i];
  }
}
      
///////////////////////////////////////////////////////////////////////////
      
// the entire gmm and get at it through sig.
float* copyToDevice(Mat3DSing& m3d,
                    float* d_m3d, bool transpose = false) {
  
  const int ndepth = m3d.dim1();
  const int nrows  = m3d.dim2();
  const int ncols  = m3d.dim3();

  const int matsize = nrows*ncols;
  
  const int data_size = sizeof(float) * nrows * ncols * ndepth;
  float* host_mem = (float*)malloc(data_size);
  
  int d, i, j;
  
  if (!transpose) {
    for (d = 0; d < ndepth; ++d) {
      for (i = 0; i < nrows; ++i) {
        for (j = 0; j < ncols; ++j) {
          host_mem[(d*matsize)+(i*ncols)+j] = m3d[d][i][j];
        }
      }
    }
  } else {
    for (d = 0; d < ndepth; ++d) {
      for (i = 0; i < nrows; ++i) {
        for (j = 0; j < ncols; ++j) {
          host_mem[(d*matsize)+(j*nrows)+i] = m3d[d][i][j];
        }
      }
    }
  }

  if (d_m3d == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_m3d, data_size));
  }
  
  CUDASAFE(cudaMemcpy(d_m3d, host_mem, data_size, cudaMemcpyHostToDevice));
  
  free(host_mem);
  return d_m3d;
}

///////////////////////////////////////////////////////////////////////////

float* copyToDevice(MatSing& mat, float* d_mat, bool transpose=false) {
  int data_size = sizeof(float) * mat.nrows() * mat.ncols();
  
  float* host_mem = (float*)malloc(data_size);
  
  const int nrows = mat.nrows();
  const int ncols = mat.ncols();
  
  unsigned int i, j;
  
  if (transpose) {
    for (i=0; i<nrows; ++i) {
      for (j=0; j<ncols; ++j) {
        host_mem[(j*nrows)+i] = mat[i][j];
      }
    }
  } else {
    for (i=0; i<nrows; ++i) {
      memcpy(host_mem + (i*ncols), mat[i], ncols * sizeof(float));
    }
  }
    
  if (d_mat == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_mat, data_size));
  }
  
  CUDASAFE(cudaMemcpy(d_mat, host_mem, data_size, cudaMemcpyHostToDevice));
  
  free(host_mem);
  return d_mat;
}

///////////////////////////////////////////////////////////////////////////

float* preallocate(Mat3DSing& m3d, float* d_m3d) {
  unsigned int data_size = sizeof(float) * m3d.dim1() 
                                         * m3d.dim2() 
                                         * m3d.dim3();
  if (d_m3d == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_m3d, data_size));
  }
  return d_m3d;
}

///////////////////////////////////////////////////////////////////////////

float* preallocate(VecSing& vec, float* d_vec) {
  unsigned int data_size = sizeof(float) * vec.size();
  if (d_vec == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_vec, data_size));
  }
  return d_vec;
}

///////////////////////////////////////////////////////////////////////////

float* preallocate(unsigned int  data_size) {
  float* d_foo;
  CUDASAFE(cudaMalloc((void**) &d_foo, data_size));
  return d_foo;
}

///////////////////////////////////////////////////////////////////////////

float* preallocate(MatSing& mat, float* d_mat) {
  const int nrows = mat.nrows();
  const int ncols = mat.ncols();
  
  int data_size = sizeof(float) * nrows * ncols;
  if (d_mat == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_mat, data_size));
  }
  
  return d_mat;
}

///////////////////////////////////////////////////////////////////////////
float* copyToDevice(VecSing& vec, float* d_vec) {
  unsigned int data_size = sizeof(float) * vec.size();
  
  float* host_mem = (float*)malloc(data_size);
  for (int i=0; i < vec.size(); ++i) {
    host_mem[i] = (float)(vec[i]);
  }
  
  if (d_vec == NULL) {
    CUDASAFE(cudaMalloc((void**) &d_vec, data_size));
  }
  
  CUDASAFE(cudaMemcpy(d_vec, host_mem, data_size, cudaMemcpyHostToDevice));

  free(host_mem);
  
  return d_vec;
}

///////////////////////////////////////////////////////////////////////////

void copyFromDevice(Mat3DSing& m3d, 
                    float* d_m3d, bool transpose = false) {

  const int ndepth = m3d.dim1();
  const int nrows  = m3d.dim2();
  const int ncols  = m3d.dim3();

  const int matsize = nrows*ncols;
  
  int data_size = sizeof(float) * nrows * ncols * ndepth;
  float* host_mem = (float*)malloc(data_size);
  
  CUDASAFE(cudaMemcpy(host_mem, d_m3d, data_size, cudaMemcpyDeviceToHost));
      
  int d, i, j;
  if (!transpose) {
    for (d = 0; d < ndepth; ++d) {
      for (i = 0; i < nrows; ++i) {
        for (j = 0; j < ncols; ++j) {
          m3d[d][i][j] = host_mem[(d*matsize)+(i*ncols)+j];
        }
      }
    }
  } else {
    for (d = 0; d < ndepth; ++d) {
      for (i = 0; i < nrows; ++i) {
        for (j = 0; j < ncols; ++j) {
          m3d[d][i][j] = host_mem[(d*matsize)+(j*nrows)+i];
        }
      }
    }
  }
  
  free(host_mem);
}

///////////////////////////////////////////////////////////////////////////

void copyFromDevice(MatSing& mat, float* d_mat, bool transpose=false) {
  const int nrows = mat.nrows();
  const int ncols = mat.ncols();
  
  int data_size = sizeof(float) * nrows * ncols;
  
  float* host_mem = (float*)malloc(data_size);
  CUDASAFE(cudaMemcpy(host_mem, d_mat, data_size, cudaMemcpyDeviceToHost));
  
  for (int i=0; i<nrows; ++i) {
    for (int j=0; j<ncols; ++j) {
      if (transpose) {
        mat[i][j] = host_mem[(j*nrows)+i];
      } else {
        mat[i][j] = host_mem[(i*ncols)+j];
      }
    }
  }
  
  free(host_mem);
}

///////////////////////////////////////////////////////////////////////////

void copyFromDevice(VecSing& vec, float* d_vec) {
  unsigned int data_size = sizeof(float) * vec.size();

  float* host_mem = (float*)malloc(data_size);
  CUDASAFE(cudaMemcpy(host_mem, d_vec, data_size, cudaMemcpyDeviceToHost));

  for (int i=0; i < vec.size(); ++i) {
    vec[i] = host_mem[i];
  }
  
  free(host_mem);
}

///////////////////////////////////////////////////////////////////////////

