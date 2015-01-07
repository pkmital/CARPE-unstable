////////////////////////////////////////////////////////////////////////////
//	File:		GlobalUtil.h
//	Author:		Changchang Wu
//	Description : 
//		GlobalParam:	Global parameters
//		ClockTimer:		Timer 
//		GlobalUtil:		Global Function wrapper
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


#ifndef _GLOBAL_UTILITY_H
#define _GLOBAL_UTILITY_H

//
#ifdef _WIN32
	#define HIGH_RESOLUTION_TIME
	//use timeGetTime instead of clock for timeing
#endif


//wrapper for some shader function
class ProgramGPU;

class GlobalParam
{
public:
	static GLuint	_texTarget;
	static GLuint   _iTexFormat;
	static int		_texMaxDim;
	static int		_texMaxDimGL; 
	static int		_MaxDrawBuffers;
	static int		_MRT8;
	static int		_verbose;
	static int		_verboseS;
	static int		_verboseO;
	static int		_verboseL;
	static int		_verboseT;
	static int		_usePackedTex;
	static int		_MultiTexCoord;
	static int		_IsATI;
	static int		_UseGLSL;
	static int		_EdgeSupression;
	static int		_debug;
	static int		_MaxFilterWidth;
	static float	_FilterWidthFactor;
	static float    _OrientationWindowFactor;
	static float	_DescriptorWindowFactor; 
	static int		_ExtraDownSample;
	static int		_MaxInstruction;
	static int		_MaxOrientation;
	static int		_ListGenGPU;
	static int		_ListReshapeGPU;
	static int		_SupportFP40; //previously named _SupportAdvanced
	static int		_SupportGeometryShader;
	static int		_ComputeDescriptors;
	static int		_ViewPortChanges;
	static float	_MaxFeaturePercent;
	static int		_MaxLevelFeatureNum;
	static int		_DescriptorFormat;
	static int		_DepthTexAllocation;
	static int		_FeatureTexBlock;
	static int		_UseGeometryShader;
	static int		_SubpixelLocalization;
	static int		_UseSiftGPUEX;
	static int		_UseNewDogAlgorithm;
	static int		_DownloadDefault; 
	static int		_ForceTightPyramid;
	static int		_octave_min_default;
	static int		_octave_num_default;
	static int		_InitPyramidWidth;
	static int		_InitPyramidHeight;
	static int		_PreProcessOnCPU;
	static int		_HaveGlSupport;
	static int		_FixedOrientation;
	static int		_LoweOrigin;
	static int		_GradientLevelOffset;
	static int		_ExiftAfterSIFT; 
	//for compatable with old version:
	static float	_OrientationExtraFactor;
	static float	_OrientationGaussianFactor;
};


class ClockTimer
{
private:
	char _current_event[256];
	int  _time_start;
	int  _time_stop;
public:
	ClockTimer(char * event){StartTimer(event);};
	void StopTimer(int verb = 1);
	void StartTimer(char * event, int verb=0);
	float  GetElapsedTime();
};

class GlobalUtil:public GlobalParam
{
    static ClockTimer _globalTimer;                             
public:
	static void CheckFragmentProgramError();
	static void StopTimer()				{	_globalTimer.StopTimer(_verboseS);}
	static void StartTimer(char * event){	_globalTimer.StartTimer(event, _verboseT);	}
	static float GetElapsedTime(){return _globalTimer.GetElapsedTime();}
	static void FitViewPort(int width, int height);
	static void SetTextureParameter();
	static void SetTextureParameterUS();
#ifdef _DEBUG
	static void CheckErrorsGL(const char* location = NULL);
#else
	static void inline CheckErrorsGL(const char* location = NULL){};
#endif
	static bool CheckFramebufferStatus();

};


#endif