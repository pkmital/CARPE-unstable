////////////////////////////////////////////////////////////////////////////
//	File:		ProgramGPU.cpp
//	Author:		Changchang Wu
//	Description : Implementation of ProgramGPU and FilterProgram
//				  This part is independent of GPU language
//				  Different version for cg and GLSL are derived from this
//
//
//
//	Copyright (c) 2007 University of North Carolina at Chapel Hill
//	All Rights Reserved
//
//	Permission to use, copy, modify and distribute this software and its
//	documentation for educational, research and non-profit purposes, without
//	fee, and without a written agreement is hereby granted, provided that the
//	above copyright notice and the following paragraph appear in all copies.
//	
//	The University of North Carolina at Chapel Hill make no representations
//	about the suitability of this software for any purpose. It is provided
//	'as is' without express or implied warranty. 
//
//	Please send BUG REPORTS to ccwu@cs.unc.edu
//
////////////////////////////////////////////////////////////////////////////


#include "GL/glew.h"
#include <iostream>
#include <strstream>
#include <vector>
using namespace std;
#include "GlobalUtil.h"
#include "GLTexImage.h"
#include "ShaderMan.h"
#include "ProgramGPU.h"
#include "math.h"
#ifdef _WIN32
#include "imdebug\imdebuggl.h"
#endif




void FilterProgram::	CreateGaussianFilter(float sigma)
{
	//pixel inside 3*sigma box
	int sz = int( ceil( GlobalUtil::_FilterWidthFactor * sigma -0.5) ) ;//
	int width = 2*sz + 1;

	//filter size truncation
	if(GlobalUtil::_MaxFilterWidth >0 && width > GlobalUtil::_MaxFilterWidth)
	{
		std::cout<<"Filter size truncated from "<<width<<" to "<<GlobalUtil::_MaxFilterWidth<<endl;
		sz = GlobalUtil::_MaxFilterWidth>>1;
		width = 2 * sz + 1;
	}

	int i;
	float * kernel = new float[width];
	float * offset = new float[width];
	float   rv = 1.0f/(sigma*sigma);
	float   v, ksum =0; 

	// pre-compute filter
	for( i = -sz ; i <= sz ; ++i) 
	{
		kernel[i+sz] =  v = exp(-0.5f * i * i *rv) ;
		ksum += v;
		offset[i+sz] = (float)i;
	}

	//normalize the kernel
	rv = 1.0f/ksum;
	for(i = 0; i< width ;i++) kernel[i]*=rv;
	//
	
	s_shader_h = CreateFilterH(kernel, offset, width);
	s_shader_v = CreateFilterV(kernel, offset, width);

	_size = sz;
	delete [] kernel;
	delete [] offset;

	if(GlobalUtil::_verbose) std::cout<<"Gaussian Filter: sigma = "<<sigma<<", size = "<<width<<"x"<<width<<endl;
}




void FilterProgram::RunFilter(GLTexImage *dst, GLTexImage *src, GLTexImage*tmp)
{

	//assume there is an fbo....
	// fill margin for out-of-boundary lookup
	src->FillMargin(_size, 0);

	//output parameter
	if(tmp) tmp->AttachToFBO(0);
	else dst->AttachToFBO(0);



	//input parameter

	src->BindTex();
	dst->FitTexViewPort();

	//horizontal filter
	s_shader_h->UseProgram();


	//run filter
	if(GlobalUtil::_MultiTexCoord)
		dst->DrawQuadMTH();
	else
		dst->DrawQuad();


	//parameters
	if(tmp)
	{
		// fill margin for out-of-boundary lookup
		tmp->FillMargin(0, _size);
		///
		tmp->DetachFBO(0);
		dst->AttachToFBO(0);
		tmp->BindTex();
	}
	else
	{
		glFinish();
		// fill margin for out-of-boundary lookup
		dst->FillMargin(0, _size);
		//
		dst->BindTex();
	}

	//vertical filter
	s_shader_v->UseProgram();

	//run
	if(GlobalUtil::_MultiTexCoord)
		dst->DrawQuadMTV();
	else
		dst->DrawQuad();



	//clean up
	dst->UnbindTex();
	dst->DetachFBO(0);

	//
	ShaderMan::UnloadProgram();
//	if(GlobalUtil::_debug)imdebugTexImagef(GlobalUtil::_texTarget, dst->GetTexID(), GL_RGBA);

}


ShaderBag::~ShaderBag()
{
		if(s_debug)delete s_debug;
		if(s_orientation)delete s_orientation;
		if(s_display_gaussian)delete s_display_gaussian;
		if(s_display_dog)delete s_display_dog;
		if(s_display_gradx)delete s_display_gradx;
		if(s_display_grady)delete s_display_grady;
		if(s_display_abs_dog)delete s_display_abs_dog;
		if(s_display_abs_gradx)delete s_display_abs_gradx;
		if(s_display_abs_grady)delete s_display_abs_grady;
		if(s_display_keys)delete s_display_keys;
		if(s_downsampling)delete s_downsampling;
		if(s_circum_extrema)delete s_circum_extrema;
		if(s_upsampling)delete s_upsampling;
		if(s_grad_pass)delete s_grad_pass;
		if(s_dog_pass1)delete s_dog_pass1;
		if(s_dog_zpass)delete s_dog_zpass;
		if(s_dog_pass2)delete s_dog_pass2;
		if(s_keypoint)delete s_keypoint;
		if(s_genlist_init)delete s_genlist_init;
		if(s_genlist_init_ex)delete s_genlist_init_ex;
		if(s_genlist_histo)delete s_genlist_histo;
		if(s_genlist_start)delete s_genlist_start;
		if(s_genlist_step)delete s_genlist_step;
		if(s_genlist_end)delete s_genlist_end;
		if(s_vertex_list)delete s_vertex_list;
		if(s_list_reshape_init)delete s_list_reshape_init;
		if(s_list_reshape_start)delete s_list_reshape_start;
		if(s_list_reshape_step)delete s_list_reshape_step;
		if(s_list_reshape_end)delete s_list_reshape_end;
		if(s_texcoord)delete s_texcoord;
		if(s_descriptor_fp)delete s_descriptor_fp;
		if(s_descriptor_vp)delete s_descriptor_vp;
		if(s_descriptor_gp)delete s_descriptor_gp;
		if(s_margin_copy) delete s_margin_copy;
}