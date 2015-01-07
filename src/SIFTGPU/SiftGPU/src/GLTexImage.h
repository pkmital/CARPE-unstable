////////////////////////////////////////////////////////////////////////////
//	File:		GLTexImage.h
//	Author:		Changchang Wu
//	Description : interface for the GLTexImage class.
//		GLTexImage:		naive texture class. 
//						sevral different quad drawing functions are provied
//		GLTexPacked: packed version (not finished yet)
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


#if !defined(GL_TEX_IMAGE_H)
#define GL_TEX_IMAGE_H

class GlobalUtil;
class GLTexImage :public GlobalUtil 
{	
protected:
	GLuint			_texID;
	int	_imgWidth;
	int	_imgHeight;
	int	_texWidth;
	int	_texHeight;
protected:
	virtual void TexRGB2Gray();
	virtual void SetTextureSize();

public:
	virtual void DrawQuadUS(int scale);
	virtual void DrawQuadDS(int scale);
	virtual void DrawImage();
public:
	static void DetachFBO(int i);
	static void UnbindTex();
	static int  IsSimpleGlFormat(unsigned gl_format)
	{
		//the formats there is a cpu code to conver rgb and downsample
		 return gl_format ==GL_LUMINANCE ||	gl_format == GL_LUMINANCE_ALPHA||
				gl_format == GL_RGB||	gl_format == GL_RGBA||
				gl_format == GL_BGR || gl_format == GL_BGRA; 
	}
public:
	static int DownSamplePixelData(int gl_format, int width, int height, int ds, unsigned char * pin, unsigned char * pout);
	int SetImageData(int width, int height, unsigned char * data, unsigned int gl_format , int& ds );
	int LoadImageFile(char * imagepath, int & w, int &h, int & ds );
	static void UnbindMultiTex(int n);
	static void EnableTarget();
	static void DisableTarget();
	void SetImageSize(int width, int height);
	void InitTexture(int width, int height, int clamp_to_edge =1);
	void DrawQuadPoints();
	void DrawScaledQuad(float scale);
	static void DrawQuad(float x1, float x2, float y1, float y2);
	void AttachToFBO(int i );
	operator GLuint (){return _texID;}
	void FitTexViewPort();
	void FitRealTexViewPort();
	void DrawQuadMTH();
	void DrawQuadMTV();
	void DrawQuadMT8();
	void DrawQuadMT4();
	void DrawQuadMT2();
	void DrawQuadReduction();
	void DrawQuadReductionX(int h);
	void DrawQuadReductionY();
	void DrawQuad();
	void FillMargin(int marginx, int marginy);
	void DrawMargin(int marginx, int marginy);
	void BindTex();

	GLTexImage(int width, int height);
	void TestDraw();
	GLuint GetTexID(){return _texID;}
	int	GetPixelCount(){return _imgWidth*_imgHeight;}
	int	GetImgWidth(){return _imgWidth;}
	int GetImgHeight(){return _imgHeight;}
	int	GetTexWidth(){return _texWidth;}
	int GetTexHeight(){return _texHeight;}
	int	IsTexTight(){return _texWidth == _imgWidth && _texHeight == _imgHeight;}
	GLTexImage();
	virtual ~GLTexImage();
	friend class SiftGPU;
};


class GLTexPacked:public GLTexImage
{
	void DrawQuadPacking();
public:
	virtual void	DrawImage();
	virtual void	TexRGB2Gray();
	virtual void	SetTextureSize();
	virtual void	DrawQuadUS(int scale);
	virtual void	DrawQuadDS(int scale);
public:
	void DrawBorder();
	GLTexPacked(int width, int height):GLTexImage(width, height){}
	GLTexPacked():GLTexImage(){}
	void InitPackedTexture(int width, int height, int tw, int th);
};

#endif // !defined(GL_TEX_IMAGE_H)
