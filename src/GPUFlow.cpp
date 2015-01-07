#include "GPUFlow.h"

GPUFlow::GPUFlow(float scaleme)
{
	prev_width = -1;
	prev_height = -1;
	num_images = 3;
	lambda = 40;
	gauss_tv_epsilon = 0.01f;
	num_iter = 20;
	num_warps = 3;
	scale_factor = scaleme;
	median_filt = true;
	diffusion_tensor = true;
	flowlib = NULL;
	if(!CommonLib::selectDevice(0))
	{
		throw std::runtime_error(std::string( "Error selecting appropriate device!" ) );
	}
}
GPUFlow::~GPUFlow()
{
	if(flowlib)
		flowlib->FlowLib::~FlowLib();
}

bool GPUFlow::initOK()
{
	/*
Functions
• CUresult cuInit (unsigned int Flags)
Initialize the CUDA driver API.
3.17.1 Detailed Description
This section describes the initialization functions of the low-level CUDA driver application programming interface.
3.17.2 Function Documentation
3.17.2.1 CUresult cuInit (unsigned int Flags)
Initializes the driver API and must be called before any other function from the driver API. Currently, the Flags
parameter must be 0. If cuInit() has not been called, any function from the driver API will return CUDA_ERROR_-
NOT_INITIALIZED.
Parameters:
Flags - Initialization flag for CUDA.
Returns:
CUDA_SUCCESS, CUDA_ERROR_INVALID_VALUE, CUDA_ERROR_INVALID_DEVICE
Note:
Note that this function may also return error codes from previous, asynchronous launches.
Generated onWed
*/
	return (cuInit(0) == CUDA_SUCCESS);
}
bool GPUFlow::getFlow(float *img1, float *img2, float *vx, float *vy, int imgwidth, int imgheight)
{
	flowlib->cleanup();
	flowlib->reset();
	//=============================================================================
	// Put the images on the GPU
	Cuda::HostMemoryReference<float,2> img1_href(Cuda::Size<2>(imgwidth,imgheight), img1);
	Cuda::Size<2> new_size = flowlib->getOptimalMemSize(img1_href.size);
	Cuda::DeviceMemoryPitched<float, 2>* img1_dm = new Cuda::DeviceMemoryPitched<float, 2>(new_size);
	img1_dm->setRegion(Cuda::Size<2>(0, 0), img1_href.size);
	Cuda::copy(*img1_dm,  img1_href, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), img1_href.size);

	Cuda::HostMemoryReference<float,2> img2_href(Cuda::Size<2>(imgwidth,imgheight), img2);
	Cuda::Size<2> new_size2 = flowlib->getOptimalMemSize(img2_href.size);
	Cuda::DeviceMemoryPitched<float, 2>* img2_dm = new Cuda::DeviceMemoryPitched<float, 2>(new_size2);
	img2_dm->setRegion(Cuda::Size<2>(0, 0), img2_href.size);
	Cuda::copy(*img2_dm,  img2_href, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), img2_href.size);


	//=============================================================================
	// Set input images and parameters
	flowlib->setNumImages(num_images);
	flowlib->setLambda(lambda);
	flowlib->setGaussTVEpsilon(gauss_tv_epsilon);
	flowlib->setNumIter(num_iter);
	flowlib->setNumWarps(num_warps);
	flowlib->setScaleFactor(scale_factor);
	flowlib->useDiffusionTensor(diffusion_tensor);
	flowlib->setStructureTextureDecomposition("ROF", 0.8f, 10.0f);

	// Establish the input on the flowlib library
	bool ready = flowlib->setInput(img1_dm, img2_dm);

	if(!ready)
	{
		printf("Error during flowlib setInput(img,img2)!");
		return false;
	}   
	// Do Calculation (and measure time)
	if(!flowlib->runAlgorithm(0)) {
		//mexErrMsgTxt("Error during calculation!");
		return false;
	}

	printf("5");
	//=============================================================================
	// Now get the Flow information off the GPU into CPU

	// Ptr to the gpu memory
	Cuda::DeviceMemoryPitched<float,2> *U_dev;
	Cuda::DeviceMemoryPitched<float,2> *V_dev;

	U_dev = flowlib->getFlowU();
	V_dev = flowlib->getFlowV();

	// Setup host memory for retrieving the UV flow
	Cuda::HostMemoryReference<float,2> U_href( Cuda::Size<2>(imgwidth,imgheight), vx );
	Cuda::HostMemoryReference<float,2> V_href( Cuda::Size<2>(imgwidth,imgheight), vy );

	// Copy to host
	Cuda::copy(U_href, *U_dev, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), Cuda::Size<2>(imgwidth,imgheight));
	Cuda::copy(V_href, *V_dev, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), Cuda::Size<2>(imgwidth,imgheight));

	img1_dm->Cuda::DeviceMemoryPitched<float, 2>::~DeviceMemoryPitched();
	img2_dm->Cuda::DeviceMemoryPitched<float, 2>::~DeviceMemoryPitched();
	printf("6");
	return true;
}

// one image at a time, if there aren't enough images to perform flow,
// return false, otherwise, flow is in vx/vy
bool GPUFlow::getFlow(float *img1, float *vx, float *vy, int imgwidth, int imgheight)
{
	if(prev_width != imgwidth && prev_height != imgheight)
	{
		if(flowlib == NULL)
			flowlib = new FlowLib;
		else
		{
			delete flowlib;
			flowlib = new FlowLib;
		}


		flowlib->setVerbose(1);

		//=============================================================================
		// Set input images and parameters
		flowlib->setNumImages(num_images);
		flowlib->setLambda(lambda);
		flowlib->setGaussTVEpsilon(gauss_tv_epsilon);
		flowlib->setNumIter(num_iter);
		flowlib->setNumWarps(num_warps);
		flowlib->setScaleFactor(scale_factor);
		flowlib->useDiffusionTensor(diffusion_tensor);
		flowlib->setStructureTextureDecomposition("ROF", 0.8f, 10.0f);

		prev_width = imgwidth;
		prev_height = imgheight;
	}

	/** Alternative input method.
	* @param buffer Float buffer containing image data.
	* @param width Image width.
	* @param height Image height.
	* @param stride Image pitch. (Number of _bytes_ in a row)
	* @param on_device Flag if buffer resides on the device (TRUE) or on the host (FALSE).
	*/
	//bool setInput(float* buffer, int width, int height, size_t pitch, bool on_device = true);
	// Establish the input on the flowlib library
	//    bool ready = flowlib->setInput(img1_dm, NULL, NULL);

	bool ready = flowlib->setInput(img1, imgwidth, imgheight, sizeof(float)*imgwidth, false);

	if(!ready)
	{
		printf("Warning: not ready for calculating flow\n");
		return false;
	}   
	// Do Calculation (and measure time)
	if(!flowlib->runAlgorithm(0)) {
		printf("Error during calculation!\n");
		return false;
	}

	//=============================================================================
	// Now get the Flow information off the GPU into CPU

	// Ptr to the gpu memory
	Cuda::DeviceMemoryPitched<float,2> *U_dev;
	Cuda::DeviceMemoryPitched<float,2> *V_dev;

	U_dev = flowlib->getFlowU();
	V_dev = flowlib->getFlowV();

	// Setup host memory for retrieving the UV flow
	Cuda::HostMemoryReference<float,2> U_href( Cuda::Size<2>(imgwidth,imgheight), vx );
	Cuda::HostMemoryReference<float,2> V_href( Cuda::Size<2>(imgwidth,imgheight), vy );

	// Copy to host
	Cuda::copy(U_href, *U_dev, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), Cuda::Size<2>(imgwidth,imgheight));
	Cuda::copy(V_href, *V_dev, Cuda::Size<2>(0, 0), Cuda::Size<2>(0, 0), Cuda::Size<2>(imgwidth,imgheight));

	return true;
}

bool GPUFlow::getColorFlowField(float *img1, float *colorflow, int imgwidth, int imgheight)
{

	if(prev_width != imgwidth && prev_height != imgheight)
	{
		if(flowlib == NULL)
			flowlib = new FlowLib;
		else
		{
			delete flowlib;
			flowlib = new FlowLib;
		}


		flowlib->setVerbose(1);

		//=============================================================================
		// Set input images and parameters
		flowlib->setNumImages(num_images);
		flowlib->setLambda(lambda);
		flowlib->setGaussTVEpsilon(gauss_tv_epsilon);
		flowlib->setNumIter(num_iter);
		flowlib->setNumWarps(num_warps);
		flowlib->setScaleFactor(scale_factor);
		flowlib->useDiffusionTensor(diffusion_tensor);
		flowlib->setStructureTextureDecomposition("ROF", 0.8f, 10.0f);

		prev_width = imgwidth;
		prev_height = imgheight;
	}

	/** Alternative input method.
	* @param buffer Float buffer containing image data.
	* @param width Image width.
	* @param height Image height.
	* @param stride Image pitch. (Number of _bytes_ in a row)
	* @param on_device Flag if buffer resides on the device (TRUE) or on the host (FALSE).
	*/
	//bool setInput(float* buffer, int width, int height, size_t pitch, bool on_device = true);
	// Establish the input on the flowlib library
	//    bool ready = flowlib->setInput(img1_dm, NULL, NULL);

	bool ready = flowlib->setInput(img1, imgwidth, imgheight, sizeof(float)*imgwidth, false);

	if(!ready)
	{
		printf("Warning: not ready for calculating flow\n");
		return false;
	}   
	// Do Calculation (and measure time)
	if(!flowlib->runAlgorithm(0)) {
		printf("Error during calculation!\n");
		return false;
	}

	//=============================================================================
	// Now get the Flow information off the GPU into CPU

	// Ptr to the gpu memory
	Cuda::DeviceMemoryPitched<float,3> *CFF_dev;

	CFF_dev = flowlib->getColorFlowField(0);

	// Setup host memory for retrieving the UV flow as a 3channel RGB
	Cuda::HostMemoryReference<float,3> CFF_href( Cuda::Size<3>(imgwidth,imgheight,3), colorflow );

	// Copy to host
	Cuda::copy(CFF_href, *CFF_dev, Cuda::Size<3>(0, 0, 0), Cuda::Size<3>(0, 0, 0), Cuda::Size<3>(imgwidth, imgheight, 3));

	CFF_href.Cuda::HostMemoryReference<float, 3>::~HostMemoryReference();
	CFF_dev->Cuda::DeviceMemoryPitched<float, 3>::~DeviceMemoryPitched();

	return true;
}


bool GPUFlow::getColorFlowField(float *colorflow, int imgwidth, int imgheight)
{
	//=============================================================================
	// Get the Flow information off the GPU into CPU

	// Ptr to the gpu memory
	Cuda::DeviceMemoryPitched<float,3> *CFF_dev;

	CFF_dev = flowlib->getColorFlowField(0);

	// Setup host memory for retrieving the UV flow as a 3channel RGB
	Cuda::HostMemoryReference<float,3> CFF_href( Cuda::Size<3>(imgwidth,imgheight,3), colorflow );

	// Copy to host
	Cuda::copy(CFF_href, *CFF_dev, Cuda::Size<3>(0, 0, 0), Cuda::Size<3>(0, 0, 0), Cuda::Size<3>(imgwidth, imgheight, 3));

	CFF_href.Cuda::HostMemoryReference<float, 3>::~HostMemoryReference();
	CFF_dev->Cuda::DeviceMemoryPitched<float, 3>::~DeviceMemoryPitched();

	return true;
}