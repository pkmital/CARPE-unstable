////////////////////////////////////////////////////////////////////////////
//	File:		ProgramGLSL.h
//	Author:		Changchang Wu
//	Description : Interface for ProgramGLSL classes
//		ProgramGLSL:	Glsl Program
//		FilterGLSL:		Glsl Gaussian Filters
//		FilterPKSL:		Reserved for packed version, not used currently
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


#ifndef _PROGRAM_GLSL_H
#define _PROGRAM_GLSL_H


#include "ProgramGPU.h"

class ProgramGLSL:public ProgramGPU
{
	class ShaderObject
	{
		GLuint			_shaderID;
		int				_type;
		int			_compiled;

		static int ReadShaderFile( char * source,  char *& code);	
		void CheckCompileLog();
	public:
		void PrintCompileLog(ostream & os  );
		int inline IsValidShaderObject(){	return _shaderID && _compiled;}
		int IsValidVertexShader();
		int IsValidFragmentShader();
		GLuint GetShaderID(){return _shaderID;}
		~ShaderObject(); 
		ShaderObject(int shadertype,  char * source, int filesource =0);
	};


protected:
	int			_used;
	int			_linked;
	GLuint		_programID; 
private:
	void AttachShaderObject(ShaderObject& shader);
	void DetachShaderObject(ShaderObject& shader);

public:
	void SetUsed(){_used = 1;}
	void ReLink();
	void SetTexParameter(int texUnit = 0);
	GLuint GetProgramID(){return _programID;}
	int	UseProgram();
	void PrintLinkLog(std::ostream&os);
	int ValidateProgram();
	void CheckLinkLog();
	int LinkProgram();
	operator GLuint (){return _programID;}
public:
	void SetTexParameter(unsigned int texID);

	ProgramGLSL();
	~ProgramGLSL();
	ProgramGLSL(char* frag_source);
	ProgramGLSL(char*frag_source, char * vert_source );
};



class GLTexImage;

class FilterGLSL: public FilterProgram
{
private:

	virtual ProgramGPU* CreateFilterH(float kernel[], float offset[], int width);
	virtual ProgramGPU* CreateFilterV(float kernel[], float offset[], int height);
	ProgramGPU* CreateTex2DFilterH(float kernel[], float offset[], int width);
	ProgramGPU* CreateTex2DFilterV(float kernel[], float offset[], int height);

public:
	static void UnloadProgram();


};



class FilterPKSL:public FilterProgram
{
public:
	ProgramGPU* CreateFilterH(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV(float kernel[], float offset[],int height);
	ProgramGPU* CreateFilterH1(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV1(float kernel[], float offset[],int height);
	ProgramGPU* CreateFilterH2(float kernel[], float offset[], int width);
	ProgramGPU* CreateFilterV2(float kernel[], float offset[],int height);
};


class ShaderBagGLSL:public ShaderBag
{
	GLint _param_dog_texu;
	GLint _param_dog_texd;
	GLint _param_ftex_width;
	GLint _param_genlist_start_tex0;
	GLint _param_genlist_step_tex;
	GLint _param_genlist_step_tex0;
	GLint _param_genvbo_size;
	GLint _param_orientation_gtex;
	GLint _param_orientation_size;
	GLint _param_margin_copy_truncate;
	GLint _param_genlist_init_bbox;
public:
	virtual void SetMarginCopyParam(float xmax, float ymax);
	void SetSimpleOrientationInput(int oTex, float size);
	void LoadOrientationShader();
	ProgramGLSL* LoadGenListStepShader(int start, int step);
	virtual void SetGenListInitParam(int w, int h);
	virtual void SetGenListStartParam(float width, int tex0);
	virtual void LoadGenListShader(int ndoglev, int nlev);
	virtual void UnloadProgram();
	virtual void LoadDownSampleShader(int scale);
	virtual void LoadUpSampleShader(int scale);
	virtual void LoadDogShader1(float threshold, float edge_threshold);
	virtual void LoadDogShader2(float threshold, float edgeThreshold);
	virtual void LoadKeypointShader(float threshold, float edgeTrheshold){};
	virtual void LoadFixedShaders();
	virtual void SetDogTexParam(int texU, int texD);
	virtual void SetGenListStepParam(int tex, int tex0);
	virtual void SetGenVBOParam( float width, float fwidth, float size);
};

class ShaderBagPKSL:public ShaderBagGLSL
{
public:
	void LoadFixedShader();
	void LoadDownSampleShader(int scale);
	void LoadUpSampleShader(int scale);
};

#endif