////////////////////////////////////////////////////////////////////////////
//	File:		ProgramGLSL.cpp
//	Author:		Changchang Wu
//	Description : GLSL related classes
//		class ProgramGLSL		A simple wrapper of GLSL programs
//		class ShaderBagGLSL		GLSL shaders for SIFT
//		class FilterGLSL		GLSL gaussian filters for SIFT
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
#include "ProgramGLSL.h"
#include "GLTexImage.h"
#include "ShaderMan.h"
#include "FrameBufferObject.h"
#include "math.h"

#ifdef _WIN32  
	#include "imdebug/imdebuggl.h"
	#pragma comment (lib, "../lib/imdebug.lib")
#endif
 

ProgramGLSL::ShaderObject::ShaderObject(int shadertype,  char * source, int filesource)
{


	_type = shadertype; 
	_compiled = 0;


	_shaderID = glCreateShader(shadertype);
	if(_shaderID == 0) return;
	
	if(source)
	{
		char * code;
		int				code_length;
		if(filesource ==0)
		{
			code  = source;
			code_length = strlen(code);
			glShaderSource(_shaderID, 1, (const char **) &code, &code_length);
		}else
		{
			if((code_length= ReadShaderFile(source, code)) ==0) return;
			glShaderSource(_shaderID, 1, (const char **) &code, &code_length);
			delete code;
		}

		glCompileShader(_shaderID);


		CheckCompileLog();
	}




}

int ProgramGLSL::ShaderObject::ReadShaderFile( char *sourcefile,  char*& code )
{
	code = NULL;
	FILE * file;
	int    len=0;

	if(sourcefile == NULL) return 0;

	file = fopen(sourcefile,"rt");
	if(file == NULL) return 0;

	
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	rewind(file);
	if(len >1)
	{
		code = new  char[len+1];
		fread(code, sizeof( char), len, file);
		code[len] = 0;
	}else
	{
		len = 0;
	}

	fclose(file);

	return len;
	
}

void ProgramGLSL::ShaderObject::CheckCompileLog()
{

	GLint status;
	glGetShaderiv(_shaderID, GL_COMPILE_STATUS, &status);
	_compiled = (status ==GL_TRUE);

}

ProgramGLSL::ShaderObject::~ShaderObject()
{
	if(_shaderID)	glDeleteShader(_shaderID);

}

int ProgramGLSL::ShaderObject::IsValidFragmentShader()
{
	return _type == GL_FRAGMENT_SHADER && _shaderID && _compiled;
}

int  ProgramGLSL::ShaderObject::IsValidVertexShader()
{
	return _type == GL_VERTEX_SHADER && _shaderID && _compiled;
}


void ProgramGLSL::ShaderObject::PrintCompileLog(ostream&os)
{
	GLint len = 0;	

	glGetShaderiv(_shaderID, GL_INFO_LOG_LENGTH , &len);
	if(len <=1) return;
	
	char * compileLog = new char[len+1];
	if(compileLog == NULL) return;

	glGetShaderInfoLog(_shaderID, len, &len, compileLog);
	

	os<<"Compile Log\n"<<compileLog<<endl;

	delete compileLog;
}


ProgramGLSL::ProgramGLSL()
{
	_used = 0;
	_linked = 0;
	_programID = glCreateProgram();
}
ProgramGLSL::~ProgramGLSL()
{
	if(_programID)glDeleteProgram(_programID);
}
void ProgramGLSL::AttachShaderObject(ShaderObject &shader)
{
	if(_programID  && shader.IsValidShaderObject()) 
		glAttachShader(_programID, shader.GetShaderID());
}
void ProgramGLSL::DetachShaderObject(ShaderObject &shader)
{
	if(_programID  && shader.IsValidShaderObject()) 
		glDetachShader(_programID, shader.GetShaderID());
}
int ProgramGLSL::LinkProgram()
{
	_linked = 0;

	if(_programID==0) return 0;

	glLinkProgram(_programID);

	CheckLinkLog();

//	GlobalUtil::StartTimer("100 link test");
//	for(int i = 0; i<100; i++) glLinkProgram(_programID);
//	GlobalUtil::StopTimer();

	return _linked;
}

void ProgramGLSL::CheckLinkLog()
{
	GLint status;
	glGetProgramiv(_programID, GL_LINK_STATUS, &status);

	_linked = (status == GL_TRUE);

}


int ProgramGLSL::ValidateProgram()
{
	if(_programID && _linked)
	{
///		GLint status;
//		glValidateProgram(_programID);
//		glGetProgramiv(_programID, GL_VALIDATE_STATUS, &status);
//		return status == GL_TRUE;
		return 1;
	}
	else
		return 0;
}

void ProgramGLSL::PrintLinkLog(std::ostream &os)
{
	GLint len = 0;	

	glGetProgramiv(_programID, GL_INFO_LOG_LENGTH , &len);
	if(len <=1) return;
	
	char* linkLog = new char[len+1];
	if(linkLog == NULL) return;

	glGetProgramInfoLog(_programID, len, &len, linkLog);
	
	linkLog[len] = 0;

	if(strstr(linkLog, "Link successful")==NULL)
		os<<linkLog + (linkLog[0] == ' '? 1:0)<<endl;

	delete linkLog;
}

int ProgramGLSL::UseProgram()
{
	if(ValidateProgram())
	{
		{
			//I got weried problem on my ATI
			//I have to re link the GLSL program, 
			//otherwise it will get wrong result.
			if(_used)
			{
				glLinkProgram(_programID);

			}
			else
				_used = 1;
			
		}
		//The weried problems happens when viewport size changes
		//but the following code does not either..
	/*	if(_used<GlobalUtil::_ViewPortChanges)
		{
				glLinkProgram(_programID);
				_used = GlobalUtil::_ViewPortChanges;
		}*/
		glUseProgram(_programID);

		return true;
	}
	else
	{
		return false;
	}
}

ProgramGLSL::ProgramGLSL( char *frag_source)
{
	_used = 0;
	_linked = 0;
	_programID = glCreateProgram();
	ShaderObject shader(GL_FRAGMENT_SHADER, frag_source);
	shader.PrintCompileLog(std::cout);
	AttachShaderObject(shader);
	LinkProgram();
	PrintLinkLog(std::cout);
	if(!_linked)
	{

		std::cout<<frag_source;
	}
}
ProgramGLSL::ProgramGLSL(char*frag_source, char * vert_source)
{
	_used = 0;
	_linked = 0;
	_programID = glCreateProgram();
	ShaderObject shader(GL_FRAGMENT_SHADER, frag_source);
	shader.PrintCompileLog(std::cout);
	ShaderObject vertex_shader(GL_VERTEX_SHADER, vert_source);
	vertex_shader.PrintCompileLog(std::cout);
	AttachShaderObject(shader);
	AttachShaderObject(vertex_shader);
	LinkProgram();
	if(!_linked)
	{
		PrintLinkLog(std::cout);
		std::cout<<vert_source;
		std::cout<<frag_source;
	}

}



void ProgramGLSL::SetTexParameter(int texUnit)
{
//	GLint loc = glGetUniformLocation(_programID, "tex");
//	glUniform1i(loc, texUnit);
}

void ProgramGLSL::ReLink()
{
	glLinkProgram(_programID);
}




//		out<<"intensity+= "<<texLookUp<<"(tex, vec2( float("<<
//			offset[i] <<") + gl_TexCoord[0].s , gl_TexCoord[0].t)).r * "<<kernel[i]<<";\n";
//		out<<"pos=vec2( float("<<offset[i] <<") + gl_TexCoord[0].s , gl_TexCoord[0].t);\n";
ProgramGPU* FilterGLSL::CreateFilterH(float kernel[], float offset[], int width)
{

	char buffer[10240];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ float intensity = 0.0 ;  vec2 pos;\n";
	if(GlobalUtil::_MultiTexCoord)
	{

		for(int i = 0; i< width; i++)
		{

			if(offset[i]==0.0)
			{

				out<<"float or = "<<texLookUp<<"(tex, gl_TexCoord[0].st).r;\n";
				out<<"intensity+= or * "<<kernel[i]<<";\n";

			}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
			{
				int off = int(offset[i]);
				int ip = off>0? 2*off-1: -2*off; 
				out<<"intensity+= "<<kernel[i]<<"*"<<texLookUp<<"(tex, gl_TexCoord["<<ip<<"].st).r;\n";
			}		
			else
			{
				out<<"pos = gl_TexCoord[0].st + vec2(float("<<offset[i] <<") , 0);\n";
				out<<"intensity+= "<<kernel[i]<<"*"<<texLookUp<<"(tex, pos).r;\n";
			}
		}

	}else
	{
		for(int i = 0; i< width; i++)
		{
			if(offset[i]==0.0)
			{

				out<<"float or = "<<texLookUp<<"(tex, gl_TexCoord[0].st).r;\n";
				out<<"intensity+= or * "<<kernel[i]<<";\n";

			}else
			{
				out<<"pos = gl_TexCoord[0].st + vec2(float("<<offset[i] <<") , 0);\n";
				out<<"intensity+= "<<kernel[i]<<"*"<<texLookUp<<"(tex, pos).r;\n";
			}
		}
	}
	//copy original data to red channel
	out<<"gl_FragColor.r = or;\n"; 
	out<<"gl_FragColor.b  = intensity;}\n"<<'\0';

	return new ProgramGLSL( buffer);
}


ProgramGPU* FilterGLSL::CreateFilterV(float kernel[], float offset[], int height)
{

	char buffer[10240];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ float intensity = 0.0;vec2 pos; \n";
	if(GlobalUtil::_MultiTexCoord)
	{
		for(int i = 0; i< height; i++)
		{

			if(offset[i]==0.0)
			{
	//			out<<"intensity+= "<<texLookUp<<"(tex, gl_TexCoord[0].st).g * "<<kernel[i]<<";\n";
				out<<"vec2 orb = "<<texLookUp<<"(tex, gl_TexCoord[0].st).rb;\n";
				out<<"intensity+= orb.y * "<<kernel[i]<<";\n";

			}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
			{
				int off = int(offset[i]);
				int ip = off>0? 2*off-1: -2*off; 
				out<<"intensity+= "<<texLookUp<<"(tex, gl_TexCoord["<<ip<<"].st).b * "<<kernel[i]<<";\n";
			}else
			{
				out<<"pos = gl_TexCoord[0].st + vec2(0, float("<<offset[i] <<") );\n";
				out<<"intensity+= "<<texLookUp<<"(tex, pos).b * "<<kernel[i]<<";\n";
			}
			
		}
	}else
	{
		for(int i = 0; i< height; i++)
		{

			if(offset[i]==0.0)
			{
	//			out<<"intensity+= "<<texLookUp<<"(tex, gl_TexCoord[0].st).g * "<<kernel[i]<<";\n";
				out<<"vec2 orb = "<<texLookUp<<"(tex, gl_TexCoord[0].st).rb;\n";
				out<<"intensity+= orb.y * "<<kernel[i]<<";\n";

			}else
			{
				out<<"pos = gl_TexCoord[0].st + vec2(0, float("<<offset[i] <<") );\n";
				out<<"intensity+= "<<texLookUp<<"(tex, pos).b * "<<kernel[i]<<";\n";
			}
			
		}
	}
	out<<"gl_FragColor.b = orb.y;\n";
	out<<"gl_FragColor.g = intensity - orb.x;\n"; // difference of gaussian..
	out<<"gl_FragColor.r = intensity;}\n"<<'\0';
	
//	std::cout<<buffer<<endl;
	return new ProgramGLSL( buffer);
}


//PACKED VERSION
ProgramGPU* FilterPKSL::CreateFilterH(float kernel[], float offset[], int width)
{
	return GlobalUtil::_usePackedTex ==2 ?
		CreateFilterH2(kernel, offset, width):
		CreateFilterH1(kernel, offset, width);

}
//PACKED VERSION
ProgramGPU* FilterPKSL::CreateFilterV(float kernel[], float offset[], int height)
{
	return GlobalUtil::_usePackedTex ==2 ?
		CreateFilterV2(kernel, offset, height):
		CreateFilterV1(kernel, offset, height);
}

ProgramGPU* FilterPKSL::CreateFilterH1(float kernel[], float offset[], int width)
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
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ gl_FragColor = vec4(0, 0, 0, 0); vec2 pos;\n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<" vec4 sum4, pc;\n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord["<<ip<<"].xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord[0].xy + vec2(float("<<i-nhpixel<<"),0));\n";
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
		out<<"gl_FragColor +=  mat4(\n";
		for( j = 0; j < 15; j ++)
		{
			out<<weight[0][j]<<",\t";
			if(j%4 ==3) out<<endl;
		}
		out<<weight[0][15]<<")*pc;\n";
	}
	out<<"}\n"<<'\0';
	
	return new ProgramGLSL( buffer);
}
ProgramGPU* FilterPKSL::CreateFilterH2(float kernel[], float offset[], int width)
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
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ gl_FragColor = vec4(0, 0, 0, 0);\n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<"vec4 pc; \n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord["<<ip<<"].xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord[0].xy + vec2(float("<<i-nhpixel<<"),0));\n";
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
	//	out<<"gl_FragColor.rg +=  mat2("<<weight[0][0]<<","<<weight[0][1]<<","<<weight[1][0]<<","<<weight[1][1]<<")*pc.rg;\n";
	//	out<<"gl_FragColor.ba +=  mat2("<<weight[0][0]<<","<<weight[0][1]<<","<<weight[1][0]<<","<<weight[1][1]<<")*pc.ba;\n";
		if(weight[0][0]!=0.0)	out<<"gl_FragColor += "<<weight[0][0]<<"*pc;\n";
		out<<"gl_FragColor += vec4("<<weight[0][1]<<","<<weight[1][0]<<","<<weight[0][1]<<","<<weight[1][0]<<")*pc.grab;\n";
	}
	out<<"}\n"<<'\0';
//	std::cout<<buffer<<endl;
	
	return new ProgramGLSL( buffer);


}

ProgramGPU* FilterPKSL::CreateFilterV1(float kernel[], float offset[], int height)
{
	char buffer[10240];
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ gl_FragColor = vec4(0, 0, 0, 0);vec2 pos ;\n\
		pos.x = gl_TexCoord[0].x;\n";
	for(int i = 0; i< height; i++)
	{
		if(offset[i]==0.0)
		{
			out<<"gl_FragColor += "<<texLookUp<<"(tex, gl_TexCoord[0].xy) * "<<kernel[i]<<";\n";
		}else if(offset[i]>=-3.0f && offset[i]<=4.0f)
		{
			int off = int(offset[i]);
			int ip = off>0? 2*off-1: -2*off; 
			out<<"gl_FragColor += "<<texLookUp<<"(tex, gl_TexCoord["<< ip <<"].xy) * "<<kernel[i]<<";\n";
		}else
		{
			out<<"pos.y = gl_TexCoord[0].y +  float("<<offset[i] <<");\n";
			out<<"gl_FragColor += "<<texLookUp<<"(tex, pos) * "<<kernel[i]<<";\n";
		}
	
	}
	out<<"}"<<'\0';
//	std::cout<<buffer<<endl;
	return new ProgramGLSL( buffer);
}
ProgramGPU* FilterPKSL::CreateFilterV2(float kernel[], float offset[], int height)
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
	char * texLookUp   = "texture2DRect" ;
	out<<  "uniform sampler2DRect tex;";
	out<< "\nvoid main(void){ gl_FragColor = vec4(0, 0, 0, 0);\n";
	///use multi texture coordinate because nhpixels can be at most 3
	out<<"vec4 pc;\n";
	for( i = 0 ; i < npixel ; i++)
	{
		if(i - nhpixel >=-3 && i -nhpixel <=4)
		{
			di = i - nhpixel;
			ip =  di >0 ? ( 2*di  -1) : ( - 2*di); 
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord["<<ip<<"].xy);\n";
		}
		else 
		{
			out<<"pc="<<texLookUp<<"(tex, gl_TexCoord[0].xy + vec2(0, float("<<i-nhpixel<<")));\n";
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
//		out<<"gl_FragColor.rb +=  mat2("<<weight[0][0]<<","<<weight[0][1]<<","<<weight[1][0]<<","<<weight[1][1]<<")*pc.rb;\n";
//		out<<"gl_FragColor.ga +=  mat2("<<weight[0][0]<<","<<weight[0][1]<<","<<weight[1][0]<<","<<weight[1][1]<<")*pc.ga;\n";
		if(weight[0][0]!=0.0)	out<<"gl_FragColor += "<<weight[0][0]<<"*pc;\n";
		out<<"gl_FragColor += vec4("<<weight[0][1]<<","<<weight[1][0]<<","<<weight[0][1]<<","<<weight[1][0]<<")*pc.brag;\n";
	}
	out<<"}\n"<<'\0';
//	std::cout<<buffer<<endl;
	
	return new ProgramGLSL( buffer);
}




ProgramGPU* FilterGLSL::CreateTex2DFilterH(float kernel[], float offset[], int width)
{

	char buffer[10240];
	int half_size = width/2;
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texture2D";
	out<< "uniform sampler2D tex;";
	out<< "uniform float offset["<<half_size<<"];\n";
	out<< "void main(void){ float intensity = 0.0 ;  vec2 pos;\n";
	for(int i = 0; i< width; i++)
	{
		if(i == half_size)
		{
			out<<"float or = "<<texLookUp<<"(tex, gl_TexCoord[0].st).r;\n";
			out<<"intensity+= or * "<<kernel[i]<<";\n";

		}else
		{
			if( i > half_size)
				out<<"pos = gl_TexCoord[0].st + vec2(offset["<<(i-half_size-1)<<"], 0);\n";
			else
				out<<"pos = gl_TexCoord[0].st - vec2(offset["<<(half_size-i -1)<<"], 0);\n";
			out<<"intensity+= "<<kernel[i]<<"*"<<texLookUp<<"(tex, pos).r;\n";
		}
	}
	//copy original data to red channel
	out<<"gl_FragColor.r = or;\n"; 
	out<<"gl_FragColor.b  = intensity;}\n"<<'\0';

	return new ProgramGLSL( buffer);
}

ProgramGPU* FilterGLSL::CreateTex2DFilterV(float kernel[], float offset[],  int height)
{
	char buffer[10240];
	int half_size = height/2;
	ostrstream out(buffer, 10240);
	char * texLookUp   = "texture2D";
	out<< "uniform sampler2D tex;";
	out<< "uniform float offset["<<half_size<<"];\n";
	out<< "void main(void){ float intensity = 0.0 ;  vec2 pos;\n";
	for(int i = 0; i< height; i++)
	{
		if(i == half_size)
		{
			out<<"vec2 orb = "<<texLookUp<<"(tex, gl_TexCoord[0].st).rb;\n";
			out<<"intensity+= orb.y * "<<kernel[i]<<";\n";

		}else
		{
			if( i > half_size)
				out<<"pos = gl_TexCoord[0].st + vec2(0, offset["<<(i-half_size-1)<<"]);\n";
			else
				out<<"pos = gl_TexCoord[0].st - vec2(0, offset["<<(half_size-i -1)<<"]);\n";
			out<<"intensity+= "<<texLookUp<<"(tex, pos).b * "<<kernel[i]<<";\n";
		}	
	}
	out<<"gl_FragColor.b = orb.y;\n";
	out<<"gl_FragColor.g = intensity - orb.x;\n"; // difference of gaussian..
	out<<"gl_FragColor.r = intensity;}\n"<<'\0';
	
	return new ProgramGLSL( buffer);
}





void FilterGLSL::UnloadProgram()
{
	glUseProgram(0);
}

void ProgramGLSL::SetTexParameter(unsigned int texID)
{

}

void ShaderBagPKSL::LoadUpSampleShader(int scale)
{
	//
	if(scale ==2)
	{
		char * 	upsample_code = "uniform sampler2DRect tex; void main(void){\n\
			 vec4 pc = texture2DRect(tex,gl_TexCoord[0].st );\n\
			 gl_FragColor = fract(gl_TexCoord[0].s)<0.5? pc.rrgg:pc.bbaa;}";//
		s_upsampling = new ProgramGLSL(upsample_code);
	}else
	{
		//must have (scale %4 ==0);

		char * 	upsample_code = "uniform sampler2DRect tex; void main(void){\
		 vec4 pc = texture2DRect(tex,gl_TexCoord[0].st );\n\
		 float ff = fract(gl_TexCoord[0].s);\n\
		 gl_FragColor = ff<0.5? (ff<0.25?pc.rrrr:pc.gggg):(ff<0.75?pc.bbbb:pc.aaaa);}";
		s_upsampling = new ProgramGLSL(upsample_code);
	}


}

void ShaderBagPKSL::LoadDownSampleShader(int scale)
{
	if(s_downsampling ) delete s_downsampling;
	//
	if(scale ==2)
	{
		char * 	downsample_code = "uniform sampler2DRect tex; void main(void){\n\
			 vec4 pc1 = texture2DRect(tex,gl_TexCoord[0].st );\n\
			 vec4 pc2 = texture2DRect(tex, gl_TexCoord[1].st);\
			 gl_FragColor = vec4(pc1.rb, pc2.rb);}";//
		s_downsampling = new ProgramGLSL(downsample_code);
	}else
	{
		//must have (scale %4 ==0);
		char * downsample_code = "uniform sampler2DRect tex; void main(void){\
		 gl_FragColor = vec4( texture2DRect(tex,gl_TexCoord[0].st ).r,\
			texture2DRect(tex,gl_TexCoord[1].st ).r,\
			texture2DRect(tex,gl_TexCoord[2].st ).r, \
			texture2DRect(tex,gl_TexCoord[2].st ).r);}";
		s_downsampling = new ProgramGLSL(downsample_code);
	}
}


void ShaderBagGLSL::LoadFixedShaders()
{

	char vertex_shader_code[]="uniform sampler2DRect tex; void main(void){ gl_Position = ftransform(); \
		gl_Color = textue2DRect(tex, gl_TexCoord[0].st); \
		gl_TexCoord[0].st = gl_Vertex.xy;}";


	char * display_r_code=
		"uniform sampler2DRect tex; void main(void){float r = texture2DRect(tex, gl_TexCoord[0].st).r;\
		gl_FragColor = vec4(r, r, r, 1);}";


	s_display_gaussian =  new ProgramGLSL( display_r_code);




	char *rgb2gray_code =
		"uniform sampler2DRect rgbTex; void main(void){\
		 float intensity = dot(vec3(0.30, 0.59, 0.11), texture2DRect(rgbTex,gl_TexCoord[0].st ).rgb);\
		 gl_FragColor = vec4(intensity, intensity, intensity, 1.0);}";//

	s_gray = new ProgramGLSL( rgb2gray_code);

	//
	s_debug = new ProgramGLSL( "void main(void){gl_FragColor.rg =  gl_TexCoord[0].st;}");
/*	s_debug = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		float c = texture2DRect(tex, gl_TexCoord[0].st).r;\n\
		float it[3]; int idx = int(c);\n\
		it[idx] = 1.0; gl_FragColor.r = float(it[2]); }\n");*/

	char* copy_rg_code=
		"uniform sampler2DRect tex; void main(void){gl_FragColor.rg= texture2DRect(tex, gl_TexCoord[0].st).rg;}";//gl_FragColor.r = texture2DRect(tex, gl_TexCoord[0].st).r;


	s_downsampling = new ProgramGLSL(copy_rg_code);


	s_copy_key = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(){\n\
	gl_FragColor.rg= texture2DRect(tex, gl_TexCoord[0].st).rg; gl_FragColor.ba = vec2(0.0,1.0);	}");
	


	s_texcoord = new ProgramGLSL("void main(){gl_FragColor= gl_TexCoord[0];}");
	
	//circum extream 
	//find the extrema of the surrounding 8 pixels
	//should be able to use multi-texcoord later...limination is 8 on my laptop...

	char * circum_extrema_code =
		"uniform sampler2DRect tex; \
		void main(void){\
		vec4 g1= vec4(	texture2DRect(tex, gl_TexCoord[0].st + vec2(-1.0, -1.0)).g, \
					texture2DRect(tex, gl_TexCoord[0].st + vec2(0.0   , -1.0)).g,\
					texture2DRect(tex, gl_TexCoord[0].st + vec2( 1.0, -1.0)).g, \
					texture2DRect(tex, gl_TexCoord[0].st + vec2( 1.0,    0)).g);\
		vec4 g2= vec4(	texture2DRect(tex, gl_TexCoord[0].st + vec2( 1.0, 1.0)).g, \
					texture2DRect(tex, gl_TexCoord[0].st + vec2(0.0   ,  1.0)).g,\
					texture2DRect(tex, gl_TexCoord[0].st + vec2(-1.0,  1.0)).g, \
					texture2DRect(tex, gl_TexCoord[0].st + vec2(-1.0,    0.0)).g);\
		vec4 g3 = max(g1, g2);\
		vec2 g4 = max(g3.xy, g3.zw);\
		float cmax = max(g4.x, g4.y);\
		vec4 g5 = min(g1, g2);\
		vec2 g6 = min(g5.xy, g5.zw);\
		float cmin = min(g6.x, g6.y);\
		vec2 rg = texture2DRect(tex, gl_TexCoord[0].st).rg;\
		gl_FragColor = vec4(rg, cmax, cmin);\
		}";
	//here rg is put back for consistensy if it is reading and writing a same texture
	//..now all the four rgba chanels are used up...
	//r for intensity, g for difference of gaussian, ba for cmax and cmin

	s_circum_extrema = new ProgramGLSL(circum_extrema_code);

	//setting the depth

	s_dog_zpass = new ProgramGLSL(
	"uniform sampler2DRect tex; void main(void){\n\
		 gl_FragDepth = texture2DRect(tex, gl_TexCoord[0].st).r;}");

	s_display_dog =  new ProgramGLSL(
	"uniform sampler2DRect tex; void main(void){float g = 0.5+(5.0*texture2DRect(tex, gl_TexCoord[0].st).g);\
		gl_FragColor = vec4(g, g, g, 0.0);}" );

	s_display_gradx = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		vec4 cc = texture2DRect(tex, gl_TexCoord[0].st);\n\
		float gx = 5.0*cc.b * (cos(cc.a))+0.5;\n\
		gl_FragColor = vec4(gx, gx, gx, 1.0);}");


	s_display_grady = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		vec4 cc = texture2DRect(tex, gl_TexCoord[0].st);\n\
		float gy = 5.0*cc.b * (sin(cc.a))+0.5;\n\
		gl_FragColor = vec4(gy, gy, gy, 1.0);}");

	s_display_abs_dog =  new ProgramGLSL(
	"uniform sampler2DRect tex; void main(void){float g = 10.0*abs(texture2DRect(tex, gl_TexCoord[0].st).g);\
		gl_FragColor = vec4(g, g, g, 0.0);}" );

	s_display_abs_gradx = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		vec4 cc = texture2DRect(tex, gl_TexCoord[0].st);\n\
		float gx = 10.0*abs(cc.b * (cos(cc.a)));\n\
		gl_FragColor = vec4(gx, gx, gx, 1.0);}");


	s_display_abs_grady = new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		vec4 cc = texture2DRect(tex, gl_TexCoord[0].st);\n\
		float gy = 10.0*abs(cc.b * (sin(cc.a)));\n\
		gl_FragColor = vec4(gy, gy, gy, 1.0);}");


	s_display_keys= new ProgramGLSL(
		"uniform sampler2DRect tex; void main(void){\n\
		vec4 cc = texture2DRect(tex, gl_TexCoord[0].st);\n\
		if(cc.r ==0.0) discard; gl_FragColor =  (cc.r==1.0? vec4(1.0, 0.0, 0,1.0):vec4(0.0,1.0,0.0,1.0));}");	


	ProgramGLSL * program;
	s_vertex_list = program = new ProgramGLSL(
	"uniform vec4 sizes; uniform sampler2DRect tex;\n\
	void main(void){\n\
	float fwidth = sizes.y; float twidth = sizes.z; float rwidth = sizes.w; \n\
	float size = sizes.x; \n\
	float index = 0.1*(fwidth*floor(gl_TexCoord[0].y) + gl_TexCoord[0].x);\n\
	float px = mod(index, twidth);\n\
	vec2 tpos= floor(vec2(px, index*rwidth))+0.5;\n\
	vec4 cc = texture2DRect(tex, tpos );\n\
	gl_FragColor.zw = vec2(0.0, 1.0);\n\
	if(cc.x<=0 || cc.y <=0) {gl_FragColor.xy = cc.xy; }\n\
	else {float type = fract(px);\n\
	vec2 dxy; \n\
	dxy.x = type < 0.1 ? 0.0 : ((type <0.5 || type > 0.9)? size : -size);\n\
	dxy.y = type < 0.2 ? 0.0 : ((type < 0.3 || type > 0.7 )? -size :size); \n\
	float s = sin(cc.b); float c = cos(cc.b); \n\
	gl_FragColor.x = cc.x + c*dxy.x-s*dxy.y;\n\
	gl_FragColor.y = cc.y + c*dxy.y+s*dxy.x;}\n}\n");


	_param_genvbo_size = glGetUniformLocation(*program, "sizes");


	s_margin_copy = program = new ProgramGLSL(
	"uniform sampler2DRect tex; uniform vec2 truncate;\n\
	void main(){ gl_FragColor = texture2DRect(tex, min(gl_TexCoord[0].xy, truncate)); }");

	_param_margin_copy_truncate = glGetUniformLocation(*program, "truncate");

	s_orientation = program =  new ProgramGLSL(
	"uniform sampler2DRect fTex; uniform sampler2DRect oTex;\n\
	float size; void main(){\n\
	vec4 cc = texture2DRect(fTex, gl_TexCoord[0].st);\n\
	vec4 oo = texture2DRect(oTex, cc.rg);\n\
	gl_FragColor.rg = cc.rg;\n\
	gl_FragColor.b = oo.a;\n\
	gl_FragColor.a = size;}");  

	_param_orientation_gtex = glGetUniformLocation(*program, "oTex");
	_param_orientation_size = glGetUniformLocation(*program, "size");
}

void ShaderBagGLSL::LoadDogShader1(float threshold, float edge_threshold)
{
	//test dog threshold
	//also compute dx, dy and compare dog with 4 neighbour points
	char  buffer2[10240];
	threshold*=0.8f;
	
	{
		//texcoord order ..c, cl, cr, uc, ud, 
		char * buffer1 = 
		"uniform sampler2DRect tex; void main (void ){\n\
		vec4 v1, v2, r4, vt;\n\
		vec4 c  = texture2DRect(tex, gl_TexCoord[0].xy);\n\
		vt = texture2DRect(tex, gl_TexCoord[1].xy);\n\
		v1.x = vt.g;		r4.x = vt.r;\n\
		vt = texture2DRect(tex, gl_TexCoord[2].xy);\n\
		v1.y = vt.g;		r4.y = vt.r;\n\
		vt = texture2DRect(tex, gl_TexCoord[3].xy);\n\
		v1.z = vt.g;		r4.z = vt.r;\n\
		vt = texture2DRect(tex, gl_TexCoord[4].xy);\n\
		v1.w = vt.g;		r4.w = vt.r;\n\
		v2.x = texture2DRect(tex, gl_TexCoord[5].xy).g;\n\
		v2.y = texture2DRect(tex, gl_TexCoord[6].xy).g;\n\
		v2.z = texture2DRect(tex, gl_TexCoord[7].xy).g;\n\
		v2.w = texture2DRect(tex, vec2(gl_TexCoord[2].x, gl_TexCoord[4].y)).g;\n\
		vec2 dxy = r4.yw - r4.xz; \n\
		float grad = length(dxy);\n\
		float theta = atan(dxy.y, dxy.x);\n\
		vec4 mmax4 = max(v1, v2);\n\
		vec4 mmin4 = min(v1, v2);\n\
		vec2 mmax2 = max(mmax4.xy, mmax4.zw);\n\
		vec2 mmin2 = min(mmin4.xy, mmin4.zw);\n\
		float mmax = max(mmax2.x, mmax2.y);\n\
		float mmin = min(mmin2.x, mmin2.y);\n";

		sprintf(buffer2, "%s\
		bool gt = (c.g > max(mmax, %f)); \n\
		bool lt = (c.g < min(mmin, %f)); \n\
		float dog = float(gt) + float(lt)*0.5;\n\
		gl_FragData[0] = vec4(c.rg, grad*0.5, theta);\n\
		gl_FragData[1] = vec4(dog, c.g, max(c.g, mmax), min(c.g,mmin));\n}\n",	
		buffer1, threshold, -threshold);
		s_dog_pass1 = new ProgramGLSL(buffer2);
//		float dog = gt.x && gt.y ?  1.0 : ((lt.x && lt.y) ? 0.5:0.0); \n
/*
		bool gt1 = c.g > mmax;	bool lt1 = c.g < mmin;\n\
		gl_FragData[0] = vec4(c.rg, grad*0.5, theta);\n\
		float dog = (gt1 && c.g > %f)? 1.0f : 0.0f;\n\
		gl_FragData[1] = vec4(dog, c.g, max(c.g,mmax), min(c.g,mmin));\n}\n",

		bvec2 gt = greaterThan(c.gg , vec2(mmax, %f)); \n\
		bvec2 lt = lessThan(c.gg , vec2(mmin, %f)); \n\
		gl_FragData[0] = vec4(c.rg, grad*0.5, theta);\n\
		float dog = all(gt)?1.0f: (all(lt)?0.5f:0.0f);\n\
		gl_FragData[1] = vec4(dog, c.g, (gt.x? c.g:mmax), (lt.x?c.g:mmin));\n}\n",	
		*/



		sprintf(buffer2, "%s\
		gl_FragData[0] = vec4(c.rg, grad*0.5, theta);\n\
		gl_FragData[1] = vec4(0, c.g, max(c.g, mmax), min(c.g, mmin));\n}\n", buffer1);

		s_grad_pass = new ProgramGLSL(buffer2);

	}
}

void ShaderBagGLSL::SetDogTexParam(int texU, int texD)
{
	glUniform1i(_param_dog_texu, 1);
	glUniform1i(_param_dog_texd, 2);
}

void ShaderBagGLSL::SetGenListStepParam(int tex, int tex0)
{
	glUniform1i(_param_genlist_step_tex0, 1);	
}
void ShaderBagGLSL::SetGenVBOParam( float width, float fwidth,  float size)
{
	float sizes[4] = {size*3.0f, fwidth, width, 1.0f/width};
	glUniform4fv(_param_genvbo_size, 1, sizes);

}
void ShaderBagGLSL::LoadDogShader2(float threshold, float edgeThreshold)
{
	ProgramGLSL * program;
	//in pass two...
	char buffer[10240];
	ostrstream out(buffer, 10240);
	//check 8 surrouning points...every one have 3 data
	
	{

	out<<"uniform sampler2DRect tex, texU, texD; void main(void){\n\
	vec4 cc = texture2DRect(tex , gl_TexCoord[0].xy);\n\
	vec4 uc = texture2DRect(texU, gl_TexCoord[0].xy);\n\
	vec4 dc = texture2DRect(texD, gl_TexCoord[0].xy);\n\
	gl_FragColor.r = (\n\
	(cc.r == 1.0 && all(greaterThan(cc.gg, vec2(uc.b, dc.b)))) ||\n\
	(cc.r == 0.5 && all(lessThan(cc.gg, vec2(uc.a, dc.a)))) )?\n\
	cc.r : 0.0; \n\
	gl_FragColor.gba = cc.gba;\n";
	out<<"\n}\n"<<'\0';

	}

	s_dog_pass2  = program = new ProgramGLSL(buffer);


	//parameter
	_param_dog_texu = glGetUniformLocation(program->GetProgramID(), "texU");
	_param_dog_texd = glGetUniformLocation(program->GetProgramID(), "texD");



}



void ShaderBagPKSL::LoadFixedShader()
{
	char * display_pk_code = 
	"uniform sampler2DRect tex; void main(void){\
		 vec4 pc = texture2DRect(tex,gl_TexCoord[0].st );\n\
		 float ff = fract(gl_TexCoord[0].s);\n\
		 gl_FragColor.rgb = ff<0.5? (ff<0.25?pc.rrr:pc.ggg):(ff<0.75?pc.bbb:pc.aaa);\n\
		 gl_FragColor.a = 1.0;}";
	s_display_gaussian = new ProgramGLSL(display_pk_code);


	char *rgb2gray_packing_code =
		"uniform sampler2DRect rgbTex; const vec3 weight = vec3(0.30, 0.59, 0.11); void main(void){\
		 gl_FragColor.r = dot(weight, texture2DRect(rgbTex,gl_TexCoord[0].st ).rgb);\
		 gl_FragColor.g = dot(weight, texture2DRect(rgbTex,gl_TexCoord[1].st ).rgb);\
		 gl_FragColor.b = dot(weight, texture2DRect(rgbTex,gl_TexCoord[2].st ).rgb);\
		 gl_FragColor.a = dot(weight, texture2DRect(rgbTex,gl_TexCoord[3].st ).rgb);}";//

	s_gray = new ProgramGLSL( rgb2gray_packing_code);
}

void ShaderBagGLSL::LoadDownSampleShader(int scale)
{


}

void ShaderBagGLSL::LoadUpSampleShader(int scale)
{
	
}





void ShaderBagGLSL::UnloadProgram()
{
	glUseProgram(0);
} 



void ShaderBagGLSL::LoadGenListShader(int ndoglev, int nlev)
{
	ProgramGLSL * program;

	s_genlist_init = new ProgramGLSL(
	"uniform sampler2DRect tex; void main (void){\n\
	vec4 helper = vec4( texture2DRect(tex, gl_TexCoord[0].xy).r,  texture2DRect(tex, gl_TexCoord[1].xy).r,\n\
	texture2DRect(tex, gl_TexCoord[2].xy).r, texture2DRect(tex, gl_TexCoord[3].xy).r);\n\
	gl_FragColor = vec4(greaterThan(helper, vec4(0.0,0.0,0.0,0.0)));\n\
	}");

/*
	FragColor.r = float( texRECT(tex, TexCoord0.xy).r >0.0);\n\
	FragColor.g = float( texRECT(tex, TexCoord1.xy).r >0.0);\n\
	FragColor.b = float( texRECT(tex, TexCoord2.xy).r >0.0);\n\
	FragColor.a = float( texRECT(tex, TexCoord3.xy).r >0.0);\n\

*/

	
	s_genlist_init_ex = program = new ProgramGLSL(
	"uniform sampler2DRect tex;uniform vec2 bbox;\n\
	void main (void ){\n\
	vec4 helper = vec4( texture2DRect(tex, gl_TexCoord[0].xy).r,  texture2DRect(tex, gl_TexCoord[1].xy).r,\n\
	texture2DRect(tex, gl_TexCoord[2].xy).r, texture2DRect(tex, gl_TexCoord[3].xy).r);\n\
	bvec4 helper2 = bvec4( \n\
	all(lessThan(gl_TexCoord[0].xy , bbox)) && helper.x >0,\n\
	all(lessThan(gl_TexCoord[1].xy , bbox)) && helper.y >0,\n\
	all(lessThan(gl_TexCoord[2].xy , bbox)) && helper.z >0,\n\
	all(lessThan(gl_TexCoord[3].xy , bbox)) && helper.w >0);\n\
	gl_FragColor = vec4(helper2);\n\
	}");
	_param_genlist_init_bbox = glGetUniformLocation( *program, "bbox");


	//reduction ...
	s_genlist_histo = new ProgramGLSL(
	"uniform sampler2DRect tex; void main (void){\n\
	vec4 helper; vec4 helper2; \n\
	helper = texture2DRect(tex, gl_TexCoord[0].xy); helper2.xy = helper.xy + helper.zw; \n\
	helper = texture2DRect(tex, gl_TexCoord[1].xy); helper2.zw = helper.xy + helper.zw; \n\
	gl_FragColor.rg = helper2.xz + helper2.yw;\n\
	helper = texture2DRect(tex, gl_TexCoord[2].xy); helper2.xy = helper.xy + helper.zw; \n\
	helper = texture2DRect(tex, gl_TexCoord[3].xy); helper2.zw = helper.xy + helper.zw; \n\
	gl_FragColor.ba= helper2.xz+helper2.yw;\n\
	}");


	//read of the first part, which generates tex coordinates 
	s_genlist_start= program =  LoadGenListStepShader(1, 1);
	_param_ftex_width= glGetUniformLocation(*program, "width");
	_param_genlist_start_tex0 = glGetUniformLocation(*program, "tex0");
	//stepping
	s_genlist_step = program = LoadGenListStepShader(0, 1);
	_param_genlist_step_tex= glGetUniformLocation(*program, "tex");
	_param_genlist_step_tex0= glGetUniformLocation(*program, "tex0");

}

void ShaderBagGLSL::SetMarginCopyParam(float xmax, float ymax)
{
	float truncate[2] = {xmax, ymax};
	glUniform2fv(_param_margin_copy_truncate, 1, truncate);
}

void ShaderBagGLSL::SetGenListInitParam(int w, int h)
{
	float bbox[2] = {w - 1.0f, h - 1.0f};
	glUniform2fv(_param_genlist_init_bbox, 1, bbox);
}
void ShaderBagGLSL::SetGenListStartParam(float width, int tex0)
{
	glUniform1f(_param_ftex_width, width);
}


ProgramGLSL* ShaderBagGLSL::LoadGenListStepShader(int start, int step)
{
	int i;
	char buffer[10240];
	char chanels[5] = "rgba";
	ostrstream out(buffer, 10240);

	for(i = 0; i < step; i++) out<<"uniform sampler2DRect tex"<<i<<";\n";
	if(start)
	{
		out<<"uniform float width;\n";
		out<<"void main(void){\n";
		out<<"float  index = floor(gl_TexCoord[0].y) * width + floor(gl_TexCoord[0].x);\n";
		out<<"vec2 pos = vec2(0.5, 0.5);\n";
	}else
	{
		out<<"uniform sampler2DRect tex;\n";
		out<<"void main(void){\n";
		out<<"vec4 tc = texture2DRect( tex, gl_TexCoord[0].xy);\n";
		out<<"vec2 pos = tc.rg; float index = tc.b;\n";
	}
	out<<"vec2 sum; 	vec4 cc;\n";


	if(step>0)
	{
		out<<"vec2 cpos = vec2(-0.5, 0.5);\t vec2 opos;\n";
		for(i = 0; i < step; i++)
		{

			out<<"cc = texture2DRect(tex"<<i<<", pos);\n";
			out<<"sum.x = cc.r + cc.g; sum.y = sum.x + cc.b;  \n";
			out<<"if (index <cc.r){ opos = cpos.xx;}\n";
			out<<"else if(index < sum.x ) {opos = cpos.yx; index -= cc.r;}\n";
			out<<"else if(index < sum.y ) {opos = cpos.xy; index -= sum.x;}\n";
			out<<"else {opos = cpos.yy; index -= sum.y;}\n";
			out<<"pos = (pos + pos + opos);\n";
		}
	}
	out<<"gl_FragColor = vec4(pos, index, 1.0);\n";
	out<<"}\n"<<'\0';
	return new ProgramGLSL(buffer);
}


void ShaderBagGLSL::LoadOrientationShader()
{
	//only cg for this part
}

void ShaderBagGLSL::SetSimpleOrientationInput(int oTex, float size)
{
	glUniform1i(_param_orientation_gtex, 1);
	glUniform1f(_param_orientation_size, size);
}
