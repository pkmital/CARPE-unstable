/*
  nvcc ptxvars.cu -g -G --host-compilation=c -c -D__DEVICE_LAUCH_PARAMETERS_H__ -Xptxas -fext
 */
 
typedef struct dim3_16 {
    unsigned short x, y, z;
} dim3_16;

typedef struct dim2_16 {
    unsigned short x, y;
} dim2_16;

__shared__ dim3_16  threadIdx;
__shared__ dim2_16  blockIdx;
__shared__ dim3_16  blockDim;
__shared__ dim2_16  gridDim;

int $gpu_registers[128];
