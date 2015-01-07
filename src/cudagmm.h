#pragma once _CUDAGMM_H

#include <gpugaumixmod.h>
#include <nr_cuda_util.h>

class cudaGMM
{
	cudaGMM()
	{
		gpugmm = NULL;

	}


	GpuGaumixmod *gpugmm;
}