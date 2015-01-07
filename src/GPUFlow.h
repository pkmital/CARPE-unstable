#ifndef _GPUFLOW_H
#define _GPUFLOW_H

// STD
#include <iomanip>
#include <iostream>

#ifdef _WIN32
    #pragma comment(lib, "common_static.lib")
    #pragma comment(lib, "flow.lib")
#else
    #pragma comment(lib, "libcommon.so")
    #pragma comment(lib, "libflow.so")
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

//#define FLOWLIB_USE_STATIC
//#define FLOWLIB_DLL_EXPORTS
//#define FLOWLIB_DLLAPI __declspec(dllimport)

#include "CommonLib.h"
#include "FlowLib.h"


class GPUFlow
{
public:
	GPUFlow(float scaleme = 1.0f);
	~GPUFlow();

	bool initOK();
	
	// don't really use this
	bool getFlow(float *img1, float *img2, float *vx, float *vy, int imgwidth, int imgheight);

	// one image at a time, if there aren't enough images to perform flow,
	// return false, otherwise, flow is in vx/vy
	bool getFlow(float *img1, float *vx, float *vy, int imgwidth, int imgheight);
	bool getColorFlowField(float *img1, float *colorflow, int imgwidth, int imgheight);

	// just returns the flow without calculating it
	bool getColorFlowField(float *colorflow, int imgwidth, int imgheight);


private:
	unsigned int prev_width, prev_height;
	FlowLib *flowlib;
	unsigned int num_images;
	float lambda;
	float gauss_tv_epsilon;
	unsigned int num_iter;
	unsigned int num_warps;
	float scale_factor;
	bool median_filt;
	bool diffusion_tensor;

};

#endif