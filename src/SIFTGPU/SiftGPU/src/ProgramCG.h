////////////////////////////////////////////////////////////////////////////
//	File:		ProgramCG.h
//	Author:		Changchang Wu
//	Description :	interface for the ProgramCG classes.
//		ProgramCG:		Cg programs
//		ShaderBagCG:	All Cg shaders for Sift in a bag
//		FilterGLCG:		Cg Gaussian Filters
//		FilterPKCG:		Reserved for packed version, not used currently
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




#ifndef _PROGRAM_CG_H
#define _PROGRAM_CG_H
#include "ProgramGPU.h"
class FragmentProgram;
#include "Cg/cgGL.h"
class ProgramCG:public ProgramGPU  
{
	CGprogram		  _programID; 
	CGparameter		  _texParamID;
	CGprofile		  _profile;
public:
	static CGcontext _Context;
	static CGprofile _FProfile;	
	static CGprofile _VProfile;
	static CGprofile _GProfile;
public:
	void SetTexParameter(unsigned int texID);
	operator CGprogram (){return _programID;}
	CGprogram GetProgramID(){return _programID;}
	int UseProgram();
	static void InitContext();
	static void DestroyContext();
	ProgramCG(char * code, CGprofile profile = ProgramCG::_FProfile);
	ProgramCG();
	virtual ~ProgramCG();

};

class ShaderBagCG:public ShaderBag
{
	CGparameter _param_dog_texu;
	CGparameter	_param_dog_texd;
	CGparameter _param_genlist_start_tex0;
	CGparameter _param_ftex_width;
	CGparameter _param_genlist_step_tex;
	CGparameter _param_genlist_step_tex0;
	CGparameter _param_genvbo_size;
	CGparameter _param_reshapelist_start_width;
	CGparameter _param_reshapelist_step_tex0;
	CGparameter _param_reshapelist_step_d;
	CGparameter _param_reshapelist_end_ftex;
	CGparameter _param_reshapelist_end_otex;
	CGparameter _param_orientation_gtex;
	CGparameter _param_orientation_stex;
	CGparameter _param_orientation_size;
	CGparameter _param_descriptor_gtex;
	CGparameter _param_descriptor_size;
	CGparameter _param_descriptor_dsize;
	CGparameter _param_margin_copy_truncate;
	CGparameter _param_genlist_init_bbox;
public:

	void LoadDescriptorShaderG();
	virtual void LoadDescriptorShader();
	void	LoadDescriptorShaderF4();
	void	LoadDescriptorShaderF8();
	virtual void SetGenListInitParam(int w, int h);
	virtual void SetMarginCopyParam(float xmax, float ymax);
	virtual void SetFeatureOrientationParam(int gtex, int width, int height, float sigma, int stex = 0, float step = 1.0f);
	virtual void SetFeatureDescirptorParam(int gtex, float dwidth, float fwidth, float width, float height, float sigma);
	virtual void SetSimpleOrientationInput(int oTex, float size);
	virtual void SetReshapeListEndParam(int fTex, int oTex );
	virtual void SetReshapeListStepParam(int tex0, int dx, int dy);
	virtual void SetReshapeListStartWidth(float width);
	virtual void LoadListReshapeShaders();
	void LoadOrientationShader();
	virtual void SetGenListStartParam(float width, int tex0);
	ProgramCG* LoadGenListStepShader(int start, int step);
	void LoadGenListShader(int ndoglev,  int nlev);
	ShaderBagCG();
	static void  ErrorCallback();
	virtual void UnloadProgram();
	virtual void SetDogTexParam(int texU, int texD);
	virtual void SetGenListStepParam(int tex, int tex0);
	virtual void SetGenVBOParam( float width, float fwidth,  float size);
	virtual void LoadFixedShaders();
	virtual void LoadDownSampleShader(int scale);
	virtual void LoadUpSampleShader(int scale);
	virtual void LoadDogShader1(float threshold, float edge_threshold);
	virtual void LoadDogShader2(float threshold, float edgeThreshold);
	virtual void LoadKeypointShader(float threshold, float edgeTrheshold);
	virtual void LoadKeypointShaderV2(float threshold, float edgeThreshold);
};


class FilterGLCG : public FilterProgram
{
private:
	ProgramGPU* CreateFilterH(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV(float kernel[], float offset[], int height);
public:

};

class FilterPKCG : public FilterProgram
{
public:
	ProgramGPU* CreateFilterH(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV(float kernel[], float offset[], int height);
	ProgramGPU* CreateFilterH1(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV1(float kernel[], float offset[],int height);
	ProgramGPU* CreateFilterH2(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV2(float kernel[], float offset[],int height);

};
#endif
