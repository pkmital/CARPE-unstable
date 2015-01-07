////////////////////////////////////////////////////////////////////////////
//	File:		GLTexImage.cpp
//	Author:		Changchang Wu
//	Description : implementation of the GLTexImage class.
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
#include "stdlib.h"
#include "GlobalUtil.h"
#include "GLTexImage.h"
#include "IL/il.h"
#include "FrameBufferObject.h"
#include <iostream>
#include <vector>
using namespace std;
#include "ShaderMan.h"

#ifdef _WIN32
	#include "imdebug/imdebuggl.h"
	#pragma comment(lib, "../lib/DevIL.lib")
	#pragma comment(lib, "../lib/glew32.lib")
	#pragma comment(lib, "../lib/glew32s.lib")
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


GLTexImage::GLTexImage()
{
	_imgWidth = _imgHeight = 0;
	_texWidth = _texHeight = 0;
	_texID = 0;

}

GLTexImage::~GLTexImage()
{
	if(_texID) glDeleteTextures(1, &_texID);
}



void GLTexImage::TestDraw()
{

}

GLTexImage::GLTexImage( int width,  int height)
{
	InitTexture(width, height);
}



//set a dimension that is smaller than the actuall size
//for drawQuad
void GLTexImage::SetImageSize( int width,  int height)
{
	_imgWidth =  width;
	_imgHeight =  height;
}

void GLTexImage::InitTexture( int width,  int height, int clamp_to_edge)
{

	if(_texID && width == _texWidth && height == _texHeight ) return;
	if(_texID==0)	glGenTextures(1, &_texID); 

	_imgWidth = width;
	_imgHeight = height;
	SetTextureSize();

	BindTex();

	if(clamp_to_edge)
	{
		glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
		glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	}else
	{
		//out of bound tex read returns 0??
		glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
	}
	glTexParameteri(_texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexImage2D(_texTarget, 0, _iTexFormat, _texWidth, _texHeight, 0, GL_RGBA, GL_FLOAT, NULL); 

	UnbindTex();

}


void  GLTexImage::BindTex()
{
	glBindTexture(_texTarget, _texID);
	CheckErrorsGL("glBindTexture");

}

void  GLTexImage::UnbindTex()
{
	glBindTexture(_texTarget, 0);
}



void  GLTexImage::DrawQuad()
{

	glBegin (GL_QUADS);
		glTexCoord2i ( 0			,   0   ); 				glVertex2i   ( 0			,		0   ); 
		glTexCoord2i ( 0			,   _imgHeight  );		glVertex2i   ( 0			,		_imgHeight   ); 
 		glTexCoord2i ( _imgWidth    ,   _imgHeight  ); 		glVertex2i   ( _imgWidth	,		_imgHeight   ); 
		glTexCoord2i ( _imgWidth	,   0   ); 				glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}

void GLTexImage::FillMargin(int marginx, int marginy)
{
	//
	marginx = min(marginx, _texWidth - _imgWidth);
	marginy = min(marginy, _texHeight - _imgHeight);
	if(marginx >0 || marginy > 0)
	{
		GlobalUtil::FitViewPort(_imgWidth + marginx, _imgHeight + marginy);
		AttachToFBO(0);
		BindTex();
		ShaderMan::UseShaderMarginCopy(_imgWidth - 0.5f, _imgHeight - 0.5f);
		DrawMargin(marginx, marginy);
	//	imdebugTexImagef(GlobalUtil::_texTarget, GetTexID(), GL_RGBA);
	}
}
void GLTexImage::DrawMargin(int marginx, int marginy)
{
	int right = _imgWidth+marginx;
	int bottom = _imgHeight+marginy;
	glBegin(GL_QUADS);
	if(marginx>0)
	{
		glTexCoord2i ( _imgWidth	,   0   ); 				glVertex2i   ( _imgWidth	,		0   ); 
		glTexCoord2i ( _imgWidth	,   bottom  );			glVertex2i   ( _imgWidth	,		bottom   ); 
		glTexCoord2i ( right		,   bottom  ); 			glVertex2i   ( right		,		bottom   ); 
		glTexCoord2i ( right		,   0   ); 				glVertex2i   ( right		,		0   ); 	
	}
	if(marginy>0)
	{
		glTexCoord2i ( 0			,   _imgHeight  ); 		glVertex2i   ( 0			,		_imgHeight   ); 
		glTexCoord2i ( 0			,   bottom		);		glVertex2i   ( 0			,		bottom		 ); 
		glTexCoord2i ( _imgWidth	,   bottom		); 		glVertex2i   ( _imgWidth	,		bottom		 ); 
		glTexCoord2i ( _imgWidth	,   _imgHeight	); 		glVertex2i   ( _imgWidth	,		_imgHeight	 ); 
	}
	glEnd();
	glFlush();


}

void GLTexImage::DrawQuadMT2()
{
	glBegin (GL_QUADS);
		glMultiTexCoord2i( GL_TEXTURE0, 0		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   0  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2i( GL_TEXTURE0, 0		,   _imgHeight  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   _imgHeight ); 
		glVertex2i   ( 0			,		_imgHeight   ); 


		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   _imgHeight  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   _imgHeight  ); 
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 

		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   0  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}

void GLTexImage::DrawQuadMT4()
{
	glBegin (GL_QUADS);
		glMultiTexCoord2i( GL_TEXTURE0, 0		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE3, 0		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, 0		,   1  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2i( GL_TEXTURE0, 0		,   _imgHeight  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   _imgHeight ); 
		glMultiTexCoord2i( GL_TEXTURE3, 0		,   _imgHeight -1 ); 
		glMultiTexCoord2i( GL_TEXTURE4, 0		,   _imgHeight +1 ); 
		glVertex2i   ( 0			,		_imgHeight   ); 


		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   _imgHeight  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   _imgHeight-1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   _imgHeight+1  ); 
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 

		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   1  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}


void GLTexImage::DrawQuadMT8()
{
	glBegin (GL_QUADS);
		glMultiTexCoord2i( GL_TEXTURE0, 0		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE3, 0		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, 0		,   1  ); 
		glMultiTexCoord2i( GL_TEXTURE5, -1		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -1		,   1  ); 
		glMultiTexCoord2i( GL_TEXTURE7, 1		,   -1  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2i( GL_TEXTURE0, 0		,   _imgHeight    ); 	
		glMultiTexCoord2i( GL_TEXTURE1, -1		,   _imgHeight    ); 
		glMultiTexCoord2i( GL_TEXTURE2, 1		,   _imgHeight    ); 
		glMultiTexCoord2i( GL_TEXTURE3, 0		,   _imgHeight  -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, 0		,   _imgHeight  +1  ); 
		glMultiTexCoord2i( GL_TEXTURE5, -1		,   _imgHeight  -1  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -1		,   _imgHeight  +1  ); 
		glMultiTexCoord2i( GL_TEXTURE7, 1		,   _imgHeight  -1  ); 
		glVertex2i   ( 0			,		_imgHeight   ); 


		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   _imgHeight    ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   _imgHeight    ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   _imgHeight    ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   _imgHeight  -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   _imgHeight  +1  ); 
		glMultiTexCoord2i( GL_TEXTURE5, _imgWidth-1		,   _imgHeight  -1  ); 
		glMultiTexCoord2i( GL_TEXTURE6, _imgWidth-1		,   _imgHeight  +1  ); 
		glMultiTexCoord2i( GL_TEXTURE7, _imgWidth+1		,   _imgHeight  -1  ); 
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 

		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth-1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth+1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   1  ); 
		glMultiTexCoord2i( GL_TEXTURE5, _imgWidth-1		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE6, _imgWidth-1		,   1  ); 
		glMultiTexCoord2i( GL_TEXTURE7, _imgWidth+1		,   -1  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}



void GLTexImage::DrawQuadMTH()
{
	glBegin (GL_QUADS);
		glMultiTexCoord2i( GL_TEXTURE0, 0		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE1, 1		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, -1		,   0  );
		glMultiTexCoord2i( GL_TEXTURE3, 2		,   0  );
		glMultiTexCoord2i( GL_TEXTURE4, -2		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE5, 3		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -3		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE7, 4		,   0  ); 
//		glTexCoord2i ( 0			,   0   ); 
		glVertex2i   ( 0			,		0   ); 


		glMultiTexCoord2i( GL_TEXTURE0, 0		,   _imgHeight ); 
		glMultiTexCoord2i( GL_TEXTURE1, 1		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, -1		,   _imgHeight ); 
		glMultiTexCoord2i( GL_TEXTURE3, 2		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE4, -2		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE5, 3		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -3		,   _imgHeight  ); 	
		glMultiTexCoord2i( GL_TEXTURE7, 4		,   _imgHeight  ); 
//		glTexCoord2i ( 0			,   _imgHeight  );
		glVertex2i   ( 0			,		_imgHeight   ); 


	


		glMultiTexCoord2i( GL_TEXTURE0, 0+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE1, 1+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE2, -1+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE3, 2+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE4, -2+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE5, 3+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -3+_imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE7, 4+_imgWidth		,   _imgHeight  ); 
 //		glTexCoord2i ( _imgWidth    ,   _imgHeight  );
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 




		glMultiTexCoord2i( GL_TEXTURE0, 0+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE1, 1+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE2, -1+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE3, 2+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE4, -2+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE5, 3+_imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE6, -3+_imgWidth		,   0  ); 	
		glMultiTexCoord2i( GL_TEXTURE7, 4+_imgWidth		,   0  ); 
//		glTexCoord2i ( _imgWidth	,   0   ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}


void GLTexImage::DrawQuadMTV()
{
		glBegin (GL_QUADS);
		glMultiTexCoord2i( GL_TEXTURE0,   0, 0		  ); 
		glMultiTexCoord2i( GL_TEXTURE1,   0, 1		  ); 
		glMultiTexCoord2i( GL_TEXTURE2,   0,-1		  ); 
		glMultiTexCoord2i( GL_TEXTURE3,   0, 2		  ); 
		glMultiTexCoord2i( GL_TEXTURE4,   0,-2		  ); 
		glMultiTexCoord2i( GL_TEXTURE5,   0, 3		  ); 
		glMultiTexCoord2i( GL_TEXTURE6,   0,-3		  ); 	
		glMultiTexCoord2i( GL_TEXTURE7,   0, 4		  ); 
//		glTexCoord2i ( 0			,   0   ); 
		glVertex2i   ( 0			,		0   ); 

	


		glMultiTexCoord2i( GL_TEXTURE0,0		,   _imgHeight     ); 
		glMultiTexCoord2i( GL_TEXTURE1,0		,   _imgHeight+ 1  ); 
		glMultiTexCoord2i( GL_TEXTURE2,0		,   _imgHeight -1  ); 
		glMultiTexCoord2i( GL_TEXTURE3,0		,   _imgHeight+ 2  ); 
		glMultiTexCoord2i( GL_TEXTURE4,0		,   _imgHeight -2  ); 
		glMultiTexCoord2i( GL_TEXTURE5,0		,   _imgHeight+ 3  ); 
		glMultiTexCoord2i( GL_TEXTURE6,0		,   _imgHeight -3  ); 
		glMultiTexCoord2i( GL_TEXTURE7,0		,   _imgHeight+ 4  ); 
//		glTexCoord2i ( 0			,   _imgHeight  );
		glVertex2i   ( 0			,		_imgHeight   ); 




		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   _imgHeight  ); 
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth		,   _imgHeight+1  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth		,   _imgHeight-1  ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   _imgHeight+2  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   _imgHeight-2  ); 
		glMultiTexCoord2i( GL_TEXTURE5, _imgWidth		,   _imgHeight+3  ); 
		glMultiTexCoord2i( GL_TEXTURE6, _imgWidth		,   _imgHeight-3  ); 	
		glMultiTexCoord2i( GL_TEXTURE7, _imgWidth		,   _imgHeight+4  ); 
 //		glTexCoord2i ( _imgWidth    ,   _imgHeight  );
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 



		glMultiTexCoord2i( GL_TEXTURE0, _imgWidth		,   0  ); 
		glMultiTexCoord2i( GL_TEXTURE1, _imgWidth		,   1  ); 
		glMultiTexCoord2i( GL_TEXTURE2, _imgWidth		,   -1  ); 
		glMultiTexCoord2i( GL_TEXTURE3, _imgWidth		,   2  ); 
		glMultiTexCoord2i( GL_TEXTURE4, _imgWidth		,   -2  ); 
		glMultiTexCoord2i( GL_TEXTURE5, _imgWidth		,   3  ); 
		glMultiTexCoord2i( GL_TEXTURE6, _imgWidth		,   -3  ); 	
		glMultiTexCoord2i( GL_TEXTURE7, _imgWidth		,   4  ); 
//		glTexCoord2i ( _imgWidth	,   0   ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}

void GLTexImage::DrawImage()
{
	DrawQuad();
}



void GLTexImage::FitTexViewPort()
{
	GlobalUtil::FitViewPort(_imgWidth, _imgHeight);
}

void GLTexImage::FitRealTexViewPort()
{
	GlobalUtil::FitViewPort(_texWidth, _texHeight);
}

void  GLTexImage::AttachToFBO(int i)
{
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, i+GL_COLOR_ATTACHMENT0_EXT, _texTarget, _texID, 0 );
}

void  GLTexImage::DetachFBO(int i)
{
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, i+GL_COLOR_ATTACHMENT0_EXT, _texTarget, 0, 0 );
}


void GLTexImage::DrawQuad(float x1, float x2, float y1, float y2)
{

	glBegin (GL_QUADS);
		glTexCoord2f ( x1	,   y1   ); 	glVertex2f   ( x1	,		y1   ); 
		glTexCoord2f ( x1	,   y2  );		glVertex2f   ( x1	,		y2   ); 
 		glTexCoord2f ( x2   ,   y2  ); 		glVertex2f   ( x2	,		y2   ); 
		glTexCoord2f ( x2	,   y1   ); 	glVertex2f   ( x2	,		y1   ); 
	glEnd ();

	glFlush();
}

void GLTexImage::TexRGB2Gray()
{

	GlobalUtil::FitViewPort(1, 1);
	FitTexViewPort();
	FrameBufferObject fbo;
	AttachToFBO(0);
	ShaderMan::UseShaderRGB2Gray();
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	DrawQuad();
	ShaderMan::UnloadProgram();
	DetachFBO(0);
}

void GLTexImage::DrawQuadDS(int scale)
{
	DrawScaledQuad(float(scale));
}

void GLTexImage::DrawQuadUS(int scale)
{
	DrawScaledQuad(1.0f/scale);
}

void GLTexImage::DrawScaledQuad(float texscale)
{
	if(_debug) CheckFramebufferStatus();	

	////the texture coordinate for 0.5 is to + 0.5*texscale
	float to = 0.5f -0.5f * texscale;
	float tx =  _imgWidth*texscale +to;
	float ty = _imgHeight*texscale +to;
	glBegin (GL_QUADS);
		glTexCoord2f ( to	,   to   ); 	glVertex2i   ( 0			,		0   ); 
		glTexCoord2f ( to	,   ty  );		glVertex2i   ( 0			,		_imgHeight   ); 
 		glTexCoord2f ( tx	,	ty ); 		glVertex2i   ( _imgWidth	,		_imgHeight   ); 
		glTexCoord2f ( tx	,   to   ); 	glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}




void GLTexImage::DrawQuadReduction()
{
	float to = -0.5f;
	float tx = _imgWidth*2 +to;
	float ty = _imgHeight*2 +to;
	glBegin (GL_QUADS);
		glMultiTexCoord2f ( GL_TEXTURE0, to	,	to   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, to	+1,	to   ); 
		glMultiTexCoord2f ( GL_TEXTURE2, to	,	to+1  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, to	+1,	to+1  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, to	,   ty  );	
		glMultiTexCoord2f ( GL_TEXTURE1, to	+1, ty  );	
		glMultiTexCoord2f ( GL_TEXTURE2, to	,   ty +1 );	
		glMultiTexCoord2f ( GL_TEXTURE3, to	+1, ty +1 );	
		glVertex2i   ( 0			,		_imgHeight   ); 

 		glMultiTexCoord2f ( GL_TEXTURE0, tx	,	ty ); 	
 		glMultiTexCoord2f ( GL_TEXTURE1, tx	+1,	ty ); 	
 		glMultiTexCoord2f ( GL_TEXTURE2, tx	,	ty +1); 	
 		glMultiTexCoord2f ( GL_TEXTURE3, tx	+1,	ty +1); 	

		glVertex2i   ( _imgWidth	,		_imgHeight   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, tx	,   to   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, tx	+1, to   ); 
		glMultiTexCoord2f ( GL_TEXTURE2, tx	,   to +1  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, tx	+1, to +1  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();

	glFlush();
}

void GLTexImage::DrawQuadReductionX(int h)
{
	float to = -1.5f;
	float tx = _imgWidth*4 +to;
	glBegin (GL_QUADS);
		glMultiTexCoord2f ( GL_TEXTURE0, to	,	0   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, to	+1,	0   ); 
		glMultiTexCoord2f ( GL_TEXTURE2, to	+2,	0  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, to	+3,	0  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, to	,   (float)h  );	
		glMultiTexCoord2f ( GL_TEXTURE1, to	+1, (float)h  );	
		glMultiTexCoord2f ( GL_TEXTURE2, to	+2, (float)  h );	
		glMultiTexCoord2f ( GL_TEXTURE3, to	+3, (float)h );	
		glVertex2i   ( 0			,		h   ); 

 		glMultiTexCoord2f ( GL_TEXTURE0, tx	,	(float)h ); 	
 		glMultiTexCoord2f ( GL_TEXTURE1, tx	+1,	(float)h ); 	
 		glMultiTexCoord2f ( GL_TEXTURE2, tx	+2,	(float)h); 	
 		glMultiTexCoord2f ( GL_TEXTURE3, tx	+3,	(float)h); 	

		glVertex2i   ( _imgWidth	,		h   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, tx	,   0   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, tx	+1, 0   ); 
		glMultiTexCoord2f ( GL_TEXTURE2, tx	+2,   0  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, tx	+3, 0  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}

void GLTexImage::DrawQuadReductionY()
{
	float to = -1.5f;
	float ty = _imgHeight*4 +to;
	glBegin (GL_QUADS);
		glMultiTexCoord2f ( GL_TEXTURE0, 0,	to   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, 0,	to+1   ); 
		glMultiTexCoord2f ( GL_TEXTURE2, 0,	to+2  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, 0,	to+3  ); 
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, 0,  ty  );	
		glMultiTexCoord2f ( GL_TEXTURE1, 0,  ty+1  );	
		glMultiTexCoord2f ( GL_TEXTURE2, 0,  ty+2 );	
		glMultiTexCoord2f ( GL_TEXTURE3, 0,  ty+3 );	
		glVertex2i   ( 0			,		_imgHeight   ); 

 		glMultiTexCoord2f ( GL_TEXTURE0, (float)_imgWidth,	ty ); 	
 		glMultiTexCoord2f ( GL_TEXTURE1, (float)_imgWidth,	ty+1 ); 	
 		glMultiTexCoord2f ( GL_TEXTURE2, (float)_imgWidth,	ty+2); 	
 		glMultiTexCoord2f ( GL_TEXTURE3, (float)_imgWidth,	ty+3); 	
		glVertex2i   ( _imgWidth	,		_imgHeight   ); 

		glMultiTexCoord2f ( GL_TEXTURE0, (float)_imgWidth,		to   ); 
		glMultiTexCoord2f ( GL_TEXTURE1, (float)_imgWidth,		to+1  ); 
		glMultiTexCoord2f ( GL_TEXTURE2, (float)_imgWidth,		to+2  ); 
		glMultiTexCoord2f ( GL_TEXTURE3, (float)_imgWidth,		to+3  ); 
		glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();

	glFlush();
}


void GLTexImage::SetTextureSize()
{
	_texWidth = _imgWidth;
	_texHeight = _imgHeight;


}

void GLTexImage::DrawQuadPoints()
{
	glDrawArrays( GL_POINTS, 0,  _imgWidth*_imgHeight);	

}


void GLTexPacked::TexRGB2Gray()
{
	FrameBufferObject fbo;
	GLuint oldTexID = _texID;
	glGenTextures(1, &_texID);
	glBindTexture(_texTarget, _texID);
	glTexImage2D(_texTarget, 0, _iTexFormat, _texWidth,	_texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,	NULL);

	//input
	glBindTexture(_texTarget, oldTexID);
	//output 
	AttachToFBO(0);
	//program
	ShaderMan::UseShaderRGB2Gray();
	//draw buffer
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	//run
	DrawQuadPacking();
	ShaderMan::UnloadProgram();

	glDeleteTextures(1, &oldTexID);
	DetachFBO(0);
}

void GLTexPacked::SetTextureSize()
{
	_texWidth = 2+((_imgWidth+3)>>2);
	//9 is used to handle the boundary effect
	//the first and last column should have r=g=b=a
	//the right edge of the first column is latched
	_texHeight = _imgHeight;
}

//the right edge of the first column is latched
void  GLTexPacked::DrawQuadPacking()
{
	if(_debug) CheckFramebufferStatus();
	float x1 = -4.5f, x2 = (_texWidth<<2) - 4.5f;

	glBegin (GL_QUADS);
		glMultiTexCoord2f( GL_TEXTURE0, x1		,   0   ); 	
		glMultiTexCoord2f( GL_TEXTURE1, x1+1	,   0   ); 	
		glMultiTexCoord2f( GL_TEXTURE2, x1+2	,   0   ); 	
		glMultiTexCoord2f( GL_TEXTURE3, x1+3	,   0   ); 	
		glVertex2i   ( 0			,		0   ); 

		glMultiTexCoord2f( GL_TEXTURE0, x1		,   (float)_texHeight   ); 	
		glMultiTexCoord2f( GL_TEXTURE1, x1+1	,   (float)_texHeight  ); 	
		glMultiTexCoord2f( GL_TEXTURE2, x1+2	,   (float)_texHeight   ); 	
		glMultiTexCoord2f( GL_TEXTURE3, x1+3	,   (float)_texHeight   ); 	
		glVertex2i   ( 0			,	_texHeight   ); 

		glMultiTexCoord2f( GL_TEXTURE0, x2		,  (float) _texHeight   ); 	
		glMultiTexCoord2f( GL_TEXTURE1, x2+1	,  (float) _texHeight  ); 	
		glMultiTexCoord2f( GL_TEXTURE2, x2+2	,  (float) _texHeight   ); 	
		glMultiTexCoord2f( GL_TEXTURE3, x2+3	,  (float) _texHeight   ); 
		glVertex2i   ( _texWidth	,	_texHeight   ); 

		glMultiTexCoord2f( GL_TEXTURE0, x2		,   0   ); 	
		glMultiTexCoord2f( GL_TEXTURE1, x2+1	,   0  ); 	
		glMultiTexCoord2f( GL_TEXTURE2, x2+2	,   0   ); 	
		glMultiTexCoord2f( GL_TEXTURE3, x2+3	,   0   ); 		
		glVertex2i   ( _texWidth	,	0   ); 
	glEnd ();
	glFlush();
}


void GLTexPacked::DrawQuadUS(int scale)
{
	float texscale = 1.0f/scale;
	float x1 = 1.5f -  1.5f*texscale;
	float x2 =  _texWidth*texscale +x1;
	float y1 = 0.5f - 0.5f*texscale;
	float y2 = _texHeight*texscale +y1;
	glBegin (GL_QUADS);
		glTexCoord2f ( x1	,   y1   ); 	glVertex2i   ( 0			,		0   ); 
		glTexCoord2f ( x1	,   y2  );		glVertex2i   ( 0			,		_texHeight   ); 
 		glTexCoord2f ( x2	,	y2 ); 		glVertex2i   ( _texWidth	,		_texHeight   ); 
		glTexCoord2f ( x2	,   y1   ); 	glVertex2i   ( _texWidth	,		0   ); 
	glEnd ();

	glFlush();
}

void GLTexPacked::InitPackedTexture( int width,  int height,  int tw,  int th)
{

}
void GLTexPacked::DrawQuadDS(int scale)
{
	float x1 =  1.5f - 1.5f*scale;
	float x2 =  _texWidth*scale +x1;
	float y1 = 0.5f - 0.5f*scale;
	float y2 = _texHeight*scale +y1;


	if(scale ==2)
	{
		glBegin (GL_QUADS);
			glMultiTexCoord2f( GL_TEXTURE0, x1		,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x1+1	,   y1   ); 		
			glVertex2i   ( 0			,		0   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x1		,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x1+1	,   y2  ); 	
			glVertex2i   ( 0			,	_texHeight   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x2		,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x2+1	,   y2  ); 	 
			glVertex2i   ( _texWidth	,	_texHeight   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x2		,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x2+1	,   y1  ); 	
			glVertex2i   ( _texWidth	,	0   ); 
		glEnd ();



	}else
	{
		scale >>= 2;
		int step[3] = { scale, scale*2, scale*3};
		glBegin (GL_QUADS);
			glMultiTexCoord2f( GL_TEXTURE0, x1		,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x1+step[0]	,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE2, x1+step[1]	,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE3, x1+step[2]	,   y1   ); 	
			glVertex2i   ( 0			,		0   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x1		,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x1+step[0]	,   y2  ); 	
			glMultiTexCoord2f( GL_TEXTURE2, x1+step[1]	,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE3, x1+step[2]	,   y2   ); 	
			glVertex2i   ( 0			,	_texHeight   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x2		,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x2+step[0]	,   y2  ); 	
			glMultiTexCoord2f( GL_TEXTURE2, x2+step[1]	,   y2   ); 	
			glMultiTexCoord2f( GL_TEXTURE3, x2+step[2]	,   y2   ); 
			glVertex2i   ( _texWidth	,	_texHeight   ); 

			glMultiTexCoord2f( GL_TEXTURE0, x2		,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE1, x2+step[0]	,   y1  ); 	
			glMultiTexCoord2f( GL_TEXTURE2, x2+step[1]	,   y1   ); 	
			glMultiTexCoord2f( GL_TEXTURE3, x2+step[2]	,   y1   ); 		
			glVertex2i   ( _texWidth	,	0   ); 
		glEnd ();

	}
	

}

void GLTexPacked::DrawImage()
{
	float x1 = 1.0f; //border..
	float x2 =  _imgWidth*0.25f +x1;
	glBegin (GL_QUADS);
		glTexCoord2f ( x1	,   0   ); 					glVertex2i   ( 0			,		0   ); 
		glTexCoord2f ( x1	,  (float)_texHeight  );	glVertex2i   ( 0			,		_imgHeight   ); 
 		glTexCoord2f ( x2   ,  (float) _texHeight  ); 	glVertex2i   ( _imgWidth	,		_imgHeight   ); 
		glTexCoord2f ( x2	,   0   ); 					glVertex2i   ( _imgWidth	,		0   ); 
	glEnd ();
	glFlush();
}



void GLTexPacked::DrawBorder()
{
	glBegin (GL_LINES);
		glTexCoord2f ( 1.5f	,			0   ); 					glVertex2f   ( 0.5f				,		0   ); 
		glTexCoord2f ( 1.5f	,			(float) _texHeight  );	glVertex2f   ( 0.5f				,		(float)_texHeight   ); 
 		glTexCoord2f ( _texWidth-1.5f,	(float)	_texHeight  ); 	glVertex2f   ( _texWidth-0.5f	,		(float)_texHeight   ); 
		glTexCoord2f ( _texWidth-1.5f	,   0   ); 				glVertex2f   ( _texWidth-0.5f	,		0   ); 
	glEnd ();
	glFlush();
}

void GLTexImage::DisableTarget()
{
	glDisable(_texTarget);
}

void GLTexImage::EnableTarget()
{
	glEnable(_texTarget);

}

void GLTexImage::UnbindMultiTex(int n)
{
	for(int i = n-1; i>=0; i--)
	{
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(_texTarget, 0);
	}
}

int GLTexImage::DownSamplePixelData(int gl_format, int width, int height, int ds, unsigned char * pin, unsigned char * pout)	
{
	int step, linestep;
	int i, j, psz; 
	int ws = width/ds;
	int hs = height/ds;
	unsigned char * line, * p, *po;
	switch(gl_format)
	{
	case GL_LUMINANCE:
	case GL_LUMINANCE_ALPHA:
		psz = gl_format == GL_LUMINANCE? 1: 2;
		step = ds * psz; 
		linestep = width * psz *ds; 
		line = pin;
		po = pout; 
		for(i = 0 ; i < hs; i++, line+=linestep)
		{
			for(j = 0, p = line; j < ws; j++, p+=step)
			{
				*po++ = *p;
			}
		}
		break;
	case GL_RGB:
	case GL_RGBA:
		psz = gl_format == GL_RGB? 3: 4;
		step = ds * psz; 
		linestep = width * psz *ds; 
		line = pin;
		po = pout; 
		for(i = 0 ; i < hs; i++, line+=linestep)
		{
			for(j = 0, p = line; j < ws; j++, p+=step)
			{
				//((77*(r)+150*(g)+29*(b))>>8)
				//0.299, 0.587, 0.114 
				//*po++ = int(p[0]*0.299 + p[1] * 0.587 + p[2]* 0.114 + 0.5);
				*po++ = ((19595*p[0] + 38470*p[1] + 7471*p[2]+ 32768)>>16);
			}
		}
		break;
	case GL_BGR:
	case GL_BGRA:
		psz = gl_format == GL_BGR? 3: 4;
		step = ds * psz; 
		linestep = width * psz *ds; 
		line = pin;
		po = pout; 
		for(i = 0 ; i < hs; i++, line+=linestep)
		{
			for(j = 0, p = line; j < ws; j++, p+=step)
			{
				*po++ = ((7471*p[0] + 38470*p[1] + 19595*p[2]+ 32768)>>16);
			}
		}
		break;
	default:
		return 0;
	}

	return 1;

}
int GLTexImage::SetImageData( int width,  int height, unsigned char * data, 
							 unsigned int gl_format,  int& ds )
{


	int simple_format = IsSimpleGlFormat(gl_format);//no cpu code to handle other formats
	int ws, hs, done = 1; 

	if( simple_format 
		&& ( width > _texMaxDim || height > _texMaxDim || GlobalUtil::_PreProcessOnCPU) 
		&& GlobalUtil::_octave_min_default >0   )
	{
		ds = GlobalUtil::_octave_min_default; 
		ws = width >> GlobalUtil::_octave_min_default;
		hs = height >> GlobalUtil::_octave_min_default;
	}else
	{
		ds = 0; 
		ws = width;
		hs = height;
	}
	
	if ( ws > _texMaxDim || hs > _texMaxDim)
	{

		if(simple_format)
		{
			std::cerr<<"Automatic down-sampling is used\n";
			do
			{
				ds ++;
				ws >>= 1;
				hs >>= 1;
			}while(ws > _texMaxDim || hs > _texMaxDim);
		}else
		{
			std::cerr<<"Input images is too big to fit into a texture\n";
			return 0;
		}
	}

	_imgWidth = ws;	
	_imgHeight = hs;

	SetTextureSize();

	if(GlobalUtil::_verbose)
	{
		std::cout<<"Image size :\t"<<width<<"x"<<height<<endl;
		if(ds >0) 	std::cout<<"Down sample to \t"<<ws<<"x"<<hs<<endl;
	}

	if(_texID ==0)		glGenTextures(1, &_texID); 
	glBindTexture(_texTarget, _texID); 
	CheckErrorsGL("glBindTexture");

	glPixelStorei(GL_UNPACK_ALIGNMENT , 1);

	if(simple_format && ( ds> 0 || (gl_format != GL_LUMINANCE && GlobalUtil::_PreProcessOnCPU) ))
	{

		unsigned char * newdata = new unsigned char [_imgWidth * _imgHeight];
		if(DownSamplePixelData(gl_format, width, height, 1<<ds, data, newdata))
		{
			glTexImage2D(_texTarget, 0, _iTexFormat, _imgWidth, _imgHeight, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, newdata);
			GlobalUtil::FitViewPort(1, 1);
		}else
		{
			//impossible 
			done= 0; 
		}
		delete newdata;


	}else
	{
		//ds must be 0 here if not simpletype
		glTexImage2D(_texTarget, 0, _iTexFormat, _imgWidth, _imgHeight, 0, 
			gl_format,	GL_UNSIGNED_BYTE, data); 

		//convert RGB 2 GRAY if necessary
		if(gl_format == GL_LUMINANCE || gl_format == GL_LUMINANCE_ALPHA)
			GlobalUtil::FitViewPort(1, 1);
		else	
			TexRGB2Gray();
	}
	UnbindTex();

	return done;
}

int GLTexImage::LoadImageFile(char *imagepath, int &w, int &h,  int& ds )
{

	unsigned int imID;
	int done = 1;


	ilGenImages(1, &imID);
	ilBindImage(imID); 

	if(ilLoadImage(imagepath))
	{

		w = ilGetInteger(IL_IMAGE_WIDTH);
		h = ilGetInteger(IL_IMAGE_HEIGHT);
		int ilformat = ilGetInteger(IL_IMAGE_FORMAT);

		if(SetImageData(w, h, ilGetData(), ilformat, ds)==0)
		{
			done =0;
		}else 	if(GlobalUtil::_verbose)
		{
			std::cout<<"Image loaded :\t"<<imagepath<<endl;
		}

	}else
	{
		std::cerr<<"Unable to open image [code = "<<ilGetError()<<"]"<<endl;
		done = 0;
	}

	ilDeleteImages(1, &imID); 
	return done;
}
