//////////////////////////////////////////////////////////////////////////////
//	File:		ProgramCG.cpp
//	Author:		Changchang Wu
//	Description :	implementation of cg related class.
//		class ProgramCG			A simple wrapper of Cg programs
//		class ShaderBagCG		cg shaders for SIFT
//		class FilterCGGL		cg gaussian filters for SIFT
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
#include <iomanip>
#include <vector>
#include <strstream>
using namespace std;
#include "GlobalUtil.h"
#include "ProgramCG.h"
#include "GLTexImage.h"
#include "ShaderMan.h"
#include "FrameBufferObject.h"
#include "math.h"

#ifdef _WIN32
	#include "imdebug/imdebuggl.h"
	#pragma comment (lib, "../lib/imdebug.lib")
	#pragma comment (lib, "../lib/cg.lib")
	#pragma comment (lib, "../lib/cggl.lib")
#endif

CGcontext	ProgramCG::_Context	=0;
CGprofile	ProgramCG::_FProfile;
CGprofile	ProgramCG::_VProfile;
CGprofile	ProgramCG::_GProfile;

//ALTERNATIVE CG.v.s. GLSL
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ProgramCG::ProgramCG()
{
	_programID = NULL;
}

ProgramCG::~ProgramCG()
{
	if(_programID) cgDestroyProgram(_programID);
}

ProgramCG::ProgramCG(char *code, CGprofile profile)
{
	_profile = profile;
	int epos;
	const char* ati_args[] = {"-po", "ATI_draw_buffers",0}; 
	const char* fp40_args[] = {"-ifcvt", "none","-unroll", "all", 0};
	_programID = ::cgCreateProgram(_Context, CG_SOURCE, code, profile, NULL, 
		GlobalUtil::_IsATI? ati_args : (GlobalUtil::_SupportFP40? fp40_args:NULL));
	if(_programID)
	{
		cgGLLoadProgram(_programID );
		_texParamID = cgGetNamedParameter(_programID, "tex");


		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &epos);
		if(epos >=0)
		{
			std::cout<<cgGetProgramString(_programID, CG_COMPILED_PROGRAM)<<endl;
			std::cerr<<glGetString(GL_PROGRAM_ERROR_STRING_ARB)<<endl;
		}
	}else
	{
		std::cerr<<code<<endl;
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &epos);
		if(epos >=0)
		{
			std::cout<<cgGetProgramString(_programID, CG_COMPILED_PROGRAM)<<endl;
			std::cerr<<glGetString(GL_PROGRAM_ERROR_STRING_ARB)<<endl;
		}else
		{
			std::cout<<glGetString(GL_PROGRAM_ERROR_STRING_ARB)<<endl;
		}
	}

}
void ProgramCG::InitContext()
{
	if(_Context == 0)
	{
		_Context	= cgCreateContext();
 
		_VProfile =  cgGLGetLatestProfile(CG_GL_VERTEX);
		cgGLSetOptimalOptions(_VProfile);
		
		/////////////
		_FProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
		cgGLSetOptimalOptions(_FProfile);

		//_GProfile = cgGLGetLatestProfile(CG_GL_GEOMETRY);
		//just assume CG_GL_GEOMETRY = CG_GL_FRAGMENT + 1
		//for compatible with old version of cg

		if(GlobalUtil::_UseGeometryShader)
		{
			_GProfile = cgGLGetLatestProfile(CGGLenum(CG_GL_FRAGMENT+1));
			if(_GProfile!=CG_PROFILE_UNKNOWN)
			{
				GlobalUtil::_SupportGeometryShader =1;
				cgGLSetOptimalOptions(_GProfile);
			}else
			{
				std::cerr<<"Your driver does not support Geometry Shader"; 
			}
		}


		const char * profilename = cgGetProfileString(_FProfile);
		if(
			strcmp(profilename, "fp40")==0 ||
			strcmp(profilename, "gp4fp")==0	||
			strcmp(profilename, "gp5fp")==0		)GlobalUtil::_SupportFP40 = 1;
		
		std::cout<<"Shader Profile: "<<cgGetProfileString(_VProfile)<<",\t"<<cgGetProfileString(_FProfile)
			<<",\t"<<(GlobalUtil::_UseGeometryShader && _GProfile!=CG_PROFILE_UNKNOWN? cgGetProfileString(_GProfile): " ")<<endl;
	}
}

void ProgramCG::DestroyContext()
{
	cgDestroyContext(_Context);
}

ShaderBagCG::ShaderBagCG()
{
	cgSetErrorCallback(ErrorCallback);
	ProgramCG::InitContext();

}


int ProgramCG::UseProgram()
{
	if(_programID)
	{
		cgGLEnableProfile(_profile);
		cgGLBindProgram(_programID);

		return 1;
	}else
	{
		return 0;
	}
}

void ShaderBagCG::UnloadProgram()
{

	cgGLUnbindProgram(ProgramCG::_FProfile);
	cgGLDisableProfile(ProgramCG::_FProfile);
	cgGLUnbindProgram(ProgramCG::_VProfile);
	cgGLDisableProfile(ProgramCG::_VProfile);
	if(GlobalUtil::_UseGeometryShader && ProgramCG::_GProfile!=CG_PROFILE_UNKNOWN)
	{
		cgGLUnbindProgram(ProgramCG::_GProfile);
		cgGLDisableProfile(ProgramCG::_GProfile);
	}

}

void ShaderBagCG::ErrorCallback()
{
	CGerror err = cgGetError();
	if(err)
	{
		std::cerr<< cgGetErrorString(err)<<endl;
	}
}
void ShaderBagCG::LoadFixedShaders()
{

	if(GlobalUtil::_SupportGeometryShader && GlobalUtil::_UseGeometryShader)
	{
		//debug geometry shader now
		s_debug = new ProgramCG( "POINT POINT_OUT void main(AttribArray<float4> point: POSITION )\n\
		{float4 pos = point[0];\n\
		for(float i = 0.0; i < 32.0; i+=1.0){\n\
		emitVertex(pos:POSITION, float4(0.0, 1.0, 0.0, 1.0): COLOR0);\n\
		pos.x+=1.0;}}", 
			ProgramCG::_GProfile);

	}

	char * display_gaussian_code=
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float r = texRECT(tex, TexCoord0.xy).r;\n\
	FragColor = float4(r, r, r, 1.0);}";

	s_display_gaussian =  new ProgramCG( display_gaussian_code);





	char *rgb2gray_code =
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float intensity = dot(float3(0.299, 0.587, 0.114), texRECT(tex,TexCoord0.xy ).rgb);\n\
	FragColor= float4(intensity, intensity, intensity, 1.0);}";//

	s_gray = new ProgramCG( rgb2gray_code);


	char* copy_rg_code=
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);	FragColor = float4(cc.rg, 0.0, 0.0);	}";
	s_downsampling = new ProgramCG(copy_rg_code);



	s_copy_key = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	FragColor.rg= texRECT(tex, TexCoord0.xy).rg; FragColor.ba = float2(0,1);	}");
	
	s_texcoord = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0){FragColor = TexCoord0;	}");
	

		//setting the depth
	s_dog_zpass = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragDepth : DEPTH, uniform samplerRECT tex){\n\
	FragDepth = texRECT(tex, TexCoord0.xy).r;}");

	s_display_dog =  new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float g = (0.5+5.0*texRECT(tex, TexCoord0.xy).g);\n\
	FragColor = float4(g, g, g, 1.0);}" );


	s_display_gradx = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	float gx = 0.5+5.0*cc.b * (cos(cc.a));\n\
	FragColor = float4(gx, gx, gx, 1.0); }");


	s_display_grady = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	float gy = 5.0*cc.b * (sin(cc.a))+0.5;\n\
	FragColor = float4(gy, gy, gy, 1.0); }");


	s_display_abs_dog =  new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float g = 10.0*abs(texRECT(tex, TexCoord0.xy).g);\n\
	FragColor = float4(g, g, g, 1.0);}" );


	s_display_abs_gradx = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	float gx = 10.0*abs(cc.b * (cos(cc.a)));\n\
	FragColor = float4(gx, gx, gx, 1.0); }");


	s_display_abs_grady = new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	float gy = 10.0*abs(cc.b * (sin(cc.a)));\n\
	FragColor = float4(gy, gy, gy, 1.0); }");

	s_display_keys= new ProgramCG(
	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	if(cc.r ==1.0) FragColor = float4(1.0, 0, 0,1.0); \n\
	else {if (cc.r ==0.5) FragColor = float4(0.0,1.0,0.0,1.0);	else discard;}}");		

/*

	"void main(float4 TexCoord0 : TEXCOORD0, out float4 FragColor : COLOR0, uniform samplerRECT tex){\n\
	float4 cc = texRECT(tex, TexCoord0.xy);\n\
	if(cc.r > 0.0) FragColor = float4(cc.r, 0, 0, 1.0);	else discard;}");
*/

	//shader used to write a vertex buffer object
	//which is used to draw the quads of each feature
	//feature orientation is not supported yet
	ProgramCG * program;
	s_vertex_list = program = new ProgramCG(
	"void main(in float4 TexCoord0: TEXCOORD0,\n\
	uniform float4 sizes, \n\
	uniform samplerRECT tex, \n\
	out float4 FragColor: COLOR0){\n\
	float fwidth = sizes.y; \n\
	float twidth = sizes.z; \n\
	float rwidth = sizes.w; \n\
	float size = sizes.x; \n\
	float index = 0.1*(fwidth*floor(TexCoord0.y) + TexCoord0.x);\n\
	float px = fmod(index, twidth);\n\
	float2 tpos= floor(float2(px, index*rwidth))+0.5;\n\
	float4 cc = texRECT(tex, tpos );\n\
	FragColor.zw = float2(0.0, 1.0);\n\
	if(any(cc.xy <=0)) {FragColor.xy = cc.xy;}else \n\
	{\n\
		float type = frac(px);\n\
		float2 dxy; float s, c;\n\
		dxy.x = type < 0.1 ? 0 : ((type <0.5 || type > 0.9)? size : -size);\n\
		dxy.y = type < 0.2 ? 0 : ((type < 0.3 || type > 0.7 )? -size :size); \n\
		sincos(cc.b, s, c);\n\
		FragColor.x = cc.x + c*dxy.x-s*dxy.y;\n\
		FragColor.y = cc.y + c*dxy.y+s*dxy.x;}\n\
	}\n\0");
	/*FragColor = float4(tpos, 0.0, 1.0);}\n\0");*/

	_param_genvbo_size = cgGetNamedParameter(*program, "sizes");

	s_margin_copy = program = new ProgramCG(
	"void main(float4 texCoord0: TEXCOORD0, out float4 FragColor: COLOR0, \n\
	uniform samplerRECT tex, uniform float2 truncate){\n\
	FragColor = texRECT(tex, min(texCoord0.xy, truncate)); }");

	_param_margin_copy_truncate = cgGetNamedParameter(*program, "truncate");

	if(GlobalUtil::_SupportFP40)
	{
		LoadOrientationShader();
		LoadListReshapeShaders();

		if(GlobalUtil::_SupportGeometryShader ==0) GlobalUtil::_UseGeometryShader = 0;

		if(GlobalUtil::_UseGeometryShader)
			GlobalUtil::_DescriptorFormat = 32;
		else if(GlobalUtil::_MRT8 && GlobalUtil::_MaxDrawBuffers==8) 
			GlobalUtil::_DescriptorFormat = 4;
		else if(GlobalUtil::_MaxDrawBuffers >=4 ) 
			GlobalUtil::_DescriptorFormat = 8;
		else
			GlobalUtil::_ComputeDescriptors = 0;

		if(GlobalUtil::_ComputeDescriptors)		LoadDescriptorShader();
	}else
	{
		s_orientation = program =  new ProgramCG(
		"void main(out float4 FragColor : COLOR0, \n\
		uniform samplerRECT fTex, uniform samplerRECT oTex, \n\
		uniform float size, \n\
		in float2 tpos : TEXCOORD0){\n\
		float4 cc = texRECT(fTex, tpos);\n\
		float4 oo = texRECT(oTex, cc.rg);\n\
		FragColor = float4(cc.rg, oo.a, size);}");  
		_param_orientation_gtex= cgGetNamedParameter(*program, "oTex");
		_param_orientation_size= cgGetNamedParameter(*program, "size");
	}

}

void ShaderBagCG::LoadDownSampleShader(int scale)
{


}

void ShaderBagCG::LoadUpSampleShader(int scale)
{
	
}
void ShaderBagCG::SetMarginCopyParam(float xmax, float ymax)
{
	float truncate[2] = {xmax, ymax};
	cgGLSetParameter2fv(_param_margin_copy_truncate, truncate);
}
void ShaderBagCG::LoadDogShader2(float threshold, float edgeThreshold)
{
	ProgramCG*  program;
	//in pass two...
	char buffer[10240];
	ostrstream out(buffer, 10240);

	//check between levels.
	
	out<<"void main(\n\
	float4  TexCoord0 : TEXCOORD0,\n\
	out float4 FragColor : COLOR0, uniform samplerRECT tex, uniform samplerRECT texU, uniform samplerRECT texD){\n\
	float4 cc = texRECT(tex , TexCoord0.xy);\n\
	float4 uc = texRECT(texU, TexCoord0.xy);\n\
	float4 dc = texRECT(texD, TexCoord0.xy);\n\
	cc.r = cc.r == 1.0 && any(cc.gg < float2(uc.b, dc.b)) ? 0.0 : cc.r ; \n\
	cc.r = cc.r == 0.5 && any(cc.gg > float2(uc.a, dc.a)) ? 0.0 : cc.r ; \n\
	FragColor = cc; \n";
	out<<"\n}\n"<<'\0';


/*  branching version..
	if(cc.r ==1.0) {if(any(cc.gg <= float2(uc.b, dc.b)))cc.r =0;}\n\
	else if(any(cc.gg>=float2(uc.a,dc.a)))cc.r=0;\n\
*/


	s_dog_pass2  = program = new ProgramCG(buffer);	
	//parameter
	_param_dog_texu = cgGetNamedParameter(*program, "texU");
	_param_dog_texd = cgGetNamedParameter(*program, "texD");
}

void ShaderBagCG::LoadKeypointShaderV2(float threshold, float edge_threshold)
{
	char buffer[10240];
	float threshold1 = threshold;
	float threshold2 = (edge_threshold+1)*(edge_threshold+1)/edge_threshold;
	ostrstream out(buffer, 10240);
	out<<
	"void main (\
	float4 TexCC : TEXCOORD0, float4 TexLC : TEXCOORD1,\n\
	float4 TexRC : TEXCOORD2, float4 TexCD : TEXCOORD3, \n\
	float4 TexCU : TEXCOORD4, float4 TexLD : TEXCOORD5, \n\
	float4 TexLU : TEXCOORD6, float4 TexRD : TEXCOORD7,\n\
	out float4 FragData0 : COLOR0, out float4 FragData1 : COLOR1, \n\
	uniform samplerRECT tex, uniform samplerRECT texU, uniform samplerRECT texD)\n\
	{\n\
		float4 v1, v2, gg;\n\
		float2 TexRU = float2(TexRC.x, TexCU.y); \n\
		float4 cc  = texRECT(tex, TexCC.xy);\n\
		v1.x = texRECT(tex, TexLC.xy).g;\n\
		gg.x = texRECT(tex, TexLC.xy).r;\n\
		v1.y = texRECT(tex, TexRC.xy).g;\n\
		gg.y = texRECT(tex, TexRC.xy).r;\n\
		v1.z = texRECT(tex, TexCD.xy).g;\n\
		gg.z = texRECT(tex, TexCD.xy).r;\n\
		v1.w = texRECT(tex, TexCU.xy).g;\n\
		gg.w = texRECT(tex, TexCU.xy).r;\n\
		v2.x = texRECT(tex, TexLD.xy).g;\n\
		v2.y = texRECT(tex, TexLU.xy).g;\n\
		v2.z = texRECT(tex, TexRD.xy).g;\n\
		v2.w = texRECT(tex, TexRU.xy).g;\n\
		float2 dxdy = 0.5*(gg.yw - gg.xz); \n\
		float grad = length(dxdy);\n\
		float theta = grad==0? 0: atan2(dxdy.y, dxdy.x);\n\
		FragData0 = float4(cc.rg, grad, theta);\n"
	<<"\
		float dog = 0.0; \n\
		FragData1 = float4(0, 0, 0, 0); \n\
		float2 v3; float4 v4, v5, v6;\n"
	<<"\
		if( cc.g > "<<threshold1 * 0.8 <<" && all(cc.gggg > max(v1, v2)))\n\
		{\n\
			v3.x = texRECT(texU, TexCC.xy).g;\n\
			v4.x = texRECT(texU, TexLC.xy).g;\n\
			v4.y = texRECT(texU, TexRC.xy).g;\n\
			v4.z = texRECT(texU, TexCD.xy).g;\n\
			v4.w = texRECT(texU, TexCU.xy).g;\n\
			v6.x = texRECT(texU, TexLD.xy).g;\n\
			v6.y = texRECT(texU, TexLU.xy).g;\n\
			v6.z = texRECT(texU, TexRD.xy).g;\n\
			v6.w = texRECT(texU, TexRU.xy).g;\n\
			if(cc.g < v3.x || any(cc.gggg<v4.xyzw) ||any(cc.gggg<v6.xyzw))return; \n\
			v3.y = texRECT(texD, TexCC.xy).g;\n\
			v5.x = texRECT(texD, TexLC.xy).g;\n\
			v5.y = texRECT(texD, TexRC.xy).g;\n\
			v5.z = texRECT(texD, TexCD.xy).g;\n\
			v5.w = texRECT(texD, TexCU.xy).g;\n\
			v6.x = texRECT(texD, TexLD.xy).g;\n\
			v6.y = texRECT(texD, TexLU.xy).g;\n\
			v6.z = texRECT(texD, TexRD.xy).g;\n\
			v6.w = texRECT(texD, TexRU.xy).g;\n\
			if(cc.g < v3.y || any(cc.gggg<v5.xyzw) ||any(cc.gggg<v6.xyzw))return; \n\
			dog = 1.0; \n\
		}\n"
	//the minimum case
	<<"\
	  else if(cc.g < "<<-threshold1 * 0.8<<" && all(cc.gggg < min(v1, v2)))\n\
	  {\n\
			v3.x = texRECT(texU, TexCC.xy).g;\n\
			v4.x = texRECT(texU, TexLC.xy).g;\n\
			v4.y = texRECT(texU, TexRC.xy).g;\n\
			v4.z = texRECT(texU, TexCD.xy).g;\n\
			v4.w = texRECT(texU, TexCU.xy).g;\n\
			v6.x = texRECT(texU, TexLD.xy).g;\n\
			v6.y = texRECT(texU, TexLU.xy).g;\n\
			v6.z = texRECT(texU, TexRD.xy).g;\n\
			v6.w = texRECT(texU, TexRU.xy).g;\n\
			if(cc.g > v3.x || any(cc.gggg>v4.xyzw) ||any(cc.gggg>v6.xyzw))return; \n\
			v3.y = texRECT(texD, TexCC.xy).g;\n\
			v5.x = texRECT(texD, TexLC.xy).g;\n\
			v5.y = texRECT(texD, TexRC.xy).g;\n\
			v5.z = texRECT(texD, TexCD.xy).g;\n\
			v5.w = texRECT(texD, TexCU.xy).g;\n\
			v6.x = texRECT(texD, TexLD.xy).g;\n\
			v6.y = texRECT(texD, TexLU.xy).g;\n\
			v6.z = texRECT(texD, TexRD.xy).g;\n\
			v6.w = texRECT(texD, TexRU.xy).g;\n\
			if(cc.g > v3.y || any(cc.gggg>v5.xyzw) ||any(cc.gggg>v6.xyzw))return; \n\
			dog = 0.5 ; \n\
		}\n\
		else\n\
			return;\n"
	<<"\
	  int i = 0; \n\
	  float2 offset = float2(0, 0);\n\
	  float2 offsets = float2(0, 0);\n\
	  float3 dxys;					\n\
	  float fx, fy, fs; \n\
	  float fxx, fyy, fxy; \n\
	  float fxs, fys, fss; \n\
	  do\n\
	  {\n\
		dxys = float3(0, 0, 0);\n\
		offset = float2(0, 0);\n\
		float4 D2 = v1.xyzw - cc.gggg;\n\
		fxx = D2.x + D2.y;\n\
		fyy = D2.z + D2.w;\n\
		float2 D4 = v2.xw - v2.yz;\n\
		fxy = 0.25*(D4.x + D4.y);\n\
		float2 D5 = 0.5*(v1.yw-v1.xz); \n\
		fx = D5.x;\n\
		fy = D5.y ; \n\
		fs = 0.5*( v3.x - v3.y ); \n\
		fss = v3.x + v3.y - cc.g - cc.g;\n\
		fxs = 0.25 * ( v4.y + v5.x - v4.x - v5.y);\n\
		fys = 0.25 * ( v4.w + v5.z - v4.z - v5.w);\n\
		float4 A0, A1, A2 ;			\n\
		A0 = float4(fxx, fxy, fxs, -fx);	\n\
		A1 = float4(fxy, fyy, fys, -fy);	\n\
		A2 = float4(fxs, fys, fss, -fs);	\n\
		float3 x3 = abs(float3(fxx, fxy, fxs));		\n\
		float maxa = max(max(x3.x, x3.y), x3.z);	\n\
		if(maxa > 1e-10 )							\n\
		{\n\
			if(x3.y ==maxa )							\n\
			{											\n\
				float4 TEMP = A1; A1 = A0; A0 = TEMP;	\n\
			}else if( x3.z == maxa )					\n\
			{											\n\
				float4 TEMP = A2; A2 = A0; A0 = TEMP;	\n\
			}											\n\
			A0 /= A0.x;									\n\
			A1 -= A1.x * A0;							\n\
			A2 -= A2.x * A0;							\n\
			float2 x2 = abs(float2(A1.y, A2.y));		\n\
			if( x2.y > x2.x )							\n\
			{											\n\
				float3 TEMP = A2.yzw;					\n\
				A2.yzw = A1.yzw;						\n\
				A1.yzw = TEMP;							\n\
				x2.x = x2.y;							\n\
			}											\n\
			if(x2.x > 1e-10)							\n\
			{\n\
				A1.yzw /= A1.y;							\n\
				A2.yzw -= A2.y * A1.yzw;				\n\
				if(abs(A2.z) > 1e-10)					\n\
				{\n"
	// compute dx, dy, ds: 
	<<"\
					dxys.z = A2.w /A2.z;						\n\
					dxys.y = A1.w - dxys.z*A1.z;			    \n\
					dxys.x = A0.w - dxys.z*A0.z - dxys.y*A0.y;	\n\
				}\n\
			}\n\
		}\n\
		offset.x = dxys.x > 0.6 ? 1 : 0 + dxys.x < -0.6 ? -1 : 0;\n\
		offset.y = dxys.y > 0.6 ? 1 : 0 + dxys.y < - 0.6? -1 : 0;\n\
		if(any(offset>0))\n\
		{\n\
			offsets += offset; \n\
		    cc  =  texRECT(tex, TexCC.xy  + offsets);\n\
			v1.x = texRECT(tex , TexLC.xy + offsets).g;\n\
			v1.y = texRECT(tex , TexRC.xy + offsets).g;\n\
			v1.z = texRECT(tex , TexCD.xy + offsets).g;\n\
			v1.w = texRECT(tex , TexCU.xy + offsets).g;\n\
			v2.x = texRECT(tex , TexLD.xy + offsets).g;\n\
			v2.y = texRECT(tex , TexLU.xy + offsets).g;\n\
			v2.z = texRECT(tex , TexRD.xy + offsets).g;\n\
			v2.w = texRECT(tex , TexRU.xy + offsets).g;\n\
			v3.x = texRECT(texU, TexCC.xy + offsets).g;\n\
			v4.x = texRECT(texU, TexLC.xy + offsets).g;\n\
			v4.y = texRECT(texU, TexRC.xy + offsets).g;\n\
			v4.z = texRECT(texU, TexCD.xy + offsets).g;\n\
			v4.w = texRECT(texU, TexCU.xy + offsets).g;\n\
			v3.y = texRECT(texD, TexCC.xy + offsets).g;\n\
			v5.x = texRECT(texD, TexLC.xy + offsets).g;\n\
			v5.y = texRECT(texD, TexRC.xy + offsets).g;\n\
			v5.z = texRECT(texD, TexCD.xy + offsets).g;\n\
			v5.w = texRECT(texD, TexCU.xy + offsets).g;\n\
		}\n\
	  }while(i < 5 && any(abs(offset)>0));\n"
	  <<"\
	  bool test1 = (abs(cc.g + 0.5*dot(float3(fy, fy, fs), dxys ))>"<<threshold1<<") ;\n\
	  float test2_v1= fxx*fyy - fxy *fxy; \n\
	  float test2_v2 = (fxx+fyy); \n\
	  test2_v2 = test2_v2*test2_v2;\n\
	  bool test2 = test2_v1>0 && test2_v2 < "<<threshold2<<"*test2_v1; \n "
    //keep the point when the offset is less than 1
	<<"\
	  FragData1 = test1 && test2 && all( abs(dxys) < 1.5)? float4( dog, dxys.xy+offsets, dxys.z) : float4(0, 0, 0, 0); \n\
	}\n"	
	<<'\0';
	ProgramCG * program; 
	s_keypoint = program = new ProgramCG(buffer);
	//parameter
	_param_dog_texu = cgGetNamedParameter(*program, "texU");
	_param_dog_texd = cgGetNamedParameter(*program, "texD");

	s_grad_pass = program = new ProgramCG(
	"void main (\
	float4 TexCC : TEXCOORD0, float4 TexLC : TEXCOORD1,\n\
	float4 TexRC : TEXCOORD2, float4 TexCD : TEXCOORD3, float4 TexCU : TEXCOORD4,\n\
	out float4 FragData0 : COLOR0, uniform samplerRECT tex)\n\
	{\n\
		float4 v1, v2, gg;\n\
		float4 cc  = texRECT(tex, TexCC.xy);\n\
		gg.x = texRECT(tex, TexLC.xy).r;\n\
		gg.y = texRECT(tex, TexRC.xy).r;\n\
		gg.z = texRECT(tex, TexCD.xy).r;\n\
		gg.w = texRECT(tex, TexCU.xy).r;\n\
		float2 dxdy = 0.5*(gg.yw - gg.xz); \n\
		float grad = length(dxdy);\n\
		float theta = grad==0? 0: atan2(dxdy.y, dxdy.x);\n\
		FragData0 = float4(cc.rg, grad, theta);\n\
	}\n\0");
}


//keypoint detection shader
//1. compare with 26 neighbours
//2. sub-pixel sub-scale localization
//3. output: [dog, offset(x,y,s)]

void ShaderBagCG:: LoadKeypointShader(float threshold, float edge_threshold)
{
	char buffer[10240];
	float threshold1 = threshold;
	float threshold2 = (edge_threshold+1)*(edge_threshold+1)/edge_threshold;
	ostrstream out(buffer, 10240);

	//tex(X)(Y)
	//X: (CLR) (CENTER 0, LEFT -1, RIGHT +1)  
	//Y: (CDU) (CENTER 0, DOWN -1, UP    +1) 

	out<<
	"void main (\
	float4 TexCC : TEXCOORD0, float4 TexLC : TEXCOORD1,\n\
	float4 TexRC : TEXCOORD2, float4 TexCD : TEXCOORD3, \n\
	float4 TexCU : TEXCOORD4, float4 TexLD : TEXCOORD5, \n\
	float4 TexLU : TEXCOORD6, float4 TexRD : TEXCOORD7,\n\
	out float4 FragData0 : COLOR0, out float4 FragData1 : COLOR1, \n\
	uniform samplerRECT tex, uniform samplerRECT texU, uniform samplerRECT texD)\n\
	{\n\
		float4 v1, v2, gg;\n\
		float2 TexRU = float2(TexRC.x, TexCU.y); \n\
		float4 cc  = texRECT(tex, TexCC.xy);\n\
		v1.x = texRECT(tex, TexLC.xy).g;\n\
		gg.x = texRECT(tex, TexLC.xy).r;\n\
		v1.y = texRECT(tex, TexRC.xy).g;\n\
		gg.y = texRECT(tex, TexRC.xy).r;\n\
		v1.z = texRECT(tex, TexCD.xy).g;\n\
		gg.z = texRECT(tex, TexCD.xy).r;\n\
		v1.w = texRECT(tex, TexCU.xy).g;\n\
		gg.w = texRECT(tex, TexCU.xy).r;\n\
		v2.x = texRECT(tex, TexLD.xy).g;\n\
		v2.y = texRECT(tex, TexLU.xy).g;\n\
		v2.z = texRECT(tex, TexRD.xy).g;\n\
		v2.w = texRECT(tex, TexRU.xy).g;\n\
		float2 dxdy = 0.5*(gg.yw - gg.xz); \n\
		float grad = length(dxdy);\n\
		float theta = grad==0? 0: atan2(dxdy.y, dxdy.x);\n\
		FragData0 = float4(cc.rg, grad, theta);\n"


	//test against 8 neighbours
	//use variable to identify type of extremum
	//1.0 for local maximum and 0.5 for minimum
	<<"\
		float dog = 0.0; \n\
		FragData1 = float4(0, 0, 0, 0); \n\
		dog = cc.g > "<<threshold1* (GlobalUtil::_SubpixelLocalization?0.8:1.0) <<" && all(cc.gggg > max(v1, v2))?1.0: 0.0;\n\
		dog = cc.g < "<<-threshold1*(GlobalUtil::_SubpixelLocalization?0.8:1.0)<<" && all(cc.gggg < min(v1, v2))?0.5: dog;\n\
		if(dog == 0.0) return;\n"

	//do edge supression first.. 
	//vector v1 is < (-1, 0), (1, 0), (0,-1), (0, 1)>
	//vector v2 is < (-1,-1), (-1,1), (1,-1), (1, 1)>

	<<"\
		float fxx, fyy, fxy; \n\
		float4 D2 = v1.xyzw - cc.gggg;\n\
		float2 D4 = v2.xw - v2.yz;\n\
		fxx = D2.x + D2.y;\n\
		fyy = D2.z + D2.w;\n\
		fxy = 0.25*(D4.x + D4.y);\n\
		float fxx_plus_fyy = fxx + fyy;\n\
		float score_up = fxx_plus_fyy*fxx_plus_fyy; \n\
		float score_down = (fxx*fyy - fxy*fxy);\n\
		if( score_down <= 0 || score_up > "<<threshold2<<" * score_down)return;\n"
	//...
	<<" \
		float2 D5 = 0.5*(v1.yw-v1.xz); \n\
		float fx = D5.x, fy = D5.y ; \n\
		float fs, fss , fxs, fys ; \n\
		float2 v3; float4 v4, v5, v6;\n"
	//read 9 pixels of upper level
	<<"\
		v3.x = texRECT(texU, TexCC.xy).g;\n\
		v4.x = texRECT(texU, TexLC.xy).g;\n\
		v4.y = texRECT(texU, TexRC.xy).g;\n\
		v4.z = texRECT(texU, TexCD.xy).g;\n\
		v4.w = texRECT(texU, TexCU.xy).g;\n\
		v6.x = texRECT(texU, TexLD.xy).g;\n\
		v6.y = texRECT(texU, TexLU.xy).g;\n\
		v6.z = texRECT(texU, TexRD.xy).g;\n\
		v6.w = texRECT(texU, TexRU.xy).g;\n"
	//compare with 9 pixels of upper level
	//read and compare with 9 pixels of lower level
	//the maximum case
	<<"\
		if(dog == 1.0)\n\
		{\n\
			if(cc.g < v3.x || any(cc.gggg<v4.xyzw) ||any(cc.gggg<v6.xyzw))return; \n\
			v3.y = texRECT(texD, TexCC.xy).g;\n\
			v5.x = texRECT(texD, TexLC.xy).g;\n\
			v5.y = texRECT(texD, TexRC.xy).g;\n\
			v5.z = texRECT(texD, TexCD.xy).g;\n\
			v5.w = texRECT(texD, TexCU.xy).g;\n\
			v6.x = texRECT(texD, TexLD.xy).g;\n\
			v6.y = texRECT(texD, TexLU.xy).g;\n\
			v6.z = texRECT(texD, TexRD.xy).g;\n\
			v6.w = texRECT(texD, TexRU.xy).g;\n\
			if(cc.g < v3.y || any(cc.gggg<v5.xyzw) ||any(cc.gggg<v6.xyzw))return; \n\
		}\n"
	//the minimum case
	<<"\
		else{\n\
			if(cc.g > v3.x || any(cc.gggg>v4.xyzw) ||any(cc.gggg>v6.xyzw))return; \n\
			v3.y = texRECT(texD, TexCC.xy).g;\n\
			v5.x = texRECT(texD, TexLC.xy).g;\n\
			v5.y = texRECT(texD, TexRC.xy).g;\n\
			v5.z = texRECT(texD, TexCD.xy).g;\n\
			v5.w = texRECT(texD, TexCU.xy).g;\n\
			v6.x = texRECT(texD, TexLD.xy).g;\n\
			v6.y = texRECT(texD, TexLU.xy).g;\n\
			v6.z = texRECT(texD, TexRD.xy).g;\n\
			v6.w = texRECT(texD, TexRU.xy).g;\n\
			if(cc.g > v3.y || any(cc.gggg>v5.xyzw) ||any(cc.gggg>v6.xyzw))return; \n\
		}\n";

	if(GlobalUtil::_SubpixelLocalization)

	// sub-pixel localization FragData1 = float4(dog, 0, 0, 0); return;
	out <<" \
		fs = 0.5*( v3.x - v3.y ); //bug fix 9/12/2007 \n\
		fss = v3.x + v3.y - cc.g - cc.g;\n\
		fxs = 0.25 * ( v4.y + v5.x - v4.x - v5.y);\n\
		fys = 0.25 * ( v4.w + v5.z - v4.z - v5.w);\n"
	
	// 
	// let dog difference be quatratic function  of dx, dy, ds; 
	// df(dx, dy, ds) = fx * dx + fy*dy + fs * ds + 
	//				  + 0.5 * ( fxx * dx * dx + fyy * dy * dy + fss * ds * ds)
	//				  + (fxy * dx * dy + fxs * dx * ds + fys * dy * ds)
	// (fx, fy, fs, fxx, fyy, fss, fxy, fxs, fys are the derivatives)
	
	//the local extremum satisfies
	// df/dx = 0, df/dy = 0, df/dz = 0
	
	//that is 
	// |-fx|     | fxx fxy fxs |   |dx|
	// |-fy|  =  | fxy fyy fys | * |dy|
	// |-fs|     | fxs fys fss |   |ds|
	// need to solve dx, dy, ds

	// Use Gauss elimination to solve the linear system
    <<"\
		FragData1 = float4(dog, 0, 0, 0);	\n\
		float4 A0, A1, A2 ;			\n\
		A0 = float4(fxx, fxy, fxs, -fx);	\n\
		A1 = float4(fxy, fyy, fys, -fy);	\n\
		A2 = float4(fxs, fys, fss, -fs);	\n\
		float3 x3 = abs(float3(fxx, fxy, fxs));		\n\
		float maxa = max(max(x3.x, x3.y), x3.z);	\n\
		if(maxa < 1e-10 ) return;					\n\
		if(x3.y ==maxa )							\n\
		{											\n\
			float4 TEMP = A1; A1 = A0; A0 = TEMP;	\n\
		}else if( x3.z == maxa )					\n\
		{											\n\
			float4 TEMP = A2; A2 = A0; A0 = TEMP;	\n\
		}											\n\
		A0 /= A0.x;									\n\
		A1 -= A1.x * A0;							\n\
		A2 -= A2.x * A0;							\n\
		float2 x2 = abs(float2(A1.y, A2.y));		\n\
		if( x2.y > x2.x )							\n\
		{											\n\
			float3 TEMP = A2.yzw;					\n\
			A2.yzw = A1.yzw;						\n\
			A1.yzw = TEMP;							\n\
			x2.x = x2.y;							\n\
		}											\n\
		if(x2.x < 1e-10) return;					\n\
		A1.yzw /= A1.y;								\n\
		A2.yzw -= A2.y * A1.yzw;					\n\
		if(abs(A2.z) < 1e-10) return;\n"
	// compute dx, dy, ds: 
	<<"\
	    float3 dxys;							\n\
		dxys.z = A2.w /A2.z;				    \n\
		dxys.y = A1.w - dxys.z*A1.z;			    \n\
		dxys.x = A0.w - dxys.z*A0.z - dxys.y*A0.y;	\n"

	//one more threshold which I forgot in the version previous to 286
	<<"\
	  bool bugfix_test = (abs(cc.g + 0.5*dot(float3(fy, fy, fs), dxys ))>"<<threshold1<<") ;\n"
    //keep the point when the offset is less than 1
	<<"\
		FragData1 = bugfix_test && all( abs(dxys) < 1.0)? float4( dog, dxys) : float4(0, 0, 0, 0); \n\
	}\n"	<<'\0';

	else		out<<"FragData1 =  float4( dog, 0, 0, 0) ;	}\n"	<<'\0';

	ProgramCG * program; 
	s_keypoint = program = new ProgramCG(buffer);
	//parameter
	_param_dog_texu = cgGetNamedParameter(*program, "texU");
	_param_dog_texd = cgGetNamedParameter(*program, "texD");


	//
	s_grad_pass = program = new ProgramCG(
	"void main (\
	float4 TexCC : TEXCOORD0, float4 TexLC : TEXCOORD1,\n\
	float4 TexRC : TEXCOORD2, float4 TexCD : TEXCOORD3, float4 TexCU : TEXCOORD4,\n\
	out float4 FragData0 : COLOR0, uniform samplerRECT tex)\n\
	{\n\
		float4 v1, v2, gg;\n\
		float4 cc  = texRECT(tex, TexCC.xy);\n\
		gg.x = texRECT(tex, TexLC.xy).r;\n\
		gg.y = texRECT(tex, TexRC.xy).r;\n\
		gg.z = texRECT(tex, TexCD.xy).r;\n\
		gg.w = texRECT(tex, TexCU.xy).r;\n\
		float2 dxdy = 0.5*(gg.yw - gg.xz); \n\
		float grad = length(dxdy);\n\
		float theta = grad==0? 0: atan2(dxdy.y, dxdy.x);\n\
		FragData0 = float4(cc.rg, grad, theta);\n\
	}\n\0");

}


void ShaderBagCG::LoadDogShader1(float threshold, float edge_threshold)
{
	char buffer2[10240];

	//first pass, texcoord order ..c, cl, cr, uc, ud, 
	//compare with 8 neighbouring pixels in the same level
	//also compute gradient
	

	//tex(X)(Y)
	//X: (CLR) (CENTER 0, LEFT -1, RIGHT +1)  
	//Y: (CDU) (CENTER 0, DOWN -1, UP    +1) 
	// (the actual vertical axis is flipped, but we just use it)

	char * buffer1 = 
	"void main (\
	float4 TexCC : TEXCOORD0, float4 TexLC : TEXCOORD1,\n\
	float4 TexRC : TEXCOORD2, float4 TexCD : TEXCOORD3, \n\
	float4 TexCU : TEXCOORD4, float4 TexLD : TEXCOORD5, \n\
	float4 TexLU : TEXCOORD6, float4 TexRD : TEXCOORD7,\n\
	out float4 FragData0 : COLOR0, out float4 FragData1 : COLOR1, uniform samplerRECT tex){\n\
	float4 v1, v2, gg;\n\
	float4 c  = texRECT(tex, TexCC.xy);\n\
	v1.x = texRECT(tex, TexLC.xy).g;\n\
	gg.x = texRECT(tex, TexLC.xy).r;\n\
	v1.y = texRECT(tex, TexRC.xy).g;\n\
	gg.y = texRECT(tex, TexRC.xy).r;\n\
	v1.z = texRECT(tex, TexCD.xy).g;\n\
	gg.z = texRECT(tex, TexCD.xy).r;\n\
	v1.w = texRECT(tex, TexCU.xy).g;\n\
	gg.w = texRECT(tex, TexCU.xy).r;\n\
	v2.x = texRECT(tex, TexLD.xy).g;\n\
	v2.y = texRECT(tex, TexLU.xy).g;\n\
	v2.z = texRECT(tex, TexRD.xy).g;\n\
	v2.w = texRECT(tex, float2(TexRC.x, TexCU.y)).g;\n\
	float2 dxy = gg.yw - gg.xz; \n\
	float grad = length(dxy);\n\
	float theta = grad==0? 0: atan2(dxy.y, dxy.x);\n\
	float4 mmax4 = max(v1, v2);\n\
	float4 mmin4 = min(v1, v2);\n\
	float2 mmax2 = max(mmax4.xy, mmax4.zw);\n\
	float2 mmin2 = min(mmin4.xy, mmin4.zw);\n\
	float mmax = max(mmax2.x, mmax2.y);\n\
	float mmin = min(mmin2.x, mmin2.y);\n";



	if(GlobalUtil::_EdgeSupression && GlobalUtil::_SupportFP40)
	{
	sprintf(buffer2, "%s\
	bool4 glt;\n\
	glt.xy = (c.gg > float2(mmax, %f)); \n\
	glt.zw = (c.gg < float2(mmin, %f)); \n\
	FragData0 = float4(c.rg, grad*0.5, theta);\n\
	float dog = all(glt.xy)?1.0: (all(glt.zw)?0.5:0.0);\n\
	if(dog>0){\n\
		float4 D2 = v1.xyzw - c.gggg;\n\
		float2 D3 = D2.xz + D2.yw;\n\
		float  dxyplus = D3.x + D3.y;\n\
		float2 dxy2 = v2.xw - v2.yz;\n\
		float Dxy = 0.25*(dxy2.x + dxy2.y);\n\
		float up = dxyplus*dxyplus; \n\
		float down = (D3.x*D3.y - Dxy*Dxy);\n\
		if( down <= 0 || up > %f * down) dog = 0.0;\n\
	}\n\
	FragData1 = float4(dog, c.g, (glt.x? c.g:mmax), (glt.z?c.g:mmin));\n}\n",
	buffer1, threshold, -threshold, (edge_threshold+1)*(edge_threshold+1)/edge_threshold);
	}else
	{
	sprintf(buffer2, "%s\
	bool4 glt;\n\
	glt.xy = (c.gg > float2(mmax, %f)); \n\
	glt.zw = (c.gg < float2(mmin, %f)); \n\
	FragData0 = float4(c.rg, grad*0.5, theta);\n\
	float dog = all(glt.xy)?1.0: (all(glt.zw)?0.5:0.0);\n\
	FragData1 = float4(dog, c.g, (glt.x? c.g:mmax), (glt.z?c.g:mmin));\n}\n",
	buffer1, threshold, -threshold);
	}

	s_dog_pass1 = new ProgramCG(buffer2);



	//for some levels, we just need to compute the gradient
	sprintf(buffer2, "%s\
	FragData0 = float4(c.rg, grad*0.5, theta);\n\
	FragData1 = float4(0, c.g, max(c.g, mmax), min(c.g, mmin));\n}\n", buffer1);

	s_grad_pass = new ProgramCG(buffer2);


	
	
}

void ShaderBagCG::SetDogTexParam(int texU, int texD)
{
	cgGLSetTextureParameter(_param_dog_texu, texU);
	cgGLEnableTextureParameter(_param_dog_texu);
	cgGLSetTextureParameter(_param_dog_texd, texD);
	cgGLEnableTextureParameter(_param_dog_texd);
}

void ShaderBagCG::SetGenListStepParam(int tex, int tex0)
{
	cgGLSetTextureParameter(_param_genlist_step_tex, tex);
	cgGLEnableTextureParameter(_param_genlist_step_tex);
	cgGLSetTextureParameter(_param_genlist_step_tex0, tex0);
	cgGLEnableTextureParameter(_param_genlist_step_tex0);
}

void ShaderBagCG::SetGenVBOParam(float width, float fwidth, float size)
{
	float sizes[4] = {size*3.0f, fwidth, width, 1.0f/width};
	cgGLSetParameter4fv(_param_genvbo_size, sizes);
}


ProgramGPU* FilterGLCG::CreateFilterH(float kernel[], float offset[], int width)
{


	char buffer[10240];
	ostrstream out(buffer, 10240);

	if(GlobalUtil::_MultiTexCoord)
	{
		out<< "void main(uniform samplerRECT tex,";
		for(int j = 0; j < 8; j ++)
			out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 )";
		out<<"\n{\n\tfloat intensity = 0.0 ;  float2 pos;\n";
		for(int i = 0; i< width; i++)
		{

			if(offset[i]==0.0)
			{

				out<<"float or = texRECT(tex, TexCoord0.xy).r;\n";
				out<<"intensity+= or * "<<kernel[i]<<";\n";

			}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
			{

				int off = int(offset[i]);
				int ip = off>0? 2*off-1: -2*off; 
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, TexCoord"<<ip<<".xy).r;\n";
			}		
			else
			{
				out<<"pos = TexCoord0.xy + float2(float("<<offset[i] <<") , 0);\n";
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, pos).r;\n";
			}
		}

	}else
	{
		out<< "void main(uniform samplerRECT tex,";
		out<<"\n\tin float4 TexCoord0: TEXCOORD0,";
		out<<"\n\tout float4 FragColor : COLOR0 )";
		out<<"\n{\n\tfloat intensity = 0.0 ;  float2 pos;\n";

		for(int i = 0; i< width; i++)
		{
			if(offset[i]==0.0)
			{

				out<<"float or = texRECT(tex, TexCoord0.xy).r;\n";
				out<<"intensity+= or * "<<kernel[i]<<";\n";

			}else
			{
				out<<"pos = TexCoord0.xy + float2(float("<<offset[i] <<") , 0);\n";
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, pos).r;\n";
			}
		}
	}
	//copy original data to red channel
	out<<"FragColor.r = or;\n"; 
	out<<"FragColor.b  = intensity;}\n"<<'\0';

	return new ProgramCG( buffer);
}


ProgramGPU* FilterGLCG::CreateFilterV(float kernel[], float offset[], int height)
{
	char buffer[10240];
	ostrstream out(buffer, 10240);

	if(GlobalUtil::_MultiTexCoord)
	{
		out<< "void main(uniform samplerRECT tex,";
		for(int j = 0; j < 8; j ++)
			out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 )";
		out<<"\n{\n\tfloat intensity = 0.0 ;  float2 pos;\n";
		for(int i = 0; i< height; i++)
		{

			if(offset[i]==0.0)
			{
				out<<"float2 orb = texRECT(tex, TexCoord0.xy).rb;\n";
				out<<"intensity+= orb.y * "<<kernel[i]<<";\n";

			}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
			{
				int off = int(offset[i]);
				int ip = off>0? 2*off-1: -2*off; 
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, TexCoord"<<ip<<".xy).b;\n";
			}		
			else
			{
				out<<"pos = TexCoord0.xy + float2(0, float("<<offset[i] <<") );\n";
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, pos).b;\n";
			}
		}

	}else
	{
		out<< "void main(uniform samplerRECT tex,";
		out<<"\n\tin float4 TexCoord0: TEXCOORD0,";
		out<<"\n\tout float4 FragColor : COLOR0 )";
		out<<"\n{\n\tfloat intensity = 0.0 ;  float2 pos;\n";

		for(int i = 0; i< height; i++)
		{
			if(offset[i]==0.0)
			{
				out<<"float2 orb = texRECT(tex, TexCoord0.xy).rb;\n";
				out<<"intensity+= orb.y * "<<kernel[i]<<";\n";

			}else
			{
				out<<"pos = TexCoord0.xy + float2(0, float("<<offset[i] <<"));\n";
				out<<"intensity+= "<<kernel[i]<<"*texRECT(tex, pos).b;\n";
			}
		}
	}
	out<<"FragColor.b = orb.y;\n";
	out<<"FragColor.g = intensity - orb.x;\n"; // difference of gaussian..
	out<<"FragColor.r = intensity;}\n"<<'\0';
	
	return new ProgramCG( buffer);
}

void ProgramCG::SetTexParameter(unsigned int texID)
{
	cgGLSetTextureParameter(_texParamID, texID);	
	cgGLEnableTextureParameter(_texParamID);
}
ProgramGPU* FilterPKCG::CreateFilterH(float kernel[], float offset[], int width)
{
	return GlobalUtil::_usePackedTex ==2 ?
		CreateFilterH2(kernel, offset, width):
		CreateFilterH1(kernel, offset, width);

}
//PACKED VERSION
ProgramGPU* FilterPKCG::CreateFilterV(float kernel[], float offset[], int height)
{
	return GlobalUtil::_usePackedTex ==2 ?
		CreateFilterV2(kernel, offset, height):
		CreateFilterV1(kernel, offset, height);
}


ProgramGPU* FilterPKCG::CreateFilterH2(float kernel[], float offset[], int width)
{
	//both h and v are packed...
	int i, j , ip, di, k,  xw, xwn;

	int halfwidth  = width >>1;
	float * pf = kernel + halfwidth;
	int nhpixel = (halfwidth+1)>>1;	//how many neighbour pixels need to be looked up
	int npixel  = (nhpixel<<1)+1;//
	char buffer[10240];
	float weight[2][2];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texRECT" ;
	out<< "void main(uniform samplerRECT tex,";
	for( j = 0; j < 8; j ++)	out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 ){\n";
	out<< "FragColor = float4(0, 0, 0, 0); \n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<"float4 pc; float3 weight;\n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, TexCoord"<<ip<<".xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, TexCoord0.xy + float2(float("<<i-nhpixel<<"),0));\n";
		}
		//for each sub-pixel j  in center, the weight of sub-pixel k 
		xw = (i - nhpixel)*2;
		for( j = 0; j < 2; j++)
		{
			for (k = 0; k < 2 ; k++)
			{
				xwn = xw - j + k;
				if( xwn < -halfwidth || xwn > halfwidth)
				{
					weight[j][k] = 0;
				}else
				{
					weight[j][k] = pf[xwn];
				}
			}
		}
		if(weight[0][0]!=0.0)	out<<"FragColor += "<<weight[0][0]<<"*pc;\n";
		out<<"FragColor += float4("<<weight[0][1]<<","<<weight[1][0]<<","<<weight[0][1]<<","<<weight[1][0]<<")*pc.grab;\n";

	}
	out<<"}\n"<<'\0';
//	std::cout<<buffer<<endl;
	
	return new ProgramCG( buffer);


}

//PACKED VERSION
ProgramGPU* FilterPKCG::CreateFilterH1(float kernel[], float offset[], int width)
{
	int i, j , ip, di, k,  xw, xwn;

	int halfwidth  = width >>1;
	float * pf = kernel + halfwidth;
	int nhpixel = (halfwidth+3)>>2;	//how many neighbour pixels need to be looked up
	int npixel  = (nhpixel<<1)+1;//
	int nmpixel = (nhpixel<<2)-halfwidth;
	char buffer[10240];
	float weight[4][4];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texRECT" ;
	out<< "void main(uniform samplerRECT tex,";
	for( j = 0; j < 8; j ++)	out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 ){\n";
	out<< "FragColor = float4(0, 0, 0, 0); \n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<" float4 pc;\n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, TexCoord"<<ip<<".xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, TexCoord0.xy + vec2(float("<<i-nhpixel<<"),0));\n";
		}
		//for each sub-pixel j  in center, the weight of sub-pixel k 
		xw = (i - nhpixel)*4;
		for( j = 0; j < 4; j++)
		{
			for (k = 0; k < 4 ; k++)
			{
				xwn = xw - j + k;
				if( xwn < -halfwidth || xwn > halfwidth)
				{
					weight[k][j] = 0;
				}else
				{
					weight[k][j] = pf[xwn];
				}
			}
		}
		out<<"FragColor +=  mul(float4x4(\n";
		for( j = 0; j < 15; j ++)
		{
			out<<weight[0][j]<<",\t";
			if(j%4 ==3) out<<endl;
		}
		out<<weight[0][15]<<"), pc);\n";
	}
	out<<"}\n"<<'\0';

	return new ProgramCG( buffer);

}
ProgramGPU* FilterPKCG::CreateFilterV2(float kernel[], float offset[], int height)
{

	//both h and v are packed...
	int i, j , ip, di, k,  yw, ywn;

	int halfh  = height >>1;
	float * pf = kernel + halfh;
	int nhpixel = (halfh+1)>>1;	//how many neighbour pixels need to be looked up
	int npixel  = (nhpixel<<1)+1;//
	char buffer[10240];
	float weight[2][2];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texRECT" ;
	out<< "void main(uniform samplerRECT tex,";
	for(j = 0; j < 8; j ++)	out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 ){\n";
	out<< "FragColor = float4(0, 0, 0, 0);\n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<"float4 pc;\n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, TexCoord"<<ip<<".xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, TexCoord0.xy + float2(0, float("<<i-nhpixel<<")));\n";
		}
		//for each sub-pixel j  in center, the weight of sub-pixel k 
		yw = (i - nhpixel)*2;
		for( j = 0; j < 2; j++)
		{
			for (k = 0; k < 2 ; k++)
			{
				ywn = yw - j + k;
				if( ywn < -halfh || ywn > halfh)
				{
					weight[j][k] = 0;
				}else
				{
					weight[j][k] = pf[ywn];
				}
			}
		}
		if(weight[0][0]!=0.0)	out<<"FragColor += "<<weight[0][0]<<"*pc;\n";
		out<<"FragColor += float4("<<weight[0][1]<<","<<weight[1][0]<<","<<weight[0][1]<<","<<weight[1][0]<<")*pc.brag;\n";
	}
	out<<"}\n"<<'\0';
//	std::cout<<buffer<<endl;
	
	return new ProgramCG( buffer);
}


//PACKED VERSION
ProgramGPU* FilterPKCG::CreateFilterV1(float kernel[], float offset[], int height)
{
	char buffer[10240];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texRECT" ;
	out<< "void main(uniform samplerRECT tex,";
	for(int j = 0; j < 8; j ++)	out<<"\n\tin float4 TexCoord"<<j<<" : TEXCOORD"<<j<<",";
		out<<"\n\tout float4 FragColor : COLOR0 ){\n";
	out<< "FragColor = float4(0, 0, 0, 0); float2 pos;pos.x = TexCoord0.x;\n";
	for(int i = 0; i< height; i++)
	{
		if(offset[i]==0.0)
		{
			out<<"FragColor += "<<texLookUp<<"(tex, TexCoord0.xy) * "<<kernel[i]<<";\n";
		}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
		{
			int off = int(offset[i]);
			int ip = off>0? 2*off-1: -2*off; 
			out<<"FragColor += "<<texLookUp<<"(tex, TexCoord"<< ip <<".xy) * "<<kernel[i]<<";\n";
		}else
		{
			out<<"pos.y = TexCoord0.y +  float("<<offset[i] <<");\n";
			out<<"FragColor += "<<texLookUp<<"(tex, pos) * "<<kernel[i]<<";\n";
		}
	
	}
	out<<"}"<<'\0';

	return new ProgramCG( buffer);
}


void ShaderBagCG::LoadGenListShader(int ndoglev, int nlev)
{
	ProgramCG * program;

	s_genlist_init = new ProgramCG(
	"void main (\n\
	uniform samplerRECT tex, in float4 TexCoord0 : TEXCOORD0,\n\
	in float4 TexCoord1 : TEXCOORD1, in float4 TexCoord2 : TEXCOORD2, in float4 TexCoord3 : TEXCOORD3,\n\
	out float4 FragColor : COLOR0){\n\
	float4 helper = float4( texRECT(tex, TexCoord0.xy).r,  texRECT(tex, TexCoord1.xy).r,\n\
	texRECT(tex, TexCoord2.xy).r, texRECT(tex, TexCoord3.xy).r);\n\
	FragColor = float4(helper>0.0);\n\
	}");

	s_genlist_init_ex = program = new ProgramCG(
	"void main (uniform float2 bbox, \n\
	uniform samplerRECT tex, \n\
	in float4 TexCoord0 : TEXCOORD0,\n\
	in float4 TexCoord1 : TEXCOORD1, \n\
	in float4 TexCoord2 : TEXCOORD2, \n\
	in float4 TexCoord3 : TEXCOORD3,\n\
	out float4 FragColor : COLOR0){\n\
	float4 helper = float4( \n\
	texRECT(tex, TexCoord0.xy).r, texRECT(tex, TexCoord1.xy).r,\n\
	texRECT(tex, TexCoord2.xy).r, texRECT(tex, TexCoord3.xy).r);\n\
	bool4 helper2 = bool4( \n\
	all(TexCoord0.xy < bbox), all(TexCoord1.xy < bbox),\n\
	all(TexCoord2.xy < bbox), all(TexCoord3.xy < bbox));\n\
	FragColor = float4(helper2 && (helper>0.0 ));\n\
	}");
	_param_genlist_init_bbox = cgGetNamedParameter( *program, "bbox");


	//reduction ...
	s_genlist_histo = new ProgramCG(
	"void main (\n\
	uniform samplerRECT tex, in float2 TexCoord0 : TEXCOORD0,\n\
	in float2 TexCoord1 : TEXCOORD1, in float2 TexCoord2 : TEXCOORD2, in float2 TexCoord3 : TEXCOORD3,\n\
	out float4 FragColor : COLOR0){\n\
	float4 helper; float4 helper2; \n\
	helper = texRECT(tex, TexCoord0); helper2.xy = helper.xy + helper.zw; \n\
	helper = texRECT(tex, TexCoord1); helper2.zw = helper.xy + helper.zw; \n\
	FragColor.rg = helper2.xz + helper2.yw;\n\
	helper = texRECT(tex, TexCoord2); helper2.xy = helper.xy + helper.zw; \n\
	helper = texRECT(tex, TexCoord3); helper2.zw = helper.xy + helper.zw; \n\
	FragColor.ba= helper2.xz+helper2.yw;\n\
	}");


	//read of the first part, which generates tex coordinates 

	s_genlist_start= program =  LoadGenListStepShader(1, 1);
	_param_ftex_width= cgGetNamedParameter(*program, "width");
	_param_genlist_start_tex0 = cgGetNamedParameter(*program, "tex0");
	//stepping
	s_genlist_step = program = LoadGenListStepShader(0, 1);
	_param_genlist_step_tex= cgGetNamedParameter(*program, "tex");
	_param_genlist_step_tex0= cgGetNamedParameter(*program, "tex0");


}

ProgramCG* ShaderBagCG::LoadGenListStepShader(int start, int step)
{
	int i;
	char buffer[10240];
	char chanels[5] = "rgba";
	ostrstream out(buffer, 10240);
	out<<"void main(out float4 FragColor : COLOR0, \n";

	for(i = 0; i < step; i++) out<<"uniform samplerRECT tex"<<i<<",\n";

	if(start)
	{
		out<<"uniform float width, \nin float2 tpos : TEXCOORD0){\n";
		out<<"float  index = floor(tpos.y) * width + floor(tpos.x);\n";
		out<<"float2 pos = float2(0.5, 0.5);\n";
	}else
	{
		out<<"uniform samplerRECT tex, in float2 tpos: TEXCOORD0 ){\n";
		out<<"float4 tc = texRECT( tex, tpos);\n";
		out<<"float2 pos = tc.rg; float index = tc.b;\n";
	}
	out<<"float2 sum; 	float4 cc;\n";



	if(step>0)
	{
		out<<"float2 cpos = float2(-0.5, 0.5);\t float2 opos;\n";
		for(i = 0; i < step; i++)
		{
//#define SETP_CODE_2

#ifndef SETP_CODE_2
			out<<"cc = texRECT(tex"<<i<<", pos);\n";
			out<<"if (index <cc.r){ opos = cpos.xx;}\n";
			out<<"else{sum.x = cc.r + cc.g;";
					out<<"if(index < sum.x ) {opos = cpos.yx; index -= cc.r;}\n";
					out<<"else{sum.y = sum.x + cc.b;";
							out<<"if(index < sum.y ) {opos = cpos.xy; index -= sum.x;}\n";
							out<<"else {opos = cpos.yy; index -= sum.y;}}}\n";
			out<<"pos = (pos + pos + opos);\n";

#else
			out<<"cc = texRECT(tex"<<i<<", pos);\n";
			out<<"if (index < cc.r) opos = cpos.xx;\n";
			out<<"else if (index < cc.r + cc.g){opos = cpos.yx; index -= cc.r;}\n";
			out<<"else if (index < cc.r + cc.g + cc.b){opos = cpos.xy; index -= (cc.r + cc.g);}\n";
			out<<"else {opos = cpos.yy; index -= (cc.r + cc.g + cc.b);}\n";
			out<<"pos = (pos + pos + opos);\n";
#endif
		}
	}
	out<<"FragColor = float4(pos, index, 1);\n";
	out<<"}\n"<<'\0';
	return new ProgramCG(buffer);
}

void ShaderBagCG::SetGenListInitParam(int w, int h)
{
	float bbox[2] = {w -1.0f, h - 1.0f};
	cgGLSetParameter2fv(_param_genlist_init_bbox, bbox);
}

void ShaderBagCG::SetGenListStartParam(float width, int tex0)
{
	cgGLSetParameter1f(_param_ftex_width, width);

	if(_param_genlist_start_tex0)
	{
		cgGLSetTextureParameter(_param_genlist_start_tex0, tex0);
		cgGLEnableTextureParameter(_param_genlist_start_tex0);
	}
}

void ShaderBagCG::LoadDescriptorShaderF4()
{
	//one shader outpout 128/8 = 16 , each fragout encodes 4
	const double twopi = 2.0*3.14159265358979323846;
	const double rpi  = 8.0/twopi;
	char buffer[10240];
	ostrstream out(buffer, 10240);

	out<<setprecision(12);

	out<<"\n\
	#define WF "<<GlobalUtil::_DescriptorWindowFactor <<"\n\
	void main(uniform samplerRECT tex,		\n\
	uniform	samplerRECT gradTex,			\n\
	uniform float4		dsize,				\n\
	uniform float2		size,				\n\
	in		float2	TexCoord0 : TEXCOORD0,	\n\
	out		float4  FragData0:COLOR0,		\n\
	out		float4	FragData1:COLOR1,		\n\
	out		float4  FragData2:COLOR2,		\n\
	out		float4  FragData3:COLOR3)		\n\
	{\n\
		float4 D0A, D0B, D1A, D1B;			\n\
		D0A = D0B = D1A = D1B = float4(0, 0, 0, 0);		\n\
		float2 dim	= size.xy;	//image size			\n\
		float index = dsize.x*floor(TexCoord0.y) + TexCoord0.x;\n\
		float idx = fmod(index, 8.0);		\n\
		index = floor(index*0.125);  \n\
		float2 coord = floor( float2( fmod(index, dsize.z), index*dsize.w)) + 0.5 ;\n\
		float2 pos = texRECT(tex, coord).xy;		\n\
		if(any(pos.xy <= 1) || any(pos.xy >=dim-1)) discard;	\n\
		float  anglef = texRECT(tex, coord).z;\n\
		float sigma = texRECT(tex, coord).w; \n\
		float spt  = sigma * WF;	//default to be 3*sigma	\n";
	//rotation
	out<<"\
		float4 cscs, rots;								\n\
		sincos(anglef, cscs.y, cscs.x);					\n\
		cscs.zw = - cscs.xy;							\n\
		rots = cscs /spt;								\n\
		cscs *= spt; \n";

	//here cscs is actually (cos, sin, -cos, -sin) * (factor: 3)*sigma
	//and rots is  (cos, sin, -cos, -sin ) /(factor*sigma)
	//decide which part of the grid  by  coord
	//compute the upper-left corner of the sample region
	//devide the 4x4 sift grid into 8 2x1,..each 2x1 is obtained from a shader thread
	//to use linear interoplation, enlarge 2x1 to 3x2, by adding 0.5 to each side
	out<<"\
		float4 offset, temp; float2 pt;					\n\
		offset.x = fmod(idx, 2)<1? -2.5:-0.5;		\n\
		offset.y = floor(idx*0.5) - 2.5;			\n\
		offset.zw = offset.xy + float2(3.0, 2.0);	\n\
		float4 offsetpt = offset + 0.5;				\n\
		temp = cscs.xwyx*offsetpt.xyxy;				\n\
		pt = pos + temp.xz + temp.yw;						\n";
	
	//get a horizontal bounding box of the rotated rectangle
	out<<"\
		float2 p1, p2, p3, p4;			\n\
		temp = cscs.xwyx*offset.xyxy;				\n\
		p1 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zyzy;				\n\
		p2 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zwzw;				\n\
		p3 =  temp.xz + temp.yw;					\n\
		temp = cscs.xwyx*offset.xwxw;				\n\
		p4 =  temp.xz + temp.yw;					\n\
		float2 pmin = min(min(p1, p2), min(p3, p4));\n\
		float2 pmax = max(max(p1, p2), max(p3, p4));\n\
		float4 sz;	float2 spos;					\n\
		sz.xy = max( pos + pmin , float2(1,1));\n\
		sz.zw = min( pos + pmax , dim -2);		\n\
		sz = floor(sz)+0.5;"; //move sample point to pixel center
	//get voting for two box

//#define DESCRIPTOR_DEBUG

#ifdef DESCRIPTOR_DEBUG
//	out<<"\n\
//		 D1B = sz;	D0B = size; D1A = offset;	D0A = float4(pos, pt);\n";
#else
	out<<"\n\
		for(spos.x = sz.x; spos.x <= sz.z;	spos.x+=1.0)				\n\
		{																\n\
			for(spos.y = sz.y; spos.y <= sz.w;	spos.y+=1.0)			\n\
			{															\n\
				float2 diff = spos - pt;								\n\
				temp = rots.xywx * diff.xyxy;\n\
				float2 nxy = (temp.xz + temp.yw); \n\
				if(all(nxy > float2(-0.5,-0.5) && nxy < float2(2.5, 1.5)))\n\
				{\n\
					float4 cc = texRECT(gradTex, spos);						\n\
					float mod = cc.b;	float angle = cc.a;					\n\
					float theta = fmod(8+(anglef - angle)*"<<rpi<<", 8); \n\
					diff = nxy + offsetpt.xy;								\n\
					float ww = exp(-0.125*dot(diff, diff));\n\
					float weight = (1 - abs(nxy.y - 0.5))*mod*ww; \n\
					float4 wa, wb; \n\
					float theta1 = floor(theta); \n\
					float weight1 = theta1 + 1.0 - theta; float weight2 = theta - theta1;\n\
					wa = float4(theta1 == float4(0, 1, 2, 3))*weight1 + float4(theta1 == float4(7, 0, 1, 2))*weight2; \n\
					wb = float4(theta1 == float4(4, 5, 6, 7))*weight1 + float4(theta1 == float4(3, 4, 5, 6))*weight2; \n\
					if(nxy.x < 0.5)\n\
					{\n\
						weight = (0.5+nxy.x)*weight; \n\
						D0A+=wa*weight;	D0B+=wb*weight; \n\
					}else if(nxy.x < 1.5)\n\
					{\n\
						weight1 = (1.5-nxy.x)*weight; weight2 = (nxy.x - 0.5)*weight; \n\
						D0A+=wa*weight1;	D0B+=wb*weight1; \n\
						D1A+=wa*weight2;	D1B+=wb*weight2; \n\
					}else\n\
					{\n\
						weight = (2.5-nxy.x)*weight; \n\
						D1A+=wa*weight;	D1B+=wb*weight; \n\
					}\n\
				}\n\
			}\n\
		}\n";
#endif
	out<<"\
		FragData0 = D0A; FragData1 = D0B;\n\
		FragData2 = D1A; FragData3 = D1B;\n\
	}\n"<<'\0';
	ProgramCG * program; 
	s_descriptor_fp = program =  new ProgramCG(buffer);
	_param_descriptor_gtex = cgGetNamedParameter(*program, "gradTex");
	_param_descriptor_size = cgGetNamedParameter(*program, "size");
	_param_descriptor_dsize = cgGetNamedParameter(*program, "dsize");
}

void ShaderBagCG::LoadDescriptorShaderF8()
{
	//one shader outpout 128/4 = 32 , each fragout encodes 4
	const double twopi = 2.0*3.14159265358979323846;
	const double rpi  = 8.0/twopi;
	char buffer[10240];
	ostrstream out(buffer, 10240);

	out<<"\n\
	#define WF "<<GlobalUtil::_DescriptorWindowFactor <<"\n\
	void main(uniform samplerRECT tex,		\n\
	uniform	samplerRECT gradTex,			\n\
	uniform float4		dsize,				\n\
	uniform float2		size,				\n\
	in		float2	TexCoord0 : TEXCOORD0,	\n\
	out		float4  FragData0:COLOR0,		\n\
	out		float4	FragData1:COLOR1,		\n\
	out		float4  FragData2:COLOR2,		\n\
	out		float4  FragData3:COLOR3,		\n\
	out		float4	FragData4:COLOR4,		\n\
	out		float4  FragData5:COLOR5,		\n\
	out		float4  FragData6:COLOR6,		\n\
	out		float4  FragData7:COLOR7)		\n\
	{\n\
		float4 D00A, D00B, D01A, D01B, D10A, D10B, D11A, D11B;			\n\
		D00A=D00B=D01A=D01B=D10A=D10B=D11A=D11B=float4(0, 0, 0, 0);\n\
		float2 dim	= size.xy;	//image size			\n\
		float index = dsize.x*floor(TexCoord0.y) + TexCoord0.x;\n\
		float idx = fmod(index, 4.0);		\n\
		index = floor(index*0.25);  \n\
		float2 coord = floor( float2( fmod(index, dsize.z), index*dsize.w)) + 0.5 ;\n\
		float2 pos = texRECT(tex, coord).xy;		\n\
		float  anglef = texRECT(tex, coord).z;		\n\
		float sigma = texRECT(tex, coord).w; \n\
		float spt  = sigma * WF;	//default to be 3*sigma	\n";
	//rotation
	out<<"\n\
		float4 cscs, rots;								\n\
		sincos(anglef, cscs.y, cscs.x);					\n\
		cscs.zw = - cscs.xy;							\n\
		rots = cscs /spt;								\n\
		cscs *= spt; \n";
	//decide which part of the grid is by y coord
	out<<"\
		float4 offset, temp; float2 pt;				\n\
		offset.x = fmod(idx, 2)<1? -2.5:-0.5;		\n\
		offset.y = idx <2?- 2.5 : - 0.5;			\n\
		offset.zw = offset.xy + float2(3.0, 3.0);	\n\
		float4 offsetpt = offset + 0.5;				\n\
		temp = cscs.xwyx*offsetpt.xyxy;				\n\
		pt = pos + temp.xz + temp.yw;				\n";
	//get the coordinate region of the rotated rectangle
	out<<"\
		float2 p1, p2, p3, p4;			\n\
		temp = cscs.xwyx*offset.xyxy;				\n\
		p1 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zyzy;				\n\
		p2 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zwzw;				\n\
		p3 =  temp.xz + temp.yw;					\n\
		temp = cscs.xwyx*offset.xwxw;				\n\
		p4 =  temp.xz + temp.yw;					\n\
		float2 pmin = min(min(p1, p2), min(p3, p4));\n\
		float2 pmax = max(max(p1, p2), max(p3, p4));\n\
		float4 sz;	float2 spos;					\n\
		sz.xy = max( pos + pmin, float2(1,1));\n\
		sz.zw = min( pos + pmax, dim -2);		\n\
		sz = floor(sz)+0.5;";
	//get voting for two box
	out<<"\n\
		for(spos.x = sz.x; spos.x <= sz.z;	spos.x+=1.0)				\n\
		{																\n\
			for(spos.y = sz.y; spos.y <= sz.w;	spos.y+=1.0)			\n\
			{															\n\
				float2 diff = spos - pt;								\n\
				temp = rots.xywx * diff.xyxy;\n\
				float2 nxy = (temp.xz + temp.yw); \n\
				if(all(nxy >= float2(-0.5,-0.5) && nxy <= float2(2.5, 2.5)))\n\
				{\n\
					float4 cc = texRECT(gradTex, spos);				\n\
					diff = nxy + offsetpt.xy;								\n\
					float mod = cc.b *  exp(-0.125*dot(diff, diff));	\n\
					float angle = cc.a;					\n\
					float theta = fmod(8+ (anglef - angle)*"<<rpi<<", 8); \n\
					float4 wa, wb; \n\
					float theta1 = floor(theta); \n\
					float weight1 = theta1 + 1.0 - theta; float weight2 = theta - theta1;\n\
					wa = float4(theta1 == float4(0, 1, 2, 3))*weight1 + float4(theta1 == float4(7, 0, 1, 2))*weight2; \n\
					wb = float4(theta1 == float4(4, 5, 6, 7))*weight1 + float4(theta1 == float4(3, 4, 5, 6))*weight2; \n\
					wa*= mod; wb*=mod; \n"
				<<"\
					if(nxy.x < 0.5)\n\
					{\n\
						float weight = 0.5+nxy.x; \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weight1 = weight* (0.5+nxy.y);\n\
							D00A+= wa * weight1;	D00B+=wb*weight1;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							weight1 = weight* (1.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							D00A+= wa * weight1;	D00B+=wb*weight1;\n\
							D10A+= wa * weight2;	D10B+=wb*weight2;\n\
						}else\n\
						{\n\
							weight2 = weight * (2.5 - nxy.y);\n\
							D10A+= wa * weight2;	D10B+=wb*weight2;\n\
						}\n\
					}"
				<<"else if(nxy.x < 1.5)\n\
					{\n\
						float2 weightx = float2(1.5 - nxy.x , nxy.x - 0.5); \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weightx*= (0.5+nxy.y);						\n\
							D00A+= wa * weightx.x;	D00B+=wb*weightx.x;\n\
							D01A+= wa * weightx.y;	D01B+=wb*weightx.y;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							float2 weighty = float2(1.5 - nxy.y, nxy.y - 0.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							D00A+= wa * weights.x;	D00B+=wb*weights.x;\n\
							D01A+= wa * weights.y;	D01B+=wb*weights.y;\n\
							D10A+= wa * weights.z;	D10B+=wb*weights.z;\n\
							D11A+= wa * weights.w;	D11B+=wb*weights.w;\n\
						}else\n\
						{\n\
							weightx*= (2.5 - nxy.y);						\n\
							D10A+= wa * weightx.x;	D10B+=wb*weightx.x;\n\
							D11A+= wa * weightx.y;	D11B+=wb*weightx.y;\n\
						}\n\
					}"
				<<"else\n\
					{\n\
						float weight = 2.5-nxy.x; \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weight1 = weight* (0.5+nxy.y);\n\
							D01A+= wa * weight1;	D01B+=wb*weight1;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							weight1 = weight* (1.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							D01A+= wa * weight1;	D01B+=wb*weight1;\n\
							D11A+= wa * weight2;	D11B+=wb*weight2;\n\
						}else\n\
						{\n\
							weight2 = weight * (2.5 - nxy.y);\n\
							D11A+= wa * weight2;	D11B+=wb*weight2;\n\
						}\n\
					}\n\
				}\n\
			}\n\
		}\n";
	out<<"\
		FragData0 = D00A; FragData1 = D00B;\n\
		FragData2 = D01A; FragData3 = D01B;\n\
 		FragData4 = D10A; FragData5 = D10B;\n\
		FragData6 = D11A; FragData7 = D11B;\n\
	}\n"<<'\0';
	ProgramCG * program; 
	s_descriptor_fp = program =  new ProgramCG(buffer);
	_param_descriptor_gtex = cgGetNamedParameter(*program, "gradTex");
	_param_descriptor_size = cgGetNamedParameter(*program, "size");
	_param_descriptor_dsize = cgGetNamedParameter(*program, "dsize");
}

//the shader that computes the descriptors
void ShaderBagCG::LoadDescriptorShader()
{

	if(GlobalUtil::_DescriptorFormat ==32)
		LoadDescriptorShaderG();
	else if(GlobalUtil::_DescriptorFormat ==8)
		LoadDescriptorShaderF4();
	else
		if(GlobalUtil::_DescriptorFormat ==4)
		LoadDescriptorShaderF8();
	
}

void ShaderBagCG::LoadOrientationShader()
{
	///
	const int method=0;
	char buffer[10240];
	ostrstream out(buffer,10240);
//	const double pi = 3.14159265358979323846;
//	const double radians_per_degree = pi/180.;
//	const double astep = (pi/36.0);


	out<<"\n\
	#define GAUSSIAN_WF "<<GlobalUtil::_OrientationGaussianFactor<<" \n\
	#define SAMPLE_WF ("<<GlobalUtil::_OrientationWindowFactor * GlobalUtil::_OrientationExtraFactor<< " )\n\
	void main(uniform samplerRECT tex,			\n\
			uniform samplerRECT gradTex,		\n\
			uniform float4 size,				\n\
			in float2 TexCoord0 : TEXCOORD0,	\n\
			out float4 FeatureData : COLOR0	";

	//multi orientation output
	//use one additional texture to store up to four orientations
	if(GlobalUtil::_MaxOrientation >1 )
		out<<", out float4 OrientationData : COLOR1";

	if(GlobalUtil::_SubpixelLocalization)
	{
		//data for sub-pixel localization
		out<<", uniform samplerRECT texS";
	}

	//use 9 float4 to store histogram of 36 directions
	out<<")		\n\
	{													\n\
		float4 bins[10];								\n\
		int i, j , k ;									\n\
		for (i=0; i<9; i++) bins[i] = float4(0,0,0,0);	\n\
		float2 pos = texRECT(tex, TexCoord0).xy;		\n\
		float sigma = size.z;							\n";
	if(GlobalUtil::_SubpixelLocalization)
	{
		out<<"\
		float3 offset = texRECT(texS, pos).yzw;\n\
		sigma = sigma*pow(size.w, offset.z); \n\
		pos.xy = pos.xy + offset.xy; ";
	}
	out<<"\
		float2 win = sigma.xx * SAMPLE_WF;				\n\
		float2 dim = size.xy;	\n\
		float dist_threshold = win.x*win.x+0.5;			\n\
		float factor = -0.5/(win.x*win.xx * GAUSSIAN_WF * GAUSSIAN_WF);	\n\
		float4 sz;	float2 spos;						\n\
		//if(any(pos.xy <= 1)) discard;					\n\
		sz.xy = max( pos - win, float2(1,1));			\n\
		sz.zw = min( pos + win, dim-2);				\n\
		sz = floor(sz)+0.5;";
	//loop to get the histogram

	out<<"\n\
		for(spos.x = sz.x; spos.x <= sz.z;	spos.x+=1.0)				\n\
		{																\n\
			for(spos.y = sz.y; spos.y <= sz.w;	spos.y+=1.0)			\n\
			{															\n\
				float2 offset = spos - pos;								\n\
				float sq_dist = dot(offset,offset);						\n\
				if( sq_dist < dist_threshold){							\n\
					float4 cc = texRECT(gradTex, spos);						\n\
					float grad = cc.b;	float theta = cc.a;					\n\
					float idx = floor(degrees(theta)*0.1);		\n\
					if(idx < 0 ) idx += 36;									\n\
					float weight = grad*exp(sq_dist * factor);				\n\
					float vidx = fmod(idx, 4);								\n\
					float4 inc = weight*float4(vidx == float4(0,1,2,3));	";


	//
	if(strcmp(cgGetProfileString(ProgramCG::_FProfile), "gp4fp")==0)
//	if(ProgramCG::_FProfile == CG_PROFILE_GPU_FP) this enumerant is not defined in cg1.5
	{
		//gp_fp supports dynamic indexing
		out<<"\n\
					int iidx = int(floor(idx*0.25));	\n\
					bins[iidx]+=inc;					\n\
				}										\n\
			}											\n\
		}";

	}else
	{
		//nvfp40 still does not support dynamic array indexing
		//unrolled binary search...
		out<<"\n\
					if(idx < 16)							\n\
					{										\n\
						if(idx < 8)							\n\
						{									\n\
							if(idx < 4)	{	bins[0]+=inc;}	\n\
							else		{	bins[1]+=inc;}	\n\
						}else								\n\
						{									\n\
							if(idx < 12){	bins[2]+=inc;}	\n\
							else		{	bins[3]+=inc;}	\n\
						}									\n\
					}else if(idx < 32)						\n\
					{										\n\
						if(idx < 24)						\n\
						{									\n\
							if(idx <20)	{	bins[4]+=inc;}	\n\
							else		{	bins[5]+=inc;}	\n\
						}else								\n\
						{									\n\
							if(idx < 28){	bins[6]+=inc;}	\n\
							else		{	bins[7]+=inc;}	\n\
						}									\n\
					}else 						\n\
					{										\n\
						bins[8]+=inc;						\n\
					}										\n\
				}										\n\
			}											\n\
		}";

	}
	//smooth histogram and find the largest
/*
	smoothing kernel:	 (1 3 6 7 6 3 1 )/27
	the same as 3 pass of (1 1 1)/3 averaging
	maybe better to use 4 pass on the vectors...
*/


	//the inner loop on different array numbers is always unrolled in fp40

	//bug fixed here:)
	out<<"\n\
		float3x3 mat1 = float3x3(1, 0, 0, 3, 1, 0, 6, 3, 1)/27.0;; //bug fix.. \n\
		float4x4 mat2 = float4x4( 7, 6, 3, 1, 6, 7, 6, 3, 3, 6, 7, 6, 1, 3, 6, 7)/27.0;;\n\
		for (j=0; j<2; j++)								\n\
		{												\n\
			float4 prev  = bins[8];						\n\
			bins[9]		 = bins[0];						\n\
			for (i=0; i<9; i++)							\n\
			{												\n\
				float4 newb	=	mul ( bins[i], mat2);		\n\
				newb.xyz	+=	mul ( prev.yzw, mat1);		\n\
				prev = bins[i];								\n\
				newb.wzy	+=	mul	( bins[i+1].zyx, mat1);	\n\
				bins[i] = newb;							\n\
			}												\n\
		}";
/*	*/
	//find the maximum voting
	out<<"\n\
		float maxh; float2 maxh2; float4 maxh4 = bins[0];				\n\
		for (i=1; i<9; i++) maxh4 = max(maxh4, bins[i]);				\n\
		maxh2 = max(maxh4.xy, maxh4.zw); maxh = max(maxh2.x, maxh2.y);";

	char *testpeak_code;
	char *savepeak_code;



	//save two orientations with the largest votings?

	//
	if(GlobalUtil::_MaxOrientation>1)
	{
		out<<"\n\
		float4 Orientations = float4(0, 0, 0, 0);				\n\
		float4 weights = float4(0,0,0,0);		";	
		
		testpeak_code = "\n\
			{test = bins[i]>hh;";

		//save the orientation in weight-decreasing order
		if(GlobalUtil::_MaxOrientation ==2)
		{
		savepeak_code = "\n\
			if(weight <=weights.g){}\n\
			else if(weight >weights.r)\n\
			{weights.rg = float2(weight, weights.r); Orientations.rg = float2(th, Orientations.r);}\n\
			else {weights.g = weight; Orientations.g = th;}";
		}else if(GlobalUtil::_MaxOrientation ==3)
		{
		savepeak_code = "\n\
			if(weight <=weights.b){}\n\
			else if(weight >weights.r)\n\
			{weights.rgb = float3(weight, weights.rg); Orientations.rgb = float3(th, Orientations.rg);}\n\
			else if(weight >weights.g)\n\
			{weights.gb = float2(weight, weights.g); Orientations.gb = float2(th, Orientations.g);}\n\
			else {weights.b = weight; Orientations.b = th;}";
		}else
		{
		savepeak_code = "\n\
			if(weight <=weights.a){}\n\
			else if(weight >weights.r)\n\
			{weights = float4(weight, weights.rgb); Orientations = float4(th, Orientations.rgb);}\n\
			else if(weight >weights.g)\n\
			{weights.gba = float3(weight, weights.gb); Orientations.gba = float3(th, Orientations.gb);}\n\
			else if(weight >weights.b)\n\
			{weights.ba = float2(weight, weights.b); Orientations.ba = float2(th, Orientations.b);}\n\
			else {weights.a = weight; Orientations.a = th;}";
		}

	}else
	{
		out<<"\n\
		float Orientation;				";
		testpeak_code ="\n\
			if(npeaks<=0){								\n\
			test = (bins[i] == maxh)	;";
		savepeak_code="\n\
					npeaks++;								\n\
					Orientation = th;";

	}
	//find the peaks
	//the following loop will be unrolled anyway in fp40,
	//taking more than 1000 instrucsions..
	//....
	out<<"\n\
		float hh = maxh * 0.8;	bool4 test;	\n\
		bins[9] = bins[0];								\n\
		int npeaks = 0; k = 0;	float kp=0;				\n\
		float prevb	= bins[8].w;						\n\
		for ( i = 0; i <9 ; i++)						\n\
		{"
		<<testpeak_code<<"									\n\
			if( any ( test ) )							\n\
			{											\n\
				if(test.r && bins[i].x > prevb && bins[i].x > bins[i].y )	\n\
				{											\n\
				    float	di = -0.5 * (bins[i].y-prevb) / (bins[i].y+prevb-bins[i].x - bins[i].x) ; \n\
					float	th = (k+di+0.5);	float weight = bins[i].x;"
					<<savepeak_code<<"\n\
				}\n\
				else if(test.g && all( bins[i].yy > bins[i].xz) )	\n\
				{											\n\
				    float	di = -0.5 * (bins[i].z-bins[i].x) / (bins[i].z+bins[i].x-bins[i].y- bins[i].y) ; \n\
					float	th = (k+di+1.5);	float weight = bins[i].y;				"
					<<savepeak_code<<"	\n\
				}"
		<<"\n\
				if(test.b && all( bins[i].zz > bins[i].yw) )	\n\
				{											\n\
				    float	di = -0.5 * (bins[i].w-bins[i].y) / (bins[i].w+bins[i].y-bins[i].z- bins[i].z) ; \n\
					float	th = (k+di+2.5);	float weight = bins[i].z;				"
					<<savepeak_code<<"	\n\
				}\n\
				else if(test.a && bins[i].w > bins[i].z && bins[i].w > bins[i+1].x )	\n\
				{											\n\
				    float	di = -0.5 * (bins[i+1].x-bins[i].z) / (bins[i+1].x+bins[i].z-bins[i].w - bins[i].w) ; \n\
					float	th = (k+di+3.5);	float weight = bins[i].w;				"
					<<savepeak_code<<"	\n\
				}\n\
			}}\n\
			k = k + 4;						\n\
			prevb = bins[i].w;\n\
		}";
	//WRITE output
	if(GlobalUtil::_MaxOrientation>1)
	{
	out<<"\n\
		 npeaks = dot(float4(1,1,1,1), float4(weights>0));\n\
		 npeaks = min(npeaks, float( "<<min(4, GlobalUtil::_MaxOrientation)<<"));\n\
		 OrientationData = radians((Orientations )*10.0);\n\
		 FeatureData = float4(pos, npeaks>4?0:npeaks, sigma);";
	}else
	{
	out<<"\n\
		 FeatureData = float4(pos, radians((Orientation)*10.0), sigma);";
	}
	//end
	out<<"\n\
	}\n"<<'\0';

	ProgramCG * program;
	s_orientation = program = new ProgramCG(buffer);
	_param_orientation_gtex = cgGetNamedParameter(*program, "gradTex");
	_param_orientation_size = cgGetNamedParameter(*program, "size");
	_param_orientation_stex = cgGetNamedParameter(*program, "texS");
}


void ShaderBagCG::LoadListReshapeShaders()
{
	ProgramCG * program;

	s_list_reshape_init = program = new ProgramCG(
	"void main (\n\
	uniform samplerRECT tex, in float4 TexCoord0 : TEXCOORD0,\n\
	in float4 TexCoord1 : TEXCOORD1, in float4 TexCoord2 : TEXCOORD2, in float4 TexCoord3 : TEXCOORD3,\n\
	out float4 FragColor : COLOR0){\n\
	FragColor= float4( texRECT(tex, TexCoord0.xy).b,  texRECT(tex, TexCoord1.xy).b,\n\
	texRECT(tex, TexCoord2.xy).b, texRECT(tex, TexCoord3.xy).b);}");


	s_list_reshape_start = program =  new ProgramCG(
	"void main(out float4 FragColor : COLOR0,  uniform float width, \nin float2 tpos : TEXCOORD0){\n\
	float  index = floor(tpos.y) * width + floor(tpos.x);\n\
	float2 pos = float2(0.5, 0.5);\n\
	FragColor = float4(pos, index, 1);}\n");
	_param_reshapelist_start_width= cgGetNamedParameter(*program, "width");



	s_list_reshape_step = program = new ProgramCG(
	"void main(out float4 FragColor : COLOR0, uniform samplerRECT tex, \n\
	uniform samplerRECT tex0, uniform float2 stepd, \nin float2 tpos : TEXCOORD0){\n\
	float4 tc = texRECT( tex, tpos);\n\
	float2 pos = tc.rg; float index = tc.b;\n\
	float2 sum; 	float4 cc;\n\
	float4 cpos = float4(0, 1, 2, 3);\t float opos;\n\
	cc = texRECT(tex0, pos);\n\
	if (index <cc.r){ opos = cpos.x;}\n\
	else{sum.x = cc.r + cc.g;\
	if(index < sum.x ) {opos = cpos.y; index -= cc.r;}\n\
	else{sum.y = sum.x + cc.b;\
	if(index < sum.y ) {opos = cpos.z; index -= sum.x;}\n\
	else {opos = cpos.w; index -= sum.y;}}}\n\
	pos = pos + (3.0*pos -1.5 +opos)*stepd.xy;\n\
	FragColor = float4(pos, index, 1);}");  
	_param_reshapelist_step_d= cgGetNamedParameter(*program, "stepd");
	_param_reshapelist_step_tex0= cgGetNamedParameter(*program, "tex0");


	s_list_reshape_end = program =  new ProgramCG(
	"void main(out float4 FragColor : COLOR0, uniform samplerRECT tex, \n\
	uniform samplerRECT fTex, uniform samplerRECT oTex, in float2 tpos : TEXCOORD0){\n\
	float4 tc = texRECT( tex, tpos);\n\
	float2 pos = tc.rg;\n\
	float4 cc, oo;\n\
	cc = texRECT(fTex, pos);\n\
	oo = texRECT(oTex, pos);\n\
	FragColor.rga = cc.rga;\n\
	FragColor.b = dot(float4(tc.b==float4(0,1,2,3)), oo);}");  
	_param_reshapelist_end_ftex= cgGetNamedParameter(*program, "fTex");
	_param_reshapelist_end_otex= cgGetNamedParameter(*program, "oTex");




}

void ShaderBagCG::SetReshapeListStartWidth(float width)
{
	cgGLSetParameter1f(_param_reshapelist_start_width, width);
}

void ShaderBagCG::SetReshapeListStepParam(int tex0, int dx, int dy)
{
	cgGLSetTextureParameter(_param_reshapelist_step_tex0, tex0);
	cgGLEnableTextureParameter(_param_reshapelist_step_tex0);
	float d[2] = {(float)dx, (float)dy};
	cgGLSetParameter2fv(_param_reshapelist_step_d, d);
}

void ShaderBagCG::SetReshapeListEndParam(int fTex, int oTex)
{
	cgGLSetTextureParameter(_param_reshapelist_end_ftex, fTex);
	cgGLEnableTextureParameter(_param_reshapelist_end_ftex);
	cgGLSetTextureParameter(_param_reshapelist_end_otex, oTex);
	cgGLEnableTextureParameter(_param_reshapelist_end_otex);
}

void ShaderBagCG::SetSimpleOrientationInput(int oTex, float size)
{
	cgGLSetTextureParameter(_param_orientation_gtex, oTex);
	cgGLEnableTextureParameter(_param_orientation_gtex);
	cgGLSetParameter1f(_param_orientation_size, size);
}

void ShaderBagCG::SetFeatureOrientationParam(int gtex, int width, int height, float sigma, int stex, float step)
{
	///
	cgGLSetTextureParameter(_param_orientation_gtex, gtex);	
	cgGLEnableTextureParameter(_param_orientation_gtex);

	if(GlobalUtil::_SubpixelLocalization && stex)
	{
		//specify texutre for subpixel subscale localization
		cgGLSetTextureParameter(_param_orientation_stex, stex);
		cgGLEnableTextureParameter(_param_orientation_stex);
	}

	float size[4];
	size[0] = (float)width;
	size[1] = (float)height;
	size[2] = sigma;
	size[3] = step;
	cgGLSetParameter4fv(_param_orientation_size, size);

}

void ShaderBagCG::SetFeatureDescirptorParam(int gtex, float dwidth, float fwidth,  float width, float height, float sigma)
{
	///
	cgGLSetTextureParameter(_param_descriptor_gtex, gtex);	
	cgGLEnableTextureParameter(_param_descriptor_gtex);

	if(GlobalUtil::_DescriptorFormat ==32)
	{
		dwidth = dwidth /32.0f;
	}
	float dsize[4] ={dwidth, 1.0f/dwidth, fwidth, 1.0f/fwidth};
	cgGLSetParameter4fv(_param_descriptor_dsize, dsize);
	float size[2];
	size[0] = width;
	size[1] = height;
	cgGLSetParameter2fv(_param_descriptor_size, size);

}

//use geometry shader to do this??
void ShaderBagCG::LoadDescriptorShaderG()
{
	const double twopi = 2.0*3.14159265358979323846;
	const double rpi  = 8.0/twopi;
	char buffer[20000];
	ostrstream out(buffer, 20000);
	
	if(GlobalUtil::_verbose)std::cout<<"Use Geometry shaders for Descriptor\n";


	//vertex here is using device coordinate from -1 to 1
	//need to transform
	out<<"\n\
	#define WF "<<GlobalUtil::_DescriptorWindowFactor <<"\n\
	POINT POINT_OUT void main(\n\
	AttribArray<float4> point: TEXCOORD0,		\n\
	uniform samplerRECT tex,				\n\
	uniform	samplerRECT gradTex,			\n\
	uniform float4		dsize,				\n\
	uniform float2		size				)		\n\
	{\n\
		float2 coord; float4 dpos;\n\
		float2 temp1, temp2;\n\
		temp1 = fmod(point[0].xx, dsize.xz);\n\
		temp2 = floor(point[0].xx * dsize.yw) + 0.5;\n\
		coord.x = temp1.y;					coord.y = temp2.y;\n\
		dpos.x = 0.5 + 32*floor(temp1.x);	dpos.y =temp2.x;	\n\
		dpos.zw = float2(0, 1);									\n\
		float2 dim	= size.xy;	//image size			\n\
		float2 pos = texRECT(tex, coord).xy;		\n\
		float  anglef = texRECT(tex, coord).z;		\n\
		float sigma = texRECT(tex, coord).w; \n\
		float spt  = sigma * WF;	//default to be 3*sigma	\n";

#ifdef DESCRIPTOR_DEBUG
	//Descriptor storage
	///
	//write the result to different location of framebuffer
	//write the result to different location of framebuffer
	out<<"\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				emitVertex(mul( glstate.matrix.mvp,dpos) : POSITION, float4(i, j, 0, 1): TEXCOORD0);\n\
				dpos.x+=1.0;\n\
				emitVertex(mul( glstate.matrix.mvp,dpos) : POSITION, point[0] : TEXCOORD0);\n\
				dpos.x+=1.0;\n\
			}\n\
		}\n\
	}\n"<<'\0';


#else
	//Descriptor storage
	out<<"\n\
		float4 DA[4][4];	float4 DB[4][4];					\n\
		for (int i = 0; i < 4; i++)								\n\
		{\n\
			for(int j = 0; j < 4; j++)							\n\
			{\n\
					DA[i][j] = DB[i][j] = float4(0, 0, 0, 0);	\n\
			}\n\
		}";
	//rotation
	out<<"\n\
		float4 cscs, rots;								\n\
		sincos(anglef, cscs.y, cscs.x);					\n\
		cscs.zw = - cscs.xy;							\n\
		rots = cscs /spt;								\n\
		cscs *= spt; \n";
	//decide which part of the grid is by y coord
	out<<"\
		float4 offset, temp; float2 pt;				\n\
		offset = float4(-2.5, -2.5, 2.5, 2.5);		\n\
		float4 offsetpt = float4(-2, -2, -2, -2);	\n\
		temp = cscs.xwyx*offsetpt.xyxy;				\n\
		pt = pos + temp.xz + temp.yw;						\n";

	//get the coordinate region of the rotated rectangle
	out<<"\
		float2 p1, p2, p3, p4;			\n\
		temp = cscs.xwyx*offset.xyxy;				\n\
		p1 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zyzy;				\n\
		p2 = temp.xz + temp.yw;						\n\
		temp = cscs.xwyx*offset.zwzw;				\n\
		p3 =  temp.xz + temp.yw;					\n\
		temp = cscs.xwyx*offset.xwxw;				\n\
		p4 =  temp.xz + temp.yw;					\n\
		float2 pmin = min(min(p1, p2), min(p3, p4));\n\
		float2 pmax = max(max(p1, p2), max(p3, p4));\n\
		float4 sz;	float2 spos;					\n\
		sz.xy = max( pos + pmin , float2(1,1));\n\
		sz.zw = min( pos + pmax , dim -2);		\n\
		sz = floor(sz)+0.5;";
	//get voting for two box



	//get voting for two box
	//25 possibilities
	out<<"\n\
		for(spos.x = sz.x; spos.x <= sz.z;	spos.x+=1.0)				\n\
		{																\n\
			for(spos.y = sz.y; spos.y <= sz.w;	spos.y+=1.0)			\n\
			{															\n\
				float2 diff = spos - pt;								\n\
				temp = rots.xywx * diff.xyxy;\n\
				float2 nxy = (temp.xz + temp.yw); \n\
				if(all(nxy >= float2(-0.5,-0.5) && nxy <= float2(4.5, 4.5)))\n\
				{\n\
					float4 cc = texRECT(gradTex, spos);				\n\
					diff = nxy + offsetpt.xy;								\n\
					float mod = cc.b *  exp(-0.125*dot(diff, diff));	\n\
					float angle = cc.a;					\n\
					float theta = fmod(8+ (anglef - angle)*"<<rpi<<", 8); \n\
					float4 wa, wb; \n\
					float theta1 = floor(theta); \n\
					float weight1 = theta1 + 1.0 - theta; float weight2 = theta - theta1;\n\
					wa = float4(theta1 == float4(0, 1, 2, 3))*weight1 + float4(theta1 == float4(7, 0, 1, 2))*weight2; \n\
					wb = float4(theta1 == float4(4, 5, 6, 7))*weight1 + float4(theta1 == float4(3, 4, 5, 6))*weight2; \n\
					wa*= mod; wb*=mod; \n"
				<<"\
					if(nxy.x < 0.5)\n\
					{\n\
						float weight = 0.5+nxy.x; \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weight2 = weight* (0.5+nxy.y);\n\
							DA[0][0]+= wa * weight2;	DB[0][0]+=wb*weight2;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							weight1 = weight* (1.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[0][0]+= wa * weight1;	DB[0][0]+=wb*weight1;\n\
							DA[1][0]+= wa * weight2;	DB[1][0]+=wb*weight2;\n\
						}else if(nxy.y < 2.5)\n\
						{\n\
							weight1 = weight* (2.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[1][0]+= wa * weight1;	DB[1][0]+=wb*weight1;\n\
							DA[2][0]+= wa * weight2;	DB[2][0]+=wb*weight2;\n\
						}else if(nxy.y < 3.5)\n\
						{\n\
							weight1 = weight* (3.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[2][0]+= wa * weight1;	DB[2][0]+=wb*weight1;\n\
							DA[3][0]+= wa * weight2;	DB[3][0]+=wb*weight2;\n\
						}else\n\
						{\n\
							weight1 = weight * (4.5 - nxy.y);\n\
							DA[3][0]+= wa * weight1;	DB[3][0]+=wb*weight1;\n\
						}\n\
					}"
				<<"else if(nxy.x < 1.5)\n\
					{\n\
						float2 weightx = float2(1.5 - nxy.x , nxy.x - 0.5); \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weightx*= (0.5+nxy.y);						\n\
							DA[0][0]+= wa * weightx.x;	DB[0][0]+=wb*weightx.x;\n\
							DA[0][1]+= wa * weightx.y;	DB[0][1]+=wb*weightx.y;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							float2 weighty = float2(1.5 - nxy.y, nxy.y - 0.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[0][0]+= wa * weights.x;	DB[0][0]+=wb*weights.x;\n\
							DA[0][1]+= wa * weights.y;	DB[0][1]+=wb*weights.y;\n\
							DA[1][0]+= wa * weights.z;	DB[1][0]+=wb*weights.z;\n\
							DA[1][1]+= wa * weights.w;	DB[1][1]+=wb*weights.w;\n\
						}else if(nxy.y < 2.5)\n\
						{\n\
							float2 weighty = float2(2.5 - nxy.y, nxy.y - 1.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[1][0]+= wa * weights.x;	DB[1][0]+=wb*weights.x;\n\
							DA[1][1]+= wa * weights.y;	DB[1][1]+=wb*weights.y;\n\
							DA[2][0]+= wa * weights.z;	DB[2][0]+=wb*weights.z;\n\
							DA[2][1]+= wa * weights.w;	DB[2][1]+=wb*weights.w;\n\
						}else if(nxy.y < 3.5)\n\
						{\n\
							float2 weighty = float2(3.5 - nxy.y, nxy.y - 2.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[2][0]+= wa * weights.x;	DB[2][0]+=wb*weights.x;\n\
							DA[2][1]+= wa * weights.y;	DB[2][1]+=wb*weights.y;\n\
							DA[3][0]+= wa * weights.z;	DB[3][0]+=wb*weights.z;\n\
							DA[3][1]+= wa * weights.w;	DB[3][1]+=wb*weights.w;\n\
						}else\n\
						{\n\
							weightx*= (4.5 - nxy.y);						\n\
							DA[3][0]+= wa * weightx.x;	DB[3][0]+=wb*weightx.x;\n\
							DA[3][1]+= wa * weightx.y;	DB[3][1]+=wb*weightx.y;\n\
						}\n\
					}"
				<<"else if(nxy.x < 2.5)\n\
					{\n\
						float2 weightx = float2(2.5 - nxy.x , nxy.x - 1.5); \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weightx*= (0.5+nxy.y);						\n\
							DA[0][1]+= wa * weightx.x;	DB[0][1]+=wb*weightx.x;\n\
							DA[0][2]+= wa * weightx.y;	DB[0][2]+=wb*weightx.y;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							float2 weighty = float2(1.5 - nxy.y, nxy.y - 0.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[0][1]+= wa * weights.x;	DB[0][1]+=wb*weights.x;\n\
							DA[0][2]+= wa * weights.y;	DB[0][2]+=wb*weights.y;\n\
							DA[1][1]+= wa * weights.z;	DB[1][1]+=wb*weights.z;\n\
							DA[1][2]+= wa * weights.w;	DB[1][2]+=wb*weights.w;\n\
						}else if(nxy.y < 2.5)\n\
						{\n\
							float2 weighty = float2(2.5 - nxy.y, nxy.y - 1.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[1][1]+= wa * weights.x;	DB[1][1]+=wb*weights.x;\n\
							DA[1][2]+= wa * weights.y;	DB[1][2]+=wb*weights.y;\n\
							DA[2][1]+= wa * weights.z;	DB[2][1]+=wb*weights.z;\n\
							DA[2][2]+= wa * weights.w;	DB[2][2]+=wb*weights.w;\n\
						}else if(nxy.y < 3.5)\n\
						{\n\
							float2 weighty = float2(3.5 - nxy.y, nxy.y - 2.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[2][1]+= wa * weights.x;	DB[2][1]+=wb*weights.x;\n\
							DA[2][2]+= wa * weights.y;	DB[2][2]+=wb*weights.y;\n\
							DA[3][1]+= wa * weights.z;	DB[3][1]+=wb*weights.z;\n\
							DA[3][2]+= wa * weights.w;	DB[3][2]+=wb*weights.w;\n\
						}else\n\
						{\n\
							weightx*= (4.5 - nxy.y);						\n\
							DA[3][1]+= wa * weightx.x;	DB[3][1]+=wb*weightx.x;\n\
							DA[3][2]+= wa * weightx.y;	DB[3][2]+=wb*weightx.y;\n\
						}\n\
					}"
				<<"else if(nxy.x < 3.5)\n\
					{\n\
						float2 weightx = float2(3.5 - nxy.x , nxy.x - 2.5); \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weightx*= (0.5+nxy.y);						\n\
							DA[0][2]+= wa * weightx.x;	DB[0][2]+=wb*weightx.x;\n\
							DA[0][3]+= wa * weightx.y;	DB[0][3]+=wb*weightx.y;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							float2 weighty = float2(1.5 - nxy.y, nxy.y - 0.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[0][2]+= wa * weights.x;	DB[0][2]+=wb*weights.x;\n\
							DA[0][3]+= wa * weights.y;	DB[0][3]+=wb*weights.y;\n\
							DA[1][2]+= wa * weights.z;	DB[1][2]+=wb*weights.z;\n\
							DA[1][3]+= wa * weights.w;	DB[1][3]+=wb*weights.w;\n\
						}else if(nxy.y < 2.5)\n\
						{\n\
							float2 weighty = float2(2.5 - nxy.y, nxy.y - 1.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[1][2]+= wa * weights.x;	DB[1][2]+=wb*weights.x;\n\
							DA[1][3]+= wa * weights.y;	DB[1][3]+=wb*weights.y;\n\
							DA[2][2]+= wa * weights.z;	DB[2][2]+=wb*weights.z;\n\
							DA[2][3]+= wa * weights.w;	DB[2][3]+=wb*weights.w;\n\
						}else if(nxy.y < 3.5)\n\
						{\n\
							float2 weighty = float2(3.5 - nxy.y, nxy.y - 2.5);\n\
							float4 weights = weightx.xyxy * weighty.xxyy; \n\
							DA[2][2]+= wa * weights.x;	DB[2][2]+=wb*weights.x;\n\
							DA[2][3]+= wa * weights.y;	DB[2][3]+=wb*weights.y;\n\
							DA[3][2]+= wa * weights.z;	DB[3][2]+=wb*weights.z;\n\
							DA[3][3]+= wa * weights.w;	DB[3][3]+=wb*weights.w;\n\
						}else\n\
						{\n\
							weightx*= (4.5 - nxy.y);						\n\
							DA[3][2]+= wa * weightx.x;	DB[3][2]+=wb*weightx.x;\n\
							DA[3][3]+= wa * weightx.y;	DB[3][3]+=wb*weightx.y;\n\
						}\n\
					}"
				<<"else\n\
					{\n\
						float weight = 4.5 - nxy.x; \n\
						if(nxy.y <  0.5)\n\
						{\n\
							weight2 = weight* (0.5+nxy.y);\n\
							DA[0][3]+= wa * weight2;	DB[0][3]+=wb*weight2;\n\
						}else if(nxy.y < 1.5)\n\
						{\n\
							weight1 = weight* (1.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[0][3]+= wa * weight1;	DB[0][3]+=wb*weight1;\n\
							DA[1][3]+= wa * weight2;	DB[1][3]+=wb*weight2;\n\
						}else if(nxy.y < 2.5)\n\
						{\n\
							weight1 = weight* (2.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[1][3]+= wa * weight1;	DB[1][3]+=wb*weight1;\n\
							DA[2][3]+= wa * weight2;	DB[2][3]+=wb*weight2;\n\
						}else if(nxy.y < 3.5)\n\
						{\n\
							weight1 = weight* (3.5 - nxy.y);\n\
							weight2 = weight* (nxy.y - 0.5);\n\
							DA[2][3]+= wa * weight1;	DB[2][3]+=wb*weight1;\n\
							DA[3][3]+= wa * weight2;	DB[3][3]+=wb*weight2;\n\
						}else\n\
						{\n\
							weight1 = weight * (4.5 - nxy.y);\n\
							DA[3][3]+= wa * weight1;	DB[3][3]+=wb*weight1;\n\
						}\n\
					}\n\
				}\n\
			}\n\
		}\n";
	//normalize, truncate to 0.2, and normalize again
	out<<"\
		float norm=0;	\n\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				norm+= dot(DA[i][j], DA[i][j]);	\n\
				norm+= dot(DB[i][j], DB[i][j]);	\n\
			}\n\
		}\n\
		norm = rsqrt(norm);			\n\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				DA[i][j] =min(float4(0.2,0.2,0.2, 0.2), DA[i][j]* norm);\n\
				DB[i][j] =min(float4(0.2,0.2,0.2, 0.2), DB[i][j]* norm);\n\
			}\n\
		}\n\
		norm = 0; \n\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				norm+= dot(DA[i][j], DA[i][j]);	\n\
				norm+= dot(DB[i][j], DB[i][j]);	\n\
			}\n\
		}\n\
		norm = rsqrt(norm);			\n\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				DA[i][j] *= norm;\n\
				DB[i][j] *= norm;\n\
			}\n\
		}\n";
	///
	//write the result to different location of framebuffer
	out<<"\
		for (int i = 0; i < 4; i++)\n\
		{\n\
			for(int j = 0; j < 4; j++)\n\
			{\n\
				emitVertex(mul(glstate.matrix.mvp,dpos) : POSITION, DA[i][j] : TEXCOORD0);\n\
				dpos.x+=1.0;\n\
				emitVertex(mul(glstate.matrix.mvp,dpos) : POSITION, DB[i][j] : TEXCOORD0);\n\
				dpos.x+=1.0;\n\
			}\n\
		}\n\
	}\n"<<'\0';
#endif

	ProgramCG * program; 
	s_descriptor_gp = program =  new ProgramCG(buffer, ProgramCG::_GProfile);
	_param_descriptor_gtex = cgGetNamedParameter(*program, "gradTex");
	_param_descriptor_size = cgGetNamedParameter(*program, "size");
	_param_descriptor_dsize = cgGetNamedParameter(*program, "dsize");

	s_descriptor_fp = new ProgramCG(
		"void main(in float4 tex : TEXCOORD0, out float4 FragColor : COLOR0){FragColor = tex;	}");
//	s_descriptor_vp = new ProgramCG(
//		"void main(in float4 pos : POSITION, out float4 position: POSITION, \n\
//		out float4 tex : TEXCOORD0 ){tex = pos; position = pos;	}", 
//		ProgramCG::_VProfile);

}
