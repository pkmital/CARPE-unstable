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
 * Module      : FlowLib Command line tool
 * Class       : $RCSfile$
 * Language    : C++/CUDA
 * Description : Implementation of FlowLib Command Line Tool
 *
 * Author     : Manuel Werlberger
 * EMail      : werlberger@icg.tugraz.at
 *
 */

// Simple Command Line Tool for testing the Flow Library

// system includes
#include <iostream>
using namespace std;
#include <math.h>
#include <stdlib.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <CommonLib.h>
#include <FlowLib.h>

////////////////////////////////////////////////////////////////////////////////
int main( int argc, char** argv)
{
  try {
    if(!CommonLib::selectDevice(0))
    {
      throw std::runtime_error(std::string( "Error selecting a propriate device!" ) );
      return -1;
    }

    bool success = true;

    //=============================================================================
    // Create Flow Library
    FlowLib flowlib;

    vector<string> filenames;
    filenames.push_back("../../data/RubberWhale10.pgm");
    filenames.push_back("../../data/RubberWhale11.pgm");

    unsigned int num_images = filenames.size();
    string flo_filename("RubberWhale.flo");
    float lambda = 40;
    float gauss_tv_epsilon = 0.01f;
    unsigned int num_iter = 40;
    unsigned int num_warps = 5;
    float scale_factor = 0.8f;
    bool median_filt = true;
    bool diffusion_tensor = true;
 
    std::cout << "[main] read input images..." << std::endl;
    //=============================================================================
    // Read all input images
    std::vector<Cuda::DeviceMemoryPitched<float,2>* > in_images;
	
    for(unsigned i = 0; i<num_images; ++i)
    {
      std::cout << "reading input image " << i+1 << ": "<< filenames[i] << std::endl;
      CommonLib::CudaPnmImage<float> pnm_image(filenames[i].c_str());
      Cuda::HostMemoryReference<float,2> *href = pnm_image.CUDAhostRef();
      Cuda::Size<2> new_size = flowlib.getOptimalMemSize(href->size);
      Cuda::DeviceMemoryPitched<float,2>* image = new Cuda::DeviceMemoryPitched<float,2>(new_size);
      image->setRegion(Cuda::Size<2>(0,0), href->size);
      Cuda::copy(*image,  *href, Cuda::Size<2>(0,0), Cuda::Size<2>(0,0), href->size);
      in_images.push_back(image);
    }

    //=============================================================================
    flowlib.setNumImages(num_images);

    // Set input images and parameters
    flowlib.setLambda(lambda);
    flowlib.setGaussTVEpsilon(gauss_tv_epsilon);
    flowlib.setNumIter(num_iter);
    flowlib.setNumWarps(num_warps);
    flowlib.setScaleFactor(scale_factor);
    //flowlib.useTextureRescale(true);
    flowlib.useMedianFilter(median_filt);
    flowlib.useDiffusionTensor(diffusion_tensor);
    flowlib.setStructureTextureDecomposition("ROF", 0.8f, 10.0f);

    bool ready = false;
    for(unsigned i = 0; i<num_images; ++i)
    {
      ready = flowlib.setInput(in_images[i]);
    }

    // Do Calculation (and measure time)
    if(!flowlib.runAlgorithm(0))
    {
      cout << "Error during calculation!" << endl;
      return -1;
    }
 
    //=============================================================================
    // Write Flow Field in .flo output format
    if(!flowlib.writeFlowField(flo_filename))
      std::cout << "ERROR writing .flo file" << std::endl;

  } 
  catch (std::exception &e)  // catch any exceptions
  { 
    cerr << "Error: " << e.what() << endl; 
    return 1;
  }
  return 1;
}

