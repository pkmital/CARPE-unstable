////////////////////////////////////////////////////////////////////////////
//	File:		GlobalUtil.cpp
//	Author:		Changchang Wu
//	Description : Global Utility class for SiftGPU
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
using namespace std;
#include "GlobalUtil.h"
#if defined(HIGH_RESOLUTION_TIME) && defined(_WIN32)
	#include <windows.h>
	#include "mmsystem.h"
	#define clock	timeGetTime
	#define CLOCKS_PER_SEC 1000.0f
#else
	#include "time.h"
#endif


//
int GlobalParam::		_verbose =  1;   
int	GlobalParam::       _verboseS = 1;  //pint out information of each step
int	GlobalParam::       _verboseO = 0;  //print out information of each octave
int	GlobalParam::       _verboseL = 0;	//print out information of each level
int	GlobalParam::		_verboseT = 0;	
GLuint GlobalParam::	_texTarget = GL_TEXTURE_RECTANGLE_ARB; //only this one is supported
GLuint GlobalParam::	_iTexFormat =GL_RGBA32F_ARB;	//only this one is supported
int	GlobalParam::		_debug = 0;		//enable debugt code?
int	GlobalParam::		_usePackedTex = 2;//not supported yet
int	GlobalParam::		_MultiTexCoord = 1;	//whether use multi texture coordinate in filter
int	GlobalParam::		_UseGLSL = 0;		//use GLSL or CG
int	GlobalParam::		_EdgeSupression = 1; //supress the features on the edge, only in CG ..
int GlobalParam::		_MaxFilterWidth = -1;	//maximum filter width, use when GPU is not good enough
float GlobalParam::     _FilterWidthFactor	= 4.0f;	//the filter size will be _FilterWidthFactor*sigma*2+1
float GlobalParam::		_OrientationWindowFactor = 3.0f;//orientation window be sigma*..*2
float GlobalParam::     _DescriptorWindowFactor = 3.0f; //descriptor sampling window factor
int	GlobalParam::		_ExtraDownSample = 0;	//when filter size is not truncated,, you can downsample more than one lelvels
int GlobalParam::		_SubpixelLocalization = 0; //sub-pixel and sub-scale localization 	
int	GlobalParam::       _MaxOrientation = 1;	//whether we find multiple orientations for each feature 
float GlobalParam::		_MaxFeaturePercent = 0.005f;//at most 0.005 of all pixels
int	GlobalParam::		_MaxLevelFeatureNum = 4096; //maximum number of features
int GlobalParam::		_FeatureTexBlock = 4; //feature texture storagte alignment
//if _ForceTightPyramid is not 0, pyramid will be reallocated to fit the size of input images.
//otherwise, pyramid can be reused for smaller input images. 
int GlobalParam::		_ForceTightPyramid = 0;
//use gpu or cpu to generate feature list ...gpu is a little bit faster
int GlobalParam::		_ListGenGPU =	1;	
int	GlobalParam::		_ListReshapeGPU = 1;
int GlobalParam::		_PreProcessOnCPU = 1; //convert rgb 2 intensity on gpu, down sample on GPU
//hardware parameter, automatically retrieved
int GlobalParam::		_texMaxDim = 2048;				//maximum texture size
int	GlobalParam::		_texMaxDimGL = 0; 
int	GlobalParam::		_IsATI = 1;					//GPU vendor
int	GlobalParam::		_MaxDrawBuffers = 0;		//max draw buffer
int	GlobalParam::		_ViewPortChanges = 0;		//track # of viewport changes
int	GlobalParam::       _MaxInstruction = 0;		//

//parameter decided according to hardware limitation and input parameter
//different gpu descriptor format for different method
//could be 8 (gl_max_draw_buffers = 4), 4(gl_max_draw_buffers= 8), or 32(geometry)
int	GlobalParam::		_DescriptorFormat = 8;

//whether orientation/descriptor is supported by hardware
int	GlobalParam::		_SupportFP40 = 0;
int	GlobalParam::       _ComputeDescriptors = 1;
int	GlobalParam::		_MRT8 = 0; //seems not hardware supports writing 8 frame buffers
int	GlobalParam::       _UseGeometryShader = 0;
int GlobalParam::		_SupportGeometryShader = 0;
int	GlobalParam::		_DepthTexAllocation = 1;

int GlobalParam::		_UseNewDogAlgorithm = 1;

//when SiftGPUEX is used, display VBO generation is skipped
int GlobalParam::		_UseSiftGPUEX = 0;
//default setting for whether the result is read back 
int GlobalParam::		_DownloadDefault = 1;
//
int GlobalParam::		_InitPyramidWidth=0;
int GlobalParam::		_InitPyramidHeight=0;
int	GlobalParam::		_octave_min_default=0;
int	GlobalParam::		_octave_num_default=-1;
int	GlobalParam::		_HaveGlSupport = -1;
int	GlobalParam::		_FixedOrientation = 0;
int	GlobalParam::		_LoweOrigin = 0;
int	GlobalParam::		_ExiftAfterSIFT = 0; //exif after saving result

// parameter changing for better matching with Lowe's SIFT
int		GlobalParam::		_GradientLevelOffset = 1;  //2 in v 289 or older
float	GlobalParam::		_OrientationExtraFactor = 1.0;	//1.5 in v292 or older
float	GlobalParam::		_OrientationGaussianFactor = 0.5; // 1 in v292 or older, (0.5*3.0 = 1.5 in the paper)
///
ClockTimer GlobalUtil::	_globalTimer("start");


#ifdef _DEBUG
void GlobalUtil::CheckErrorsGL(const char* location)
{

	GLuint errnum;
	const char *errstr;
	while (errnum = glGetError()) 
	{
		errstr = (const char *)(gluErrorString(errnum));
		if(errstr) {
			std::cerr << errstr; 
		}
		else {
			std::cerr  << "Error " << errnum;
		}
		
		if(location) std::cerr  << " at " << location;		
		std::cerr  << endl;
	}
	
	return;
}

#endif


void GlobalUtil::SetTextureParameter()
{

	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(_texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//if image need to be up sampled ..use this one

void GlobalUtil::SetTextureParameterUS()
{

	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(_texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
void GlobalUtil::FitViewPort(int width, int height)
{
	int port[4];
	glGetIntegerv(GL_VIEWPORT, port);
	if(port[2] !=width || port[3] !=height)
	{
		glViewport(0, 0, width, height);      
		glMatrixMode(GL_PROJECTION);    
		glLoadIdentity();               
		glOrtho(0, width, 0, height,  0, 1);		
		glMatrixMode(GL_MODELVIEW);     
		glLoadIdentity();  
		_ViewPortChanges++;//not used
	}
}


bool GlobalUtil::CheckFramebufferStatus() {
    GLenum status;
    status=(GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            std::cerr<<("Framebuffer incomplete,incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            std::cerr<<("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            std::cerr<<("Framebuffer incomplete,missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            std::cerr<<("Framebuffer incomplete,attached images must have same dimensions\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
             std::cerr<<("Framebuffer incomplete,attached images must have same format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            std::cerr<<("Framebuffer incomplete,missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            std::cerr<<("Framebuffer incomplete,missing read buffer\n");
            return false;
    }
	return false;
}


void ClockTimer::StartTimer(char *event, int verb)
{
	strcpy(_current_event, event);
	_time_start = clock();
	if(verb && GlobalUtil::_verbose)
	{
		std::cout<<"\n["<<_current_event<<"]:\tbegin ...\n";
	}
}

void ClockTimer::StopTimer(int verb)
{
	_time_stop = clock();
	if(verb && GlobalUtil::_verbose)
	{
		std::cout<<"["<<_current_event<<"]:\t"<<((double)(_time_stop - _time_start)) /CLOCKS_PER_SEC<<"\n";
	}
}

float ClockTimer::GetElapsedTime()
{
	return float(_time_stop - _time_start) / CLOCKS_PER_SEC;
}




void GlobalUtil::CheckFragmentProgramError()
{
	GLint	 UnderNativeLimits;
	glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB,   GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,   &UnderNativeLimits );

	if (UnderNativeLimits!=1)
	{


		GLint aluInstructions, maxAluInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ALU_INSTRUCTIONS_ARB, &aluInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, &maxAluInstructions);
		if (aluInstructions>maxAluInstructions)
			fprintf(stderr, "too many ALU instructions (%d, limit is %d)\n", aluInstructions, maxAluInstructions);

		GLint textureInstructions, maxTextureInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INSTRUCTIONS_ARB, &textureInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB, &maxTextureInstructions);
		if (textureInstructions>maxTextureInstructions)
			fprintf(stderr, "too many texture instructions (%d, limit is %d)\n", textureInstructions, maxTextureInstructions);

		GLint textureIndirections, maxTextureIndirections;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INDIRECTIONS_ARB, &textureIndirections);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB, &maxTextureIndirections);
		if (textureIndirections>maxTextureIndirections)
			fprintf(stderr, "too many texture indirections (%d, limit is %d)\n", textureIndirections, maxTextureIndirections);

		GLint nativeTextureIndirections, maxNativeTextureIndirections;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &nativeTextureIndirections);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &maxNativeTextureIndirections);
		if (nativeTextureIndirections>maxNativeTextureIndirections)
			fprintf(stderr, "too many native texture indirections (%d, limit is %d)\n", nativeTextureIndirections, maxNativeTextureIndirections);

		GLint nativeAluInstructions, maxNativeAluInstructions;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &nativeAluInstructions);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &maxNativeAluInstructions);
		if (nativeAluInstructions>maxNativeAluInstructions)
			fprintf(stderr, "too many native ALU instructions (%d, limit is %d)\n", nativeAluInstructions, maxNativeAluInstructions);
	}
}
