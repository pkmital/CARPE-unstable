////////////////////////////////////////////////////////////////////////////
//	File:		ShaderMan.cpp
//	Author:		Changchang Wu
//	Description :	implementation of the ShaderMan class.
//					A Shader Manager that calls different implementation of shaders
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
#include <vector>
#include <iostream>
#include <strstream>
#include <algorithm>
using namespace std;
#include "ProgramGLSL.h"
#include "ProgramCG.h"
#include "stdlib.h"
#include "GlobalUtil.h"
#include "GLTexImage.h"
#include "SiftGPU.h"
#include "ShaderMan.h"
#include "math.h"
#include "FrameBufferObject.h"
#ifdef _WIN32
#include "imdebug/imdebuggl.h"
#endif
///
ShaderBag   * ShaderMan::s_bag = NULL;
FilterProgram  * ShaderMan::f_gaussian_skip0=NULL;
FilterProgram  * ShaderMan::f_gaussian_skip1=NULL;
FilterProgram  ** ShaderMan::f_gaussian_step=NULL;
vector<FilterProgram*> ShaderMan::f_gaussian_skip0_v; 
//
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// 0.28965f  0.60581f 0.10454f
void ShaderMan::InitShaderContext()
{
	if(GlobalUtil::_UseGLSL==0) ProgramCG::InitContext();


}

void ShaderMan::InitializeShaders()
{

	if(s_bag==NULL)
	{
		
		if(GlobalUtil::_UseGLSL)
		{
			s_bag =new ShaderBagGLSL() ;
		}
		else
		{
			s_bag = new ShaderBagCG();
		}
		s_bag->LoadFixedShaders();


		//change parameter according to hardware limit
		if(GlobalUtil::_SupportFP40==0 ) 
		{
			GlobalUtil::_MaxOrientation = 1;
			GlobalUtil::_ComputeDescriptors = 0;
			GlobalUtil::_SubpixelLocalization = 0;
			GlobalUtil::_UseNewDogAlgorithm = 0;
			std::cerr<<"Orientation simplified on this hardware"<<endl;
			std::cerr<<"Descriptor ignored on this hardware"<<endl;
		}
		if(GlobalUtil::_SubpixelLocalization || GlobalUtil::_UseNewDogAlgorithm)
		{
			//not depth buffer is required if using subpixel localization
			GlobalUtil::_DepthTexAllocation = 0;
		}
	}
	GlobalUtil::CheckErrorsGL();
}


void ShaderMan::DestroyShaders(int sigma_num)
{
	if(s_bag) delete s_bag;
	if(f_gaussian_skip1) delete f_gaussian_skip1;

	for(unsigned int i = 0; i < f_gaussian_skip0_v.size(); i++)
	{
		if(f_gaussian_skip0_v[i]) delete f_gaussian_skip0_v[i];
	}
	if(f_gaussian_step) 
	{
		for(int i = 0; i< sigma_num; i++)
		{
			delete f_gaussian_step[i];
		}
		delete[] f_gaussian_step;
	}
}

void ShaderMan::UnloadProgram()
{
	s_bag->UnloadProgram();
}





void ShaderMan::TextureCopy(GLTexImage*dst, GLTexImage*src)
{

	dst->AttachToFBO(0);

	src->BindTex();

	dst->FitTexViewPort();

	dst->DrawQuad();

	dst->UnbindTex();
//	ShaderMan::UnloadProgram();
	dst->DetachFBO(0);
	return;
}
void ShaderMan::TextureDownSample(GLTexImage *dst, GLTexImage *src, int scale)
{
	//output parameter
	
	dst->AttachToFBO(0);

	//input parameter
	src->BindTex();

	//
	dst->FitTexViewPort();

	s_bag->s_downsampling->UseProgram();

	dst->DrawQuadDS(scale);
	src->UnbindTex();

	UnloadProgram();

	dst->DetachFBO(0); 
	
//	imdebugTexImagef(GlobalUtil::_texTarget, dst->GetTexID(), GL_RGBA);

}

void ShaderMan::TextureUpSample(GLTexImage *dst, GLTexImage *src, int scale)
{

	//output parameter
	dst->AttachToFBO(0);
	//input parameter
	src->BindTex();

	dst->FitTexViewPort();

	GlobalUtil::SetTextureParameterUS();
	dst->DrawQuadUS(scale);
	src->UnbindTex();

	dst->DetachFBO(0);

	GlobalUtil::SetTextureParameter();
}



void ShaderMan::UseShaderDisplayGaussian()
{
	s_bag->s_display_gaussian->UseProgram();
}

void ShaderMan::UseShaderDisplayDOG()
{
	s_bag->s_display_dog->UseProgram();
}



void ShaderMan::UseShaderRGB2Gray()
{
	s_bag->s_gray->UseProgram();
}



void ShaderMan::UseShaderDisplayGradX()
{
	s_bag->s_display_gradx->UseProgram();
}

void ShaderMan::UseShaderDisplayGradY()
{
	s_bag->s_display_grady->UseProgram();
}

void ShaderMan::UseShaderDisplayKeypoints()
{
	s_bag->s_display_keys->UseProgram();
}


void ShaderMan::UseShaderCopyTexCoord()
{
	s_bag->s_texcoord->UseProgram();
}


void ShaderMan::LoadDogShaders(float dog_threshold, float edge_threshold)
{

	if(GlobalUtil::_SubpixelLocalization || GlobalUtil::_UseNewDogAlgorithm)
	{
		//s_bag->LoadKeypointShaderV2(dog_threshold, edge_threshold);
		s_bag->LoadKeypointShader(dog_threshold, edge_threshold);
	}else
	{
		s_bag->LoadDogShader1(dog_threshold, edge_threshold);
		s_bag->LoadDogShader2(dog_threshold, edge_threshold);
	}
}



void ShaderMan::UseShaderGradientPass()
{
	s_bag->s_grad_pass->UseProgram();

}

void ShaderMan::UseShaderDogPass1()
{
	s_bag->s_dog_pass1->UseProgram();

}

void ShaderMan::UseShaderDogZpass()
{
	s_bag->s_dog_zpass->UseProgram();

}

void ShaderMan::UseShaderDogPass2(int texU, int texD)
{
	s_bag->s_dog_pass2->UseProgram();
	s_bag->SetDogTexParam(texU, texD);
}


void ShaderMan::UseShaderKeypoint(int texU, int texD)
{
	s_bag->s_keypoint->UseProgram();
	s_bag->SetDogTexParam(texU, texD);
}



void ShaderMan::LoadGenListShader(int ndoglev, int nlev)
{
	s_bag->LoadGenListShader(ndoglev, nlev);
}

void ShaderMan::UseShaderGenListInit(int w, int h, int tight)
{
	if(tight)
	{
		s_bag->s_genlist_init->UseProgram();
	}else
	{
		s_bag->s_genlist_init_ex->UseProgram();
		s_bag->SetGenListInitParam(w, h);
	}

}

void ShaderMan::UseShaderGenListHisto()
{
	s_bag->s_genlist_histo->UseProgram();

}




void ShaderMan::UseShaderGenListStart(float fw, int tex0)
{
	s_bag->s_genlist_start->UseProgram();
	s_bag->SetGenListStartParam(fw, tex0);
}

void ShaderMan::UseShaderGenListStep(int tex, int tex0)
{
	s_bag->s_genlist_step->UseProgram();
	s_bag->SetGenListStepParam( tex,  tex0);
}

void ShaderMan::UseShaderGenListEnd()
{
	s_bag->s_genlist_end->UseProgram();
}

void ShaderMan::UseShaderDebug()
{
	if(s_bag->s_debug)	s_bag->s_debug->UseProgram();
}


void ShaderMan::UseShaderDisplayDOGABS()
{
	s_bag->s_display_abs_dog->UseProgram();
}

void ShaderMan::UseShaderDisplayGradXABS()
{
	s_bag->s_display_abs_gradx->UseProgram();
}

void ShaderMan::UseShaderDisplayGradYABS()
{
	s_bag->s_display_abs_grady->UseProgram();
}

void ShaderMan::UseShaderGenVBO( float width, float fwidth, float size)
{
	s_bag->s_vertex_list->UseProgram();
	s_bag->SetGenVBOParam(width, fwidth, size);
}
void ShaderMan::UseShaderMarginCopy(float xmax, float ymax)
{
	s_bag->s_margin_copy->UseProgram();
	s_bag->SetMarginCopyParam(xmax, ymax);
	
}
void ShaderMan::UseShaderCopyKeypoint()
{
	s_bag->s_copy_key->UseProgram();
}
void ShaderMan::UseShaderListReshapeInit()
{
	s_bag->s_list_reshape_init->UseProgram();
}
void ShaderMan::UseShaderListReshapeStart(float width)
{
	s_bag->s_list_reshape_start->UseProgram();
	s_bag->SetReshapeListStartWidth(width);
}
void ShaderMan::UseShaderListReshapeStep(int tex0, int dx, int dy)
{
	s_bag->s_list_reshape_step->UseProgram();
	s_bag->SetReshapeListStepParam(tex0, dx, dy);
}
void ShaderMan::UseShaderListReshapeEnd(int fTex, int oTex)
{
	s_bag->s_list_reshape_end->UseProgram();
	s_bag->SetReshapeListEndParam(fTex, oTex);

}
void ShaderMan::UseShaderSimpleOrientation(int oTex, float size)
{
	s_bag->s_orientation->UseProgram();
	s_bag->SetSimpleOrientationInput(oTex, size);
}



void ShaderMan::UseShaderOrientation(int gtex, int width, int height, float sigma, int stex, float step)
{
	s_bag->s_orientation->UseProgram();
	s_bag->SetFeatureOrientationParam(gtex, width, height, sigma, stex, step);
}

void ShaderMan::UseShaderDescriptor(int gtex,int dwidth, int fwidth,  int width, int height, float sigma)
{
	if(s_bag->s_descriptor_fp) s_bag->s_descriptor_fp->UseProgram();
	if(s_bag->s_descriptor_gp) s_bag->s_descriptor_gp->UseProgram();
	if(s_bag->s_descriptor_fp) s_bag->s_descriptor_fp->UseProgram();
	s_bag->SetFeatureDescirptorParam(gtex, (float)dwidth,  (float)fwidth, (float)width, (float)height, sigma);
	
}


void ShaderMan::SelectInitialSmoothingFilter(int octave_min, SiftParam&param)
{

	for(unsigned int i = 0; i < f_gaussian_skip0_v.size(); i++)
	{
		if(f_gaussian_skip0_v[i]->_id == octave_min)
		{
			f_gaussian_skip0 = f_gaussian_skip0_v[i];
			return ;
		}
	}
	FilterProgram * filter = (GlobalUtil::_UseGLSL?((FilterProgram*)( new FilterGLSL() )): ((FilterProgram*)( new FilterGLCG()))); 
	filter->CreateGaussianFilter(param.GetInitialSmoothSigma(octave_min));
	filter->_id = octave_min;
	f_gaussian_skip0_v.push_back(filter);
	f_gaussian_skip0 = filter; 


}
void ShaderMan::CreateGaussianFilters(SiftParam&param)
{
	FilterProgram * filter;

	if(param._sigma_skip0>0.0f) 
	{
		f_gaussian_skip0 = filter = (GlobalUtil::_UseGLSL?((FilterProgram*)( new FilterGLSL() )): ((FilterProgram*)( new FilterGLCG())));
		filter->CreateGaussianFilter(param._sigma_skip0);
		filter->_id = GlobalUtil::_octave_min_default; 
		f_gaussian_skip0_v.push_back(filter);
	}
	if(param._sigma_skip1>0.0f) 
	{
		f_gaussian_skip1 = filter = (GlobalUtil::_UseGLSL?((FilterProgram*)( new FilterGLSL() )): ((FilterProgram*)( new FilterGLCG())));;
		filter->CreateGaussianFilter(param._sigma_skip1);
	}

	f_gaussian_step = new FilterProgram*[param._sigma_num];
	for(int i = 0; i< param._sigma_num; i++)
	{
		f_gaussian_step[i] = filter = (GlobalUtil::_UseGLSL?((FilterProgram*)( new FilterGLSL() )): ((FilterProgram*)( new FilterGLCG())));;
		filter->CreateGaussianFilter(param._sigma[i]);
	}
}