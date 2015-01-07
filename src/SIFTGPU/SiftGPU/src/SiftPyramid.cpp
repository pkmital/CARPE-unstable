////////////////////////////////////////////////////////////////////////////
//	File:		SiftPyramid.cpp
//	Author:		Changchang Wu
//	Description :	Implementation of the SiftPyramid/PyramidNaive class.
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
#include <vector>
#include <algorithm>
using namespace std;
#include "stdlib.h"


#include "GlobalUtil.h"
#include "GLTexImage.h"
#include "SiftGPU.h"
#include "ProgramGLSL.h"
#include "ProgramCG.h"
#include "ShaderMan.h"
#include "SiftPyramid.h"
#include "math.h"
#include "FrameBufferObject.h"

#ifdef _WIN32
#include "imdebug/imdebuggl.h"
#endif


#if defined(HIGH_RESOLUTION_TIME) && defined(_WIN32)
	#include <windows.h>
	#include "mmsystem.h"
	#define clock	timeGetTime
	#define CLOCKS_PER_SEC 1000.0
#else
	#include "time.h"
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
PyramidNaive::PyramidNaive(SiftParam& sp): SiftPyramid(sp)
{
	_texPyramid = NULL;
	_auxPyramid = NULL;
	_alignment = 1;
	_vBufferID = NULL;
}

PyramidNaive::~PyramidNaive()
{


	DestroyPyramid();
}

//align must be 2^i
void PyramidNaive::	GetAlignedStorageSize(int num, int align,  int &fw, int &fh)
{
	if(num <=0)
	{
		fw = fh = 0;
	}else if(num < align*align)
	{
		fw = align;
		fh = (int)ceil(double(num) / fw);
	}		
	else
	{
		double dn = double(num);
		int nb = (int) ceil(dn/GlobalUtil::_texMaxDim/align);
		fh = align * nb;
		if(nb <=1)
		{
			fw = (int)ceil(dn / fh);
			//align this dimension to blocksize
			fw = ((int) ceil(double(fw) /align))*align;
		}else
		{
			fw = GlobalUtil::_texMaxDim;
		}

	}


}

void PyramidNaive::GetTextureStorageSize(int num, int &fw, int& fh)
{
	if(num <=0)
	{
		fw = fh = 0;
	}else if(num <= GlobalUtil::_FeatureTexBlock)
	{
		fw = num;
		fh = 1;
	}else
	{
		double dn = double(num);
		int nb = (int) ceil(dn/GlobalUtil::_texMaxDim/GlobalUtil::_FeatureTexBlock);
		fh = GlobalUtil::_FeatureTexBlock * nb;
		if(nb <=1)
		{
			fw = (int)ceil(dn / fh);

			//align this dimension to blocksize

			//
			if( fw < fh)
			{
				int temp = fh;
				fh = fw;
				fw = temp;
			}
		}else
		{
			fw = GlobalUtil::_texMaxDim;
		}
	}
}

void PyramidNaive::DestroyPyramid()
{
	DestroyPyramidData();

	if(_histoPyramidTex)
	{
		delete[]	_histoPyramidTex;
		_hpLevelNum = 0;
		_histoPyramidTex = NULL;
	}
	if(_tmpFeatureTex)
	{
		delete [] _tmpFeatureTex;
		_tmpFeatureTex = NULL;
	}

	if(_fHistoTex)
	{
		delete [] _fHistoTex;
		_fwLevel = 0;
		_fhLevel = 0;
		_fHistoTex = NULL;
	}

	if(_vBufferID)
	{
		glDeleteBuffers(1, &_vBufferID);
		_vBufferID = 0;
	}

	_allocated = 0;

}
void PyramidNaive::DestroyPyramidData()
{
	if(_texPyramid)
	{
		delete [] _texPyramid;
		_texPyramid = NULL;
	}
	if(_auxPyramid)
	{
		delete [] _auxPyramid;  
		_auxPyramid = NULL;
	}
	if(_depthTex)
	{
		glDeleteTextures(_octave_num, _depthTex);
		delete[]	_depthTex ;
		_depthTex =	NULL;
	}
	if(_levelFeatureNum)
	{
		delete _levelFeatureNum;
		_levelFeatureNum = NULL;
	}
	if(	_featureTex)
	{
		delete [] _featureTex;
		_featureTex =	NULL;
	}
	if(_featureTexAux )
	{
		delete [] _featureTexAux ;
		_featureTexAux = NULL;
	}

	if(_orientationTex)
	{
		delete []_orientationTex;
		_orientationTex = NULL;
	}
	int no = _octave_num* param._dog_level_num;
	if( _descriptorPBO)
	{
		glDeleteBuffers(no, _descriptorPBO);
		delete _descriptorPBO;
		_descriptorPBO = NULL;
	}
	if(_featureDisplayVBO)
	{
		glDeleteBuffers(no, _featureDisplayVBO);
		delete _featureDisplayVBO;
		_featureDisplayVBO = NULL;
	}
	if( _featurePointVBO)
	{
		glDeleteBuffers(no, _featurePointVBO);
		delete _featurePointVBO;
		_featurePointVBO = NULL;
	}
}

void PyramidNaive::FitPyramid(int w, int h)
{
	//(w, h) <= (_pyramid_width, _pyramid_height);

	_pyramid_octave_first = 0;
	//
	_octave_num  = GlobalUtil::_octave_num_default;

	int _octave_num_max = max(1, (int) floor (log ( double(min(w, h)))/log(2.0))  -3 );

	if(_octave_num < 1 || _octave_num > _octave_num_max) 
	{
		_octave_num = _octave_num_max;
	}


	int pw = _pyramid_width>>1, ph = _pyramid_height>>1;
	while(_pyramid_octave_first + _octave_num < _pyramid_octave_num &&  
		pw >= w && ph >= h)
	{
		_pyramid_octave_first++;
		pw >>= 1;
		ph >>= 1;
	}

	for(int i = 0; i < _octave_num; i++)
	{
		GLTexImage * tex = GetBaseLevel(i + _octave_min);
		GLTexImage * aux = GetBaseLevel(i + _octave_min, DATA_AUX);
		for(int j = param._level_min; j <= param._level_max; j++, tex++, aux++)
		{
			tex->SetImageSize(w, h);
			aux->SetImageSize(w, h);
		}
		w>>=1;
		h>>=1;
	}

}
void PyramidNaive::InitPyramid(int w, int h, int ds)
{
	int wp, hp ;
	if(ds == 0)
	{
		_down_sample_factor = 0;
		if(GlobalUtil::_octave_min_default>=0)
		{
			wp = w >> GlobalUtil::_octave_min_default;
			hp = h >> GlobalUtil::_octave_min_default;
		}else 
		{
			wp = w << (-GlobalUtil::_octave_min_default);
			hp = h << (-GlobalUtil::_octave_min_default);
		}
		_octave_min = _octave_min_default;
	}else
	{
		//must use 0 as _octave_min; 
		_octave_min = 0;
		_down_sample_factor = ds;
		w >>= ds;
		h >>= ds;
		wp = w;
		hp = h; 

	}

	while(wp > GlobalUtil::_texMaxDim || hp > GlobalUtil::_texMaxDim)
	{
		_octave_min ++;
		wp >>= 1;
		hp >>= 1;
	}

	if( wp == _pyramid_width && hp == _pyramid_height && _allocated )
	{
		FitPyramid(wp, hp);
	}else if(GlobalUtil::_ForceTightPyramid || _allocated ==0)
	{
		ResizePyramid(wp, hp);
	}
	else if( wp > _pyramid_width || hp > _pyramid_height )
	{
		ResizePyramid(max(wp, _pyramid_width), max(hp, _pyramid_height));
	}
	else
	{
		//try use the pyramid allocated for large image on small input images
		FitPyramid(wp, hp);
	}

	//select the initial smoothing filter according to the new _octave_min
	ShaderMan::SelectInitialSmoothingFilter(_octave_min, param);
}

void PyramidNaive::ResizePyramid( int w,  int h)
{
	//
	int _octave_num_new, input_sz;
	int wmax, hmax, num, i, j;
	GLTexImage * tex, *aux;
	//

	if(_pyramid_width == w && _pyramid_height == h && _allocated) return;

	if(w > GlobalUtil::_texMaxDim || h > GlobalUtil::_texMaxDim) return ;
	//first octave does not change
	_pyramid_octave_first = 0;

	
	//compute # of octaves

	input_sz = min(w,h) ;


	_pyramid_width = wmax = w;
	_pyramid_height = hmax = h;



	//reset to preset parameters

	_octave_num_new  = GlobalUtil::_octave_num_default;

	if(_octave_num_new < 1) 
	{
		_octave_num_new = (int) floor (log ( double(input_sz))/log(2.0)) -3 ;
		if(_octave_num_new<1 ) _octave_num_new = 1;
	}

	if(_pyramid_octave_num != _octave_num_new)
	{
		//destroy the original pyramid if the # of octave changes
		if(_octave_num >0) DestroyPyramidData();
		_pyramid_octave_num = _octave_num_new;
	}

	_octave_num = _pyramid_octave_num;

	int noct = _octave_num;
	int nlev = param._level_num;







	//	//initialize the pyramid
	if(_texPyramid==NULL)	_texPyramid = new GLTexImage[ noct* nlev ];
	if(_auxPyramid==NULL)	_auxPyramid = new GLTexImage[ noct* nlev ];


	//pre allocate depth texture?
	if(	GlobalUtil::_DepthTexAllocation && _depthTex==NULL)
	{
		_depthTex = new GLuint[_octave_num];
		glGenTextures(_octave_num, _depthTex);
	}

	//
	for(i = 0, tex = _texPyramid, aux = _auxPyramid; i< noct; i++)
	{
		for( j = 0; j< nlev; j++, tex++, aux++)
		{
			tex->InitTexture(w, h);
			//tex->AttachToFBO(0);
			aux->InitTexture(w, h, 0);
			//aux->AttachToFBO(0);
		}
		if(GlobalUtil::	_DepthTexAllocation)
		{
			glBindTexture(GlobalUtil::_texTarget, _depthTex[i]);
			glTexImage2D	(	GlobalUtil::_texTarget, 0, 	GL_DEPTH_COMPONENT, w, h,
							0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GlobalUtil::_texTarget, 0);
		}
		w>>=1;
		h>>=1;
	}



	if(_levelFeatureNum==NULL)	_levelFeatureNum = new int[_octave_num * param._dog_level_num];
	std::fill(_levelFeatureNum, _levelFeatureNum+_octave_num * param._dog_level_num, 0); 

	//use a fbo to initialize textures..
	FrameBufferObject fbo;
	
	//histogram for feature detection


	num = (int)ceil(log(double(max(wmax, hmax)))/log(2.0));
	if( _hpLevelNum != num)
	{
		_hpLevelNum = num;
		if(_histoPyramidTex ) delete [] _histoPyramidTex;
		_histoPyramidTex = new GLTexImage[_hpLevelNum];
		w = 1;
		h = 1 ;
		for(i = 0; i < _hpLevelNum; i++)
		{
			_histoPyramidTex[i].InitTexture(w, h, 0);
			_histoPyramidTex[i].AttachToFBO(0);
			w<<=1;
			h<<=1;
		}
	}



	//initialize the feature texture

	int idx = 0, n = _octave_num * param._dog_level_num;
	if(_featureTex==NULL)	_featureTex = new GLTexImage[n];
	if(GlobalUtil::_MaxOrientation >1)
	{
		if(GlobalUtil::_ListReshapeGPU && _featureTexAux == NULL)_featureTexAux = new GLTexImage[n];
		if(_orientationTex== NULL)		_orientationTex = new GLTexImage[n];
	}



	for(i = 0; i < _octave_num; i++)
	{
		GLTexImage * tex = GetBaseLevel(i+_octave_min);
		int fmax = int(tex->GetImgWidth()*tex->GetImgHeight()*GlobalUtil::_MaxFeaturePercent);
		int fw, fh;
		//
		if(fmax > GlobalUtil::_MaxLevelFeatureNum) fmax = GlobalUtil::_MaxLevelFeatureNum;
		else if(fmax < 32) fmax = 32;	//give it at least a space of 32 feature

		GetTextureStorageSize(fmax, fw, fh);
		
		for(int j = 0; j < param._dog_level_num; j++, idx++)
		{
			_featureTex[idx].InitTexture(fw, fh, 0);
			_featureTex[idx].AttachToFBO(0);
			//
			if(GlobalUtil::_MaxOrientation>1)
			{
				if(GlobalUtil::_ListReshapeGPU)_featureTexAux[idx].InitTexture(fw, fh, 0);
				if(GlobalUtil::_ListReshapeGPU)_featureTexAux[idx].AttachToFBO(0);
				_orientationTex[idx].InitTexture(fw, fh, 0);
				_orientationTex[idx].AttachToFBO(0);
			}
		}


	}

	//initialize feature texture pyramid
	wmax = _featureTex->GetImgWidth();
	hmax = _featureTex->GetImgHeight();

	if(GlobalUtil::_MaxOrientation>1 && GlobalUtil::_ListReshapeGPU)
	{
		ReallocateFeatureHistoTex(wmax, hmax);
	}
	int nf, ns;
	if(GlobalUtil::_ComputeDescriptors)
	{
		//32*4 = 128. 
		nf = 32 / GlobalUtil::_DescriptorFormat;	// how many textures we need
		ns = GlobalUtil::_DescriptorFormat;		    // how many point in one texture for one descriptor
	}else
	{
		nf = 1; ns = 4;
	}
	//
	_alignment = ns;
	//
	if(_tmpFeatureTex==NULL)	_tmpFeatureTex = new GLTexImage[nf];

	int fw, fh;
	GetAlignedStorageSize(hmax*wmax* max(ns, 10), _alignment, fw, fh);

	if(fh < hmax ) fh = hmax;
	if(fw < wmax ) fw = wmax;
	for(i =0; i < nf; i++)
	{
		_tmpFeatureTex[i].InitTexture(fw, fh);
	}

	//build the vertex buffer for features??
	if(GlobalUtil::_DescriptorFormat ==32 && GlobalUtil::_ComputeDescriptors)
	{
		BuildVertexBuffer(wmax*hmax);
	}

	//
	_allocated = 1;

}

#define USE_TIMING()		double t, t0, tt;
#define OCTAVE_START()		if(GlobalUtil::_verboseO){	t = t0 = clock();		cout<<"#"<<i+_down_sample_factor<<"\t";	}
#define LEVEL_FINISH()		if(GlobalUtil::_verboseL){	glFinish();	tt = clock();cout<<(tt-t)/CLOCKS_PER_SEC<<"\t";	t = clock();}
#define OCTAVE_FINISH()		if(GlobalUtil::_verboseO)cout<<"|\t"<<(clock()-t0)/CLOCKS_PER_SEC<<endl;


void PyramidNaive::BuildPyramid(GLTexImage *input)
{

	//
	USE_TIMING();
	int i, j, k;
	GLTexImage * tex;
	FilterProgram ** filter;
	FrameBufferObject fbo;

	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	input->FitTexViewPort();

	for ( i = _octave_min; i < _octave_min + _octave_num; i++)
	{

		tex = GetBaseLevel(i);

		j = param._level_min + 1;
		filter = ShaderMan::f_gaussian_step;

		OCTAVE_START();

		if( i == _octave_min )
		{
			if(i < 0)
			{

				TextureUpSample(tex, input, 1<<(-i)	);			
			}else
			{
				//image might have been already down-sampled by cpu code
				TextureDownSample(tex, input, 1<<i);
			}
		

			//
			if(ShaderMan::f_gaussian_skip0)
			{
				ShaderMan::f_gaussian_skip0->RunFilter(tex, tex, NULL);
			}
	
			LEVEL_FINISH();
		}else
		{
			TextureDownSample(tex, GetLevelTexture(i-1, param._level_ds)); 
	
			LEVEL_FINISH();

			if(ShaderMan::f_gaussian_skip1)
			{
				ShaderMan::f_gaussian_skip1->RunFilter(tex, tex, NULL);
				LEVEL_FINISH();
			}else if (GlobalUtil::_MaxFilterWidth == -1 && GlobalUtil::_ExtraDownSample)
			{
				//copy and downsample other extra levels...
				//according to definition, we can down sample more than one level
				for(k = param._level_ds +1 ;k <= param._level_max; k++, tex++, j++, filter++)
				{

					TextureDownSample(tex+1, GetLevelTexture(i-1, k));
					LEVEL_FINISH();
				}
				//this won't work well if the gussian kernel size is truncated for large size
			}
		}
		for( ; j <=  param._level_max ; j++, tex++, filter++)
		{
			// filtering
			(*filter)->RunFilter(tex+1, tex, NULL);
			LEVEL_FINISH();

		}
		OCTAVE_FINISH();

	}
	if(GlobalUtil::_verboseS)	glFinish();
	UnloadProgram();
}


void PyramidNaive::RunSIFT(GLTexImage*input)
{
	//
	GlobalUtil::StartTimer("Build    Pyramid");
	BuildPyramid(input);
	GlobalUtil::StopTimer();
	_timing[0] = GetElapsedTime();

	GlobalUtil::StartTimer("Detect Keypoints");
	if(GlobalUtil::_SubpixelLocalization || GlobalUtil::_UseNewDogAlgorithm)
		DetectKeypointsEX();
	else
		DetectKeypoints();
	GlobalUtil::StopTimer();
	_timing[1] = GetElapsedTime();


	if(GlobalUtil::_ListGenGPU)
	{
		GlobalUtil::StartTimer("Get Feature List");
		GenerateFeatureList();
		GlobalUtil::StopTimer();

	}else
	{
		GlobalUtil::StartTimer("Download/Upload Feature List");
		GenerateFeatureListCPU();
		GlobalUtil::StopTimer();
	}
	_timing[2] = GetElapsedTime();

	if(GlobalUtil::_FixedOrientation)
	{
		//use 0 as orientation for all features
		_timing[3] = _timing[4] =0;
	}else 	if(GlobalUtil::_SupportFP40)
	{
		GlobalUtil::StartTimer("Feature Orientations");
		GetFeatureOrienations();
		GlobalUtil::StopTimer();
		_timing[3] = GetElapsedTime();

		if(GlobalUtil::_MaxOrientation >1 )
		{
			GlobalUtil::StartTimer("Reshape Feature List");
			if(GlobalUtil::_ListReshapeGPU)
				ReshapeFeatureList();
			else
				ReshapeFeatureListCPU();
			GlobalUtil::StopTimer();	
			_timing[4] = GetElapsedTime();
		}else
		{
			_timing[4] = 0;
		}
	}else
	{
		GlobalUtil::StartTimer("Feature Orientations");
		GetSimplifiedOrientation();
		GlobalUtil::StopTimer();
		_timing[3] = GetElapsedTime();
		_timing[4] = 0; 
	}



	GlobalUtil::StartTimer("Download Keypoints");
	PrepareBuffer();
	DownloadKeypoints();
	GlobalUtil::StopTimer();
	_timing[5] =  GetElapsedTime(); 



	if(GlobalUtil::_ComputeDescriptors)
	{
		//desciprotrs are downloaded in descriptor computation of each level
		GlobalUtil::StartTimer("Descriptor");
		GetFeatureDescriptors();
		GlobalUtil::StopTimer();
		_timing[6] =  GetElapsedTime(); 
	}else
	{
		_timing[6] = 0;
	}

}





GLTexImage*  PyramidNaive::GetLevelTexture(int octave, int level)
{
	return _texPyramid+ (_pyramid_octave_first + octave - _octave_min) * param._level_num 
		+ (level - param._level_min);
}


GLTexImage*  PyramidNaive::GetBaseLevel(int octave, int dataName)
{
	if(octave <_octave_min || octave > _octave_min + _octave_num) return NULL;
	switch(dataName)
	{
		case DATA_GAUSSIAN:
		case DATA_DOG:
		case DATA_GRADX:
		case DATA_GRADY:
			return _texPyramid+ (_pyramid_octave_first + octave - _octave_min) * param._level_num;
		case DATA_KEYPOINT:
		case DATA_AUX:
			return _auxPyramid + (_pyramid_octave_first + octave - _octave_min) * param._level_num;
		default:
			return NULL;
	}
}



void PyramidNaive::BuildVertexBuffer(int count)
{
	//UseShaderCopyTexCoord
	int w = _tmpFeatureTex[0].GetTexWidth();
	int h = (int)ceil(count / double(w));
	_tmpFeatureTex[0].AttachToFBO(0);
	_tmpFeatureTex[0].FitRealTexViewPort();
	ShaderMan::UseShaderCopyTexCoord();
	glBegin(GL_QUADS);
	for(int i = 0, c = 0; i < h; i++)
	{
		glTexCoord2i(c, 0);			glVertex2i( 0, i);
		glTexCoord2i(c, 1);			glVertex2i( 0, i +1);
		c+=w;
		glTexCoord2i(c, 1);			glVertex2i( w, i +1);
		glTexCoord2i(c, 0);			glVertex2i( w, i);
	}
	glEnd();

	if(_vBufferID ==0)		glGenBuffers(1, &_vBufferID);
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _vBufferID);
	glBufferData(GL_PIXEL_PACK_BUFFER_ARB, w*h*4*sizeof(float),	NULL, GL_STATIC_DRAW_ARB);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
}


void PyramidNaive::DrawVertexBuffer(int count)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer( GL_ARRAY_BUFFER, _vBufferID );
	glVertexPointer( 4, GL_FLOAT, 0, (char *) NULL );	
	glTexCoordPointer(4, GL_FLOAT, 0, (char*) NULL);
	glDrawArrays( GL_POINTS, 0, count);
	glFlush();
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

/*	glBegin(GL_POINTS);
	for(int i = 0; i < count; i++)
	{
		glVertex2f(i+0.5, 0.5);
	}
	glEnd();	*/
	GlobalUtil::CheckErrorsGL();
}









//keypoint detection with subpixel localization
void PyramidNaive::DetectKeypointsEX()
{
	//

	int i, j;
	double t0, t, ts, t1, t2;
	GLTexImage * tex, *aux;
	FrameBufferObject fbo;


	if(GlobalUtil::_verbose)ts = clock();
	
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);


	if(GlobalUtil::_UseSiftGPUEX)
	{
		//extra levels for visualization
		int levels[3] = {param._level_min, param._level_min +1, param._level_max};
		for ( i = _octave_min; i < _octave_min + _octave_num; i++)
		{
			for( j =0; j < 3 ; j++)
			{
				tex = GetLevelTexture(i, levels[j]);
				tex->FitTexViewPort();
				tex->AttachToFBO(0);
				tex->BindTex();
				ShaderMan::UseShaderGradientPass();
				tex->DrawQuadMT4();
			}
		}		

	}else if(GlobalUtil::_GradientLevelOffset ==1) //must be either 1 or 2
	{
		for ( i = _octave_min; i < _octave_min + _octave_num; i++)
		{
			tex = GetLevelTexture(i, param._level_min +1);
			tex->FitTexViewPort();
			tex->AttachToFBO(0);
			tex->BindTex();
			ShaderMan::UseShaderGradientPass();
			tex->DrawQuadMT4();
		}
	}
	if(GlobalUtil::_verboseS)
	{
		glFinish();
		t1 = clock();
	}


	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	glDrawBuffers(2, buffers);
	for ( i = _octave_min; i < _octave_min + _octave_num; i++)
	{
		if(GlobalUtil::_verboseO)
		{
			t0 = clock();
			std::cout<<"#"<<(i + _down_sample_factor)<<"\t";
		}
		tex = GetBaseLevel(i) + 2;
		aux = GetBaseLevel(i, DATA_AUX) +2;
		aux->FitTexViewPort();

		for( j = param._level_min +2; j <  param._level_max ; j++, aux++, tex++)
		{
			if(GlobalUtil::_verboseL)t = clock();		
			tex->AttachToFBO(0);
			aux->AttachToFBO(1);
			glActiveTexture(GL_TEXTURE0);
			tex->BindTex();
			glActiveTexture(GL_TEXTURE1);
			(tex+1)->BindTex();
			glActiveTexture(GL_TEXTURE2);
			(tex-1)->BindTex();
			ShaderMan::UseShaderKeypoint((tex+1)->GetTexID(), (tex-1)->GetTexID());
			aux->DrawQuadMT8();
	
			if(GlobalUtil::_verboseL)
			{
				glFinish();
				std::cout<<(clock()-t)/CLOCKS_PER_SEC<<"\t";
			}
			//::imdebugTexImagef(GlobalUtil::_texTarget, (tex-1)->GetTexID(), GL_RGBA);			
			//::imdebugTexImagef(GlobalUtil::_texTarget, tex->GetTexID(), GL_RGBA);
			//::imdebugTexImagef(GlobalUtil::_texTarget, aux->GetTexID(), GL_RGBA);
		}
		if(GlobalUtil::_verboseO)
		{
			std::cout<<"|\t"<<(clock()-t0)/CLOCKS_PER_SEC<<"\n";
		}
	}

	if(GlobalUtil::_verboseS)
	{
		glFinish();
		t2 = clock();
		std::cout<<"<Get Keypoints ..  >\t"<<(t2-t1)/CLOCKS_PER_SEC<<"\n";
		std::cout<<"<Comptue Extra Gradient..>\t"<<(t1-ts)/CLOCKS_PER_SEC<<"\n";
	}
	UnloadProgram();
	GLTexImage::UnbindMultiTex(3);
	fbo.UnattachTex(GL_COLOR_ATTACHMENT1_EXT);


}
void PyramidNaive::DetectKeypoints()
{


	//
	int i, j;
	double t0, t, ts, t1, t2, tt;
	GLTexImage * tex, *aux;
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	FrameBufferObject fbo;
	glDrawBuffers(2, buffers);

	if(GlobalUtil::_verbose)ts = clock();

	for ( i = _octave_min; i < _octave_min + _octave_num; i++)
	{

		tex = GetBaseLevel(i)+1;
		aux = GetBaseLevel(i, DATA_AUX)+1;
		tex->FitTexViewPort();
		if(GlobalUtil::_verboseO)
		{
			t = t0 = clock();
			std::cout<<"#"<<(i+ _down_sample_factor)<<"\t";
		}
		for( j = param._level_min+1; j <=  param._level_max ; j++, tex++, aux++)
		{
			//the first level is not needed to compute when 
			//SiftGPU is running a no_visualization mode
			if(j == param._level_min && !GlobalUtil::_UseSiftGPUEX ) continue;
			//
			tex->AttachToFBO(0);
			aux->AttachToFBO(1);
			tex->BindTex();

			if ( j == param._level_min || j == param._level_min + 1 || j == param._level_max )
			{
				//
				ShaderMan::UseShaderGradientPass();
			}else
			{
				//compute gradient and do non-extrema supression in the level
				ShaderMan::UseShaderDogPass1();
			}
			tex->DrawQuadMT8();
			
			
			if(GlobalUtil::_verboseL)
			{
				glFinish();
				tt = clock();
				std::cout<<(tt-t)/CLOCKS_PER_SEC<<"\t";
				t = tt;
			}

		}
		if(GlobalUtil::_verboseO)
		{
			std::cout<<"|\t"<<(clock()-t0)/CLOCKS_PER_SEC<<"\n";
		}

	}

	if(GlobalUtil::_verboseS)
	{
		glFinish();
		t1 = clock();
	}

	//rund dog test with early-z

	GLuint depthTex;
	if(GlobalUtil::_DepthTexAllocation==0)	glGenTextures	(1, &depthTex);

	GlobalUtil::CheckErrorsGL();


	glEnable(GL_DEPTH_TEST);

	for ( i = _octave_min; i < _octave_min + _octave_num; i++)
	{
		if(GlobalUtil::_verboseO)
		{
			t0 = clock();
			std::cout<<"#"<<(i-_octave_min+ _down_sample_factor)<<"\t";
		}
		aux = GetBaseLevel(i, DATA_AUX)+2;
		aux->FitTexViewPort();
		
		//define new depth buffer with different dimension
		if(GlobalUtil::_DepthTexAllocation)
		{
			depthTex = _depthTex[i-_octave_min];
		}else
		{
			glBindTexture	(GlobalUtil::_texTarget, depthTex);
			glTexImage2D	(	GlobalUtil::_texTarget, 0, GL_DEPTH_COMPONENT, 
								aux->GetTexWidth(), aux->GetTexHeight(),
								0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GlobalUtil::_texTarget, 0);
		}

		//
		fbo.UnattachTex(GL_COLOR_ATTACHMENT0_EXT);
		fbo.UnattachTex(GL_COLOR_ATTACHMENT1_EXT);
		fbo.AttachDepthTexture(GlobalUtil::_texTarget, depthTex);
		

		for( j = param._level_min +2; j <  param._level_max ; j++, aux++)
		{

			if(GlobalUtil::_verboseL)t = clock();

	        glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glDepthFunc(GL_ALWAYS);	
			glDepthMask(GL_TRUE); 
		
	
			glActiveTexture(GL_TEXTURE0);
			aux->BindTex();
			
			//early-z pass
			ShaderMan::UseShaderDogZpass();
			aux->DrawQuad();
			

			glDepthFunc(GL_LESS);	

			glDrawBuffer(buffers[0]);
			aux->AttachToFBO(0);


			aux->BindTex();
			glActiveTexture(GL_TEXTURE1);
			(aux+1)->BindTex();
			glActiveTexture(GL_TEXTURE2);
			(aux-1)->BindTex();

			ShaderMan::UseShaderDogPass2((aux+1)->GetTexID(),  (aux-1)->GetTexID());

			//
			aux->DrawQuad();
			
			if(GlobalUtil::_verboseL)
			{
				glFinish();
				std::cout<<(clock()-t)/CLOCKS_PER_SEC<<"\t";
			}
		}
		if(GlobalUtil::_verboseO)
		{
			std::cout<<"|\t"<<(clock()-t0)/CLOCKS_PER_SEC<<"\n";
		}
	}

	if(GlobalUtil::_verboseS)
	{
		glFinish();
		t2 = clock();
		std::cout<<"<Comptue Gradient..>\t"<<(t1-ts)/CLOCKS_PER_SEC<<"\n";
		std::cout<<"<Get Keypoints ..  >\t"<<(t2-t1)/CLOCKS_PER_SEC<<"\n";
	}
	UnloadProgram();
	glDisable(GL_DEPTH_TEST);
	if(GlobalUtil::_DepthTexAllocation==0)glDeleteTextures(1, &depthTex);
	GLTexImage::UnbindMultiTex(3);
	fbo.UnattachTex(GL_COLOR_ATTACHMENT1_EXT);
	fbo.AttachDepthTexture(GlobalUtil::_texTarget, 0);
}

//generate feature list on GPU
void PyramidNaive::GenerateFeatureList()
{
	//generate the histogram pyramid
	FrameBufferObject fbo;
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	GLTexImage * htex, * ftex;
	double t, tt, ttt, to;//, ts, tst; 
//	double ts1, ts2;
	int ocount, idx = 0;
	_featureNum = 0;
	_featureNumEX = 0;
	for(int i = 0; i < _octave_num; i++)
	{
		GLTexImage * tex = GetBaseLevel(_octave_min + i, DATA_AUX) + 2;
		//output

		if(GlobalUtil::_verboseO)
		{
			to = clock();
			ocount = 0;
			std::cout<<"#"<<i+_octave_min + + _down_sample_factor<<" Octave:\t";
		}
		for(int j = 0; j < param._dog_level_num; j++, tex++, idx++)
		{
			htex = _histoPyramidTex + _hpLevelNum - 1 - i;
			if(GlobalUtil::_verboseL) t= clock();
			glActiveTexture(GL_TEXTURE0);
			tex->BindTex();
			htex->AttachToFBO(0);
			ShaderMan::UseShaderGenListInit(tex->GetImgWidth(), tex->GetImgHeight(), tex->IsTexTight());
			htex->FitTexViewPort();
			//this uses the fact that no feature is on the edge.
			htex->DrawQuadReduction();

			//reduction..
			htex--;
	
			//this part might have problems on several GPUS
			//because the output of one pass is the input of the next pass
			//need to call glFinish to make it right
			//but too much glFinish makes it slow
			for(int k = 0; k <_hpLevelNum - i-1; k++, htex--)
			{
				htex->AttachToFBO(0);
				htex->FitTexViewPort();
				(htex+1)->BindTex();
				ShaderMan::UseShaderGenListHisto();
				htex->DrawQuadReduction();					
			}
			//glFinish();

			//read back one pixel
			float fn[4];
			glReadPixels(0, 0, 1, 1, GL_RGBA , GL_FLOAT, fn);
			float fcount = (fn[0] + fn[1] + fn[2] + fn[3]);
			if(fcount < 1) fcount = 0;


			//
			if(GlobalUtil::_verboseL)
			{
				tt= clock();
			}
			//
			ftex = _featureTex+idx;
			_levelFeatureNum[ idx] = (int)(fcount);
			SetLevelFeatureNum(idx, (int)fcount);

			//save  number of features
			ocount+=int(fcount);
			_featureNumEX = max(_featureNumEX, _featureNum + ftex->GetPixelCount());
			_featureNum += int(fcount);

			//
			if(fcount < 1.0) 
			{
				if(GlobalUtil::_verboseL) std::cout<<"("<<(clock()-t)/CLOCKS_PER_SEC<<",\t0,\t0)\t";
				continue;
			}
		

			///generate the feature texture

			htex=  _histoPyramidTex;

			if(GlobalUtil::_IsATI)
			{
				unsigned char zeros[4] = {0, 0, 0, 0};
				ftex->BindTex();
				glTexSubImage2D(GlobalUtil::_texTarget, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, zeros);
				//something need to be done to activate the texture on my ATI..

			}
			htex->BindTex();

			
			if(GlobalUtil::_IsATI) 
			{
				glTexSubImage2D(GlobalUtil::_texTarget, 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, fn);
				//something need to be done to activate the texture on my ATI..
			}
			//first pass
			ftex->AttachToFBO(0);
			if(GlobalUtil::_MaxOrientation>1)
			{
				//this is very important...
				ftex->FitRealTexViewPort();
				glClear(GL_COLOR_BUFFER_BIT);
				glFinish();
			}else
			{
				
				ftex->FitTexViewPort();
				
			}


			ShaderMan::UseShaderGenListStart((float)ftex->GetImgWidth(), htex->GetTexID());

			ftex->DrawQuad();
			//make sure it finishes before the next step
			ftex->DetachFBO(0);

			
			
			//pass on each pyramid level
			htex++;
			for(int lev = 1; lev < _hpLevelNum  - i; lev++, htex++)
			{

				glActiveTexture(GL_TEXTURE0);
				ftex->BindTex();
				ftex->AttachToFBO(0);
				glActiveTexture(GL_TEXTURE1);
				htex->BindTex();
				ShaderMan::UseShaderGenListStep(ftex->GetTexID(), htex->GetTexID());
				ftex->DrawQuad();
				ftex->DetachFBO(0);	
			}
			if(GlobalUtil::_verboseL)
			{
				glFinish();
				ttt = clock();
				std::cout<<"("<<(tt-t)/CLOCKS_PER_SEC<<",\t"<<(ttt-tt)/CLOCKS_PER_SEC<<",\t"<<int(fcount)<<")\t";
			}
			GLTexImage::UnbindMultiTex(2);


		}
		if(GlobalUtil::_verboseO)
		{	
			glFinish();
			std::cout<<"|\t("<<(clock()-to)/CLOCKS_PER_SEC<<",\t"<<int(ocount)<<")\n";
		}
	}
	if(GlobalUtil::_verboseS)glFinish();
	if(GlobalUtil::_verbose)
	{
		std::cout<<"#Features:\t"<<_featureNum<<"\n";
	}
}


void PyramidNaive::GenerateFeatureDisplayVBO()
{
	//use a big VBO to save all the SIFT box vertices
	int w, h, bsize, esize;
	int nvbo = _octave_num * param._dog_level_num;
	if(_featureDisplayVBO==NULL)
	{
		//initialize the vbos
		_featureDisplayVBO = new unsigned int[nvbo];
		_featurePointVBO = new unsigned int[nvbo];
		glGenBuffers( nvbo, _featureDisplayVBO );	
		glGenBuffers(nvbo, _featurePointVBO);
	}

	FrameBufferObject fbo;
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glActiveTexture(GL_TEXTURE0);
	
	//
	GLTexImage & tempTex = * _tmpFeatureTex;
	//
	for(int i = 0, idx = 0; i < _octave_num; i++)
	{
		for(int j = 0; j < param._dog_level_num; j ++, idx++)
		{
			GLTexImage * ftex  = _featureTex + idx;

			if(_levelFeatureNum[idx]<=0)continue;
			//box display vbo
			int count = _levelFeatureNum[idx]* 10;
			GetAlignedStorageSize(count, _alignment, w, h);
			w = (int)ceil(double(count)/ h);

			//render
			//input
			fbo.BindFBO();
			ftex->BindTex();

			//output
			tempTex.AttachToFBO(0);
			GlobalUtil::FitViewPort(w, h);
			//shader
			ShaderMan::UseShaderGenVBO(
				(float)ftex->GetImgWidth(),  (float) w, 
				param.GetLevelSigma(j + param._level_min + 1));

			GLTexImage::DrawQuad(0,  (float)w, 0, (float)h);
		
			//imdebugTexImagef(GlobalUtil::_texTarget, tempTex, GL_RGBA);

			//
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _featureDisplayVBO[ idx]);
			glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_ARB, GL_BUFFER_SIZE, &bsize);
			esize = w*h * sizeof(float)*4;
			if(bsize < esize) //increase size when necessary
				glBufferData(GL_PIXEL_PACK_BUFFER_ARB, esize*3/2,	NULL, GL_STATIC_DRAW_ARB);
			glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, 0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);


			//copy the texture into vbo
			fbo.BindFBO();
			tempTex.AttachToFBO(0);


			ftex->BindTex();
			ftex->FitTexViewPort();
			ShaderMan::UseShaderCopyKeypoint();
			ftex->DrawQuad();

			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,  _featurePointVBO[ idx]);
			glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_ARB, GL_BUFFER_SIZE, &bsize);
			esize = ftex->GetImgHeight() * ftex->GetImgWidth()*sizeof(float) *4;
			if(bsize < esize) //increase size when necessary
				glBufferData(GL_PIXEL_PACK_BUFFER_ARB, esize*3/2 ,	NULL, GL_STATIC_DRAW_ARB);
			glReadPixels(0, 0, ftex->GetImgWidth(), ftex->GetImgHeight(), GL_RGBA, GL_FLOAT, 0);

			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
		/*	*/
			
		}
	}

	glReadBuffer(GL_NONE);
	glFinish();

}





void PyramidNaive::GetFeatureOrienations()
{
	//

	///
	int n = _octave_num  * param._dog_level_num;

	GLTexImage * gtex;
	GLTexImage * stex = NULL;
	GLTexImage * ftex = _featureTex;
	GLTexImage * otex = _orientationTex;
	int sid = 0; 
	int * count	 = _levelFeatureNum;
	float sigma, sigma_step = powf(2.0f, 1.0f/param._dog_level_num);
	FrameBufferObject fbo;
	if(GlobalUtil::_MaxOrientation>1)
	{
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
		glDrawBuffers(2, buffers);
	}else
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	}
	for(int i = 0; i < _octave_num; i++)
	{
		gtex = GetLevelTexture(i+_octave_min, param._level_min + GlobalUtil::_GradientLevelOffset);
		if(GlobalUtil::_SubpixelLocalization)
			stex = GetBaseLevel(i+_octave_min, DATA_AUX) + 2;

		for(int j = 0; j < param._dog_level_num; j++, ftex++, otex++, count++, gtex++, stex++)
		{
			if(*count<=0)continue;

			sigma = param.GetLevelSigma(j+param._level_min+1);

			//
			ftex->FitTexViewPort();

			glActiveTexture(GL_TEXTURE0);
			ftex->BindTex();
			glActiveTexture(GL_TEXTURE1);
			gtex->BindTex();
			//
			ftex->AttachToFBO(0);
			if(GlobalUtil::_MaxOrientation>1)
				otex->AttachToFBO(1);
			if(GlobalUtil::_SubpixelLocalization) sid = * stex;
			ShaderMan::UseShaderOrientation(gtex->GetTexID(),
				gtex->GetImgWidth(), gtex->GetImgHeight(), sigma,  
				sid, sigma_step);
			ftex->DrawQuad();
	//		glFinish();
			
		}
	}

	GLTexImage::UnbindMultiTex(2);
	if(GlobalUtil::_verboseS)glFinish();

	if(GlobalUtil::_MaxOrientation>1)	fbo.UnattachTex(GL_COLOR_ATTACHMENT1_EXT);

}



//to compare with GPU feature list generation
void PyramidNaive::GenerateFeatureListCPU()
{

	FrameBufferObject fbo;
	_featureNum = 0;
	_featureNumEX = 0;
	GLTexImage * tex = GetBaseLevel(_octave_min);
	float * mem = new float [tex->GetTexWidth()*tex->GetTexHeight()];
	vector<float> list;
	int idx = 0;
	for(int i = 0; i < _octave_num; i++)
	{
		for(int j = 0; j < param._dog_level_num; j++, idx++)
		{
			tex = GetBaseLevel(_octave_min + i, DATA_AUX) + j + 2;
			tex->BindTex();
			glGetTexImage(GlobalUtil::_texTarget, 0, GL_RED, GL_FLOAT, mem);
			//tex->AttachToFBO(0);
			//tex->FitTexViewPort();
			//glReadPixels(0, 0, tex->GetTexWidth(), tex->GetTexHeight(), GL_RED, GL_FLOAT, mem);
			//
			//make a list of 
			list.resize(0);
			float * p = mem;
			int fcount = 0 ;
			for(int k = 0; k < tex->GetTexHeight(); k++)
			{
				for( int m = 0; m < tex->GetTexWidth(); m ++, p++)
				{
					if(*p==0)continue;
					if(m ==0 || k ==0 || k >= tex->GetImgHeight() -1 || m >= tex->GetImgWidth() -1 ) continue;
					list.push_back(m+0.5f);
					list.push_back(k+0.5f);
					list.push_back(0);
					list.push_back(1);
					fcount ++;


				}
			}
			if(fcount==0)continue;


			
			GLTexImage * ftex = _featureTex+idx;
			_levelFeatureNum[idx] = (fcount);
			SetLevelFeatureNum(idx, fcount);

			_featureNumEX = max(_featureNumEX, _featureNum + ftex->GetPixelCount());
			_featureNum += (fcount);


			int fw = ftex->GetImgWidth();
			int fh = ftex->GetImgHeight();

			list.resize(4*fh*fw);

			ftex->BindTex();
			ftex->AttachToFBO(0);
	//		glTexImage2D(GlobalUtil::_texTarget, 0, GlobalUtil::_iTexFormat, fw, fh, 0, GL_BGRA, GL_FLOAT, &list[0]);
			glTexSubImage2D(GlobalUtil::_texTarget, 0, 0, 0, fw, fh, GL_RGBA, GL_FLOAT, &list[0]);
			//
		}
	}
	GLTexImage::UnbindTex();
	delete mem;
	if(GlobalUtil::_verbose)
	{
		std::cout<<"#Features:\t"<<_featureNum<<"\n";
	}
}


void PyramidNaive::ReshapeFeatureListCPU()
{
	//make a compact feature list
	//each wit only one orientation

	///download orientations list
	///download featue list
	//reshape it
	//upload it

	FrameBufferObject fbo;
	int i, szmax =0, sz;
	int n = param._dog_level_num*_octave_num;
	for( i = 0; i < n; i++)
	{
		sz = _featureTex[i].GetImgWidth() * _featureTex[i].GetImgHeight();
		if(sz > szmax ) szmax = sz;
	}
	float * buffer = new float[szmax*24];
	float * buffer1 = buffer;
	float * buffer2 = buffer + szmax*4;
	float * buffer3 = buffer + szmax*8;

	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);


	_featureNum = 0;
	_featureNumEX = 0;

	for(i = 0; i < n; i++)
	{
		if(_levelFeatureNum[i]==0)continue;

		_featureTex[i].AttachToFBO(0);
		_featureTex[i].FitTexViewPort();
		glReadPixels(0, 0, _featureTex[i].GetImgWidth(), _featureTex[i].GetImgHeight(),GL_RGBA, GL_FLOAT, buffer1);
		
		_orientationTex[i].AttachToFBO(0);
		glReadPixels(0, 0, _orientationTex[i].GetImgWidth(), _orientationTex[i].GetImgHeight(),GL_RGBA, GL_FLOAT, buffer2);
	
		int fcount =0, ocount;
		float * src = buffer1;
		float * orientation  = buffer2;
		float * des = buffer3;
		for(int j = 0; j < _levelFeatureNum[i]; j++, src+=4, orientation+=4)
		{
			ocount = (int)src[2];
			for(int k = 0 ; k < ocount; k++, des+=4)
			{
				des[0] = src[0];
				des[1] = src[1];
				des[2] = orientation[k];
				des[3] = 1;			
				fcount++;
			}
		}
		//texture size
		SetLevelFeatureNum(i, fcount);
		//
		int nfw = _featureTex[i].GetImgWidth();
		int nfh = _featureTex[i].GetImgHeight();
		int sz = nfh * nfw;
		for(int u = fcount; u < sz; u++, des+=4)
		{
			des[0] = des[1] = des[2] = des[3] = 0;
		}
		//glDrawPixels or gltexSubImage2D
		//_featureTex[i].AttachToFBO(0);
		//_featureTex[i].FitTexViewPort();
		//glRasterPos2i(0,0);
		//glDrawPixels(nfw, nfh, GL_RGBA, GL_FLOAT, buffer3);
		_featureTex[i].BindTex();
		glTexSubImage2D(GlobalUtil::_texTarget, 0, 0, 0, nfw, nfh, GL_RGBA, GL_FLOAT, buffer3);
		_featureTex[i].UnbindTex();

		_levelFeatureNum[i] = fcount;
		_featureNumEX = max(_featureNumEX, _featureNum + _featureTex[i].GetPixelCount());
		_featureNum += fcount;
	}

	delete[] buffer;
	if(GlobalUtil::_verbose)
	{
		std::cout<<"#Features MO:\t"<<_featureNum<<endl;
	}

}
void PyramidNaive::ReshapeFeatureList()
{

	//make a compact feature list
	//each wit only one orientation
	int i, wmax = 0, hmax=1, w = 1, h= 1;
	int n = param._dog_level_num*_octave_num;
	const double log4 = log(4.0);
	GLTexImage * tex;
	GLTexImage* ftex = _featureTex;
	GLTexImage* nftex = _featureTexAux;
	GLTexImage* otex = _orientationTex;
	//
	FrameBufferObject fbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

	int  idx = 0;

	_featureNum = 0;
	_featureNumEX = 0;

	for(i = 0; i < n; i++, ftex++, nftex++, otex++, idx++)
	{
		if(_levelFeatureNum[idx]==0)continue;
		int fw = ftex->GetImgWidth();
		int fh = ftex->GetImgHeight();
		int wlev, hlev;
		float fn[4];
		int fcount;
		//fw>=fh
		if(fw ==1)//fh must be 1
		{
			//read back??
			ftex->AttachToFBO(0);
			glReadPixels(0, 0, 1, 1, GL_RGBA , GL_FLOAT, fn);
			fcount = (int)fn[2];
			wlev  =	hlev = 0;
		}else
		{
			//reduction
			//find corresponding texture in histogram
			//
			wlev = ((int)(ceil(log((double)fw)/log4)));
			hlev = ((int)(ceil(log((double)fh)/log4)));
			//starting place
			tex = _fHistoTex +  _fhLevel + wlev -1;
			//output
			tex->AttachToFBO(0);
			tex->FitTexViewPort();
			//input
			ftex->BindTex();
			if(1 ==wlev &&hlev>0)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				glFinish();
			}

			//use program
			ShaderMan::UseShaderListReshapeInit();
		
			tex->DrawQuadReductionX(fh);	

			for(int j = 1; j < wlev; j++, tex--)
			{
				//reduction
				(tex-1)->AttachToFBO(0);
		
				//(tex-1)->FitTexViewPort();
				tex->BindTex();
				if(j ==wlev-1 && hlev>0)
				{
					glClear(GL_COLOR_BUFFER_BIT);
					glFinish();
				}
				ShaderMan::UseShaderGenListHisto();
				(tex-1)->DrawQuadReductionX(fh);
						
			}
			for(int k = 0; k < hlev; k++, tex--)
			{
				(tex-1)->AttachToFBO(0);
				tex->BindTex();
				//(tex-1)->FitTexViewPort();
				ShaderMan::UseShaderGenListHisto();
				(tex-1)->DrawQuadReductionY();
			}

			//glFinish();

			//build the list
			//read back one pixel
			glReadPixels(0, 0, 1, 1, GL_RGBA , GL_FLOAT, fn);
			fcount = int(fn[0] + fn[1] + fn[2] + fn[3]);
		}


		//set feature texture size. 
		//reallocation of texture
		SetLevelFeatureNumAux(idx, fcount);
		//
		_featureNum += fcount;	
		_levelFeatureNum[idx] = fcount;
		_featureNumEX = max(_featureNumEX, _featureNum + _featureTex[i].GetPixelCount());
		if(fcount <=0) 		continue;
		
		//use pyramid to generate new feature tex

		//first pass
		nftex->AttachToFBO(0);	
		nftex->FitTexViewPort();
		ShaderMan::UseShaderListReshapeStart(float(nftex->GetImgWidth()));
		nftex->DrawQuad();	
		nftex->DetachFBO(0);
		//pass on each pyramid level


		glActiveTexture(GL_TEXTURE0);
		nftex->BindTex();

		for(int hh = 0; hh < hlev; hh++, tex++)
		{
			//glActiveTexture(GL_TEXTURE0);
			//nftex->BindTex();
			nftex->AttachToFBO(0);
			glActiveTexture(GL_TEXTURE1);
			tex->BindTex();
			ShaderMan::UseShaderListReshapeStep(tex->GetTexID(), 0, 1);
			nftex->DrawQuad();
			nftex->DetachFBO(0);	//just to make sure it finishes before the next pass

		}
		for(int l = 0; l < wlev; l++, tex++)
		{

			//glActiveTexture(GL_TEXTURE0);
			//nftex->BindTex();
			nftex->AttachToFBO(0);
			glActiveTexture(GL_TEXTURE1);
			tex->BindTex();
			ShaderMan::UseShaderListReshapeStep(tex->GetTexID(), 1, 0);
			nftex->DrawQuad();
			nftex->DetachFBO(0);
			
			//imdebugTexImagef(GlobalUtil::_texTarget, *nftex, GL_RGBA);

		}
		
		//glActiveTexture(GL_TEXTURE0);
		//nftex->BindTex();
		nftex->AttachToFBO(0);
		glActiveTexture(GL_TEXTURE1);
		ftex->BindTex();
		glActiveTexture(GL_TEXTURE2);
		otex->BindTex();
		ShaderMan::UseShaderListReshapeEnd(ftex->GetTexID(), otex->GetTexID());
		nftex->DrawQuad();


		//imdebugTexImagef(GlobalUtil::_texTarget, *nftex, GL_RGBA);
		//need to reallocate?
		SetLevelFeatureNum(idx, fcount);
		GLTexImage::UnbindMultiTex(3);
	}
	//switch
	tex = _featureTexAux;
	_featureTexAux = _featureTex;
	_featureTex = tex;


	if(GlobalUtil::_verbose)
	{
		std::cout<<"#Features MO:\t"<<_featureNum<<endl;
	}
}

//

inline void PyramidNaive::ReallocateFeatureHistoTex(int wmax, int hmax)
{

	GLTexImage * tex;
	int w, h, i; 
	int wl =  ((int)(ceil(log((double)wmax)/log(4.0))));
	int hl =  ((int)(ceil(log((double)hmax)/log(4.0))));
	if( wl <= _fwLevel && hl <= _fhLevel ) return ;
	int num = _fwLevel + _fhLevel;
	_fwLevel = max( wl, _fwLevel);
	_fhLevel = max( hl, _fhLevel); 

	if(_fHistoTex)
	{
		delete [] _fHistoTex;
		if(GlobalUtil::_verbose) cout<<"Reallocate Multi-Orientation Histogram Texture\n";
	}
	_fHistoTex = new GLTexImage[_fwLevel+_fhLevel];	
	w = 1; h = 1;
	tex = _fHistoTex;
	//first
	tex ->InitTexture(w, h);
	tex->AttachToFBO(0);
	tex++;
	//levels
	for( i = 0; i< _fhLevel; i++, tex++)
	{
		h <<=2;
		if(h>hmax) h = hmax;
		tex->InitTexture(w, h, 0);
		tex->AttachToFBO(0);
	}
	for(i = 0; i < _fwLevel-1; i++, tex++)
	{
		w<<=2;
		if(w > wmax) w=wmax;
		tex->InitTexture(w, h, 0);
		tex->AttachToFBO(0);
	}	
}

inline void PyramidNaive::SetLevelFeatureNum(int idx, int fcount)
{
	int fw, fh;
	GLTexImage * ftex = _featureTex + idx;
	//set feature texture size. normally fh will be one
	GetTextureStorageSize(fcount, fw, fh);
	if(fcount >  ftex->GetTexWidth()*ftex->GetTexHeight())
	{
		if(GlobalUtil::_verbose)
			std::cout<<"Too many features, reallocate texture\n";

		ftex->InitTexture(fw, fh, 0);
		if(GlobalUtil::_MaxOrientation>1)
		{
			_orientationTex[idx].InitTexture(fw, fh, 0);
			ReallocateFeatureHistoTex(fw, fh);
		}

	}
	fw = fcount ==0? 0:(int)ceil(double(fcount)/fh);
	ftex->SetImageSize(fw, fh);
	if(GlobalUtil::_MaxOrientation > 1)
		_orientationTex[idx].SetImageSize(fw, fh);



}
void PyramidNaive::SetLevelFeatureNumAux(int idx, int fcount)
{
	int fw, fh;
	GLTexImage * ftex = _featureTexAux + idx;
	//set feature texture size. normally fh will be one
	GetTextureStorageSize(fcount, fw, fh);
	if(fcount > ftex->GetTexWidth()*ftex->GetTexHeight())
	{
		if(GlobalUtil::_verbose)
		{
			std::cout<<"Too many features, reallocate texture\n";
		}

		ftex->InitTexture(fw, fh, 0);
	}
	fw = fcount ==0? 0:(int)ceil(double(fcount)/fh);
	ftex->SetImageSize(fw, fh);

}
void PyramidNaive::GetSimplifiedOrientation()
{
	//
	int idx = 0;
	int n = _octave_num  * param._dog_level_num;
	float sigma; 
	GLTexImage * ftex = _featureTex;

	FrameBufferObject fbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	for(int i = 0; i < _octave_num; i++)
	{
		GLTexImage *gtex = GetLevelTexture(i+_octave_min, 2+param._level_min);
		for(int j = 0; j < param._dog_level_num; j++, ftex++,  gtex++, idx ++)
		{
			if(_levelFeatureNum[idx]<=0)continue;
			sigma = param.GetLevelSigma(j+param._level_min+1);
//			imdebugTexImagef(GlobalUtil::_texTarget, ftex->GetTexID(), GL_RGBA);
			//
			ftex->AttachToFBO(0);
			ftex->FitTexViewPort();

			glActiveTexture(GL_TEXTURE0);
			ftex->BindTex();
			glActiveTexture(GL_TEXTURE1);
			gtex->BindTex();

			ShaderMan::UseShaderSimpleOrientation(gtex->GetTexID(), sigma);
			ftex->DrawQuad();
			
//			imdebugTexImagef(GlobalUtil::_texTarget, ftex->GetTexID(), GL_RGBA);
		}
	}

	GLTexImage::UnbindMultiTex(2);

}

void PyramidNaive::GetFeatureDescriptors()
{
	//descriptors...
	float sigma;
	int idx, i, j, k,   w, h;
	int ndf = 32 / GlobalUtil::_DescriptorFormat;
	int n = _octave_num * param._dog_level_num;
	float* pd =  _download_result ?&_descriptor_buffer[0] : NULL;
	float* pbuf  = NULL, *ppd;
	vector<float>read_buffer;
	/*if ( _descriptorPBO == NULL)
	{
		//CUDA can write PBO for output.. not used so far
		_descriptorPBO = new unsigned int [n];
		glGenBuffers( n, _descriptorPBO );
	}
	*/
	GLTexImage * gtex, * ftex;
	ftex = _featureTex;
	FrameBufferObject fbo;

	GLenum buffers[8] = { 
		GL_COLOR_ATTACHMENT0_EXT,		GL_COLOR_ATTACHMENT1_EXT ,
		GL_COLOR_ATTACHMENT2_EXT,		GL_COLOR_ATTACHMENT3_EXT ,
		GL_COLOR_ATTACHMENT4_EXT,		GL_COLOR_ATTACHMENT5_EXT ,
		GL_COLOR_ATTACHMENT6_EXT,		GL_COLOR_ATTACHMENT7_EXT ,
	};
	ShaderMan::UnloadProgram();
	glDrawBuffers(ndf, buffers);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	for( i = 0, idx = 0; i < _octave_num; i++)
	{
		gtex = GetLevelTexture(i+_octave_min, param._level_min + GlobalUtil::_GradientLevelOffset);
		for( j = 0; j < param._dog_level_num; j++, ftex++, idx++, gtex++)
		{
			if(_levelFeatureNum[idx]==0)continue;

			sigma = param.GetLevelSigma(j+param._level_min+1);

			int count = _levelFeatureNum[idx]*GlobalUtil::_DescriptorFormat;
			GetAlignedStorageSize( count, _alignment, w, h);
			h = (int)ceil(double(count) / w);
			//
			for(k = 0; k < ndf; k++)
				_tmpFeatureTex[k].AttachToFBO(k);
			GlobalUtil::FitViewPort(w, h);
			glActiveTexture(GL_TEXTURE0);
			ftex->BindTex();
			glActiveTexture(GL_TEXTURE1);
			gtex->BindTex();

			ShaderMan::UseShaderDescriptor(gtex->GetTexID(),
				w, ftex->GetImgWidth(), 
				gtex->GetImgWidth(), gtex->GetImgHeight(), sigma);
			if(GlobalUtil::_DescriptorFormat ==32)
			{
				//glClear(GL_COLOR_BUFFER_BIT);
				//drawing point vertices when using geometry shader 
				DrawVertexBuffer(_levelFeatureNum[idx]);
			}else
			{
				GLTexImage::DrawQuad(0, (float)w, 0, (float)h);
			}


			if(_download_result )
			{	
				//download descriptor
				if(ndf ==1)
				{
					//WHEN geometry shader is used...simply read back
					glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pd);
				}else
				{
					int step = w*h*4;
					int np = GlobalUtil::_DescriptorFormat * _levelFeatureNum[idx];
					if((unsigned int)step*ndf > read_buffer.size())
					{
						read_buffer.resize(ndf*step);
					}
					pbuf = &read_buffer[0];
					
					//read back
					for(k = 0; k < ndf; k++, pbuf+=step)
					{
						glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + k);
						glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pbuf);
					}
					GlobalUtil::CheckErrorsGL();
					pbuf = &read_buffer[0];
					ppd = pd;

					//interlace
					if(ndf ==4)
					{
						float * pp;

						for(k = 0; k < np; k++)
						{
							pp = pbuf;
							for(int v = 0; v < 4; v++)
							{
								*ppd++ = pp[0];
								*ppd++ = pp[1];
								*ppd++ = pp[2];
								*ppd++ = pp[3];
								pp+=step;
							}
							pbuf += 4;
						}
					}else if(ndf ==8)
					{
						float * pp;
						for(k = 0; k < np; k++)
						{
							pp = pbuf;
							for(int v = 0; v < 8; v++)
							{
								*ppd++ = pp[0];
								*ppd++ = pp[1];
								*ppd++ = pp[2];
								*ppd++ = pp[3];
								pp+=step;
							}
							pbuf += 4;
						}

					}
					//need to do normalization
					ppd = pd;
					for(k = 0; k < _levelFeatureNum[idx]; k++)
					{
						float sq = 0;
						int v;
						//normalize
						for( v = 0; v < 128; v++, ppd++)
						{
							sq += (*ppd)*(*ppd);
						}
						sq = 1.0f / sqrtf(sq);
						ppd -= 128;
						//truncate to .2
						for(v = 0; v < 128; v ++, ppd++)
						{
							*ppd = min(*ppd*sq, 0.2f);
						}
						ppd -= 128;
						//renormalize
						sq = 0;
						for( v = 0; v < 128; v++, ppd++)
						{
							sq += (*ppd)*(*ppd);
						}
						sq = 1.0f / sqrtf(sq);
						ppd -= 128;
						for(v = 0; v < 128; v ++, ppd++)
						{
							*ppd = *ppd*sq;
						}
					}
					pd += 128*_levelFeatureNum[idx];
				}
				glReadBuffer(GL_NONE);
				GlobalUtil::CheckErrorsGL();
			}else
			{
				glFinish();
			}
		}
	}
	GLTexImage::UnbindMultiTex(2); 
	glDrawBuffer(GL_NONE);

	ShaderMan::UnloadProgram();
	if(GlobalUtil::_verboseS)glFinish();
	for(i = 0; i < ndf; i++)
	{
		fbo.UnattachTex(GL_COLOR_ATTACHMENT0_EXT +i);
	}
}


void PyramidNaive::PrepareBuffer()
{
	if(_download_result )
	{
		_keypoint_buffer.resize(4 * _featureNumEX +1);

		if(GlobalUtil::_ComputeDescriptors)
		{
			_descriptor_buffer.resize(128*_featureNumEX +1);
		}

	}
	else
	{
		_keypoint_buffer.resize(0);
		_descriptor_buffer.resize(0);
	}
}

void PyramidNaive::DownloadKeypoints()
{
	//download keypoints..
	if(_download_result ==  0 ) return;
	const double twopi = 2.0*3.14159265358979323846;
	int idx = 0;
	float * buffer = &_keypoint_buffer[0];
	float * p = buffer, *ps, sigma;
	GLTexImage * ftex = _featureTex;
	FrameBufferObject fbo;
	ftex->FitRealTexViewPort();
	float os = _octave_min>=0? float(1<<_octave_min): 1.0f/(1<<(-_octave_min));

	if(_down_sample_factor>0) os *= float(1<<_down_sample_factor); 

	float offset = GlobalUtil::_LoweOrigin? 0 : 0.5f;
	for(int i = 0; i < _octave_num; i++, os *= 2.0f)
	{
		
		for(int j = 0; j  < param._dog_level_num; j++, idx++, ftex++)
		{

			if(_levelFeatureNum[idx]>0)
			{	
				ftex->AttachToFBO(0);
				glReadPixels(0, 0, ftex->GetImgWidth(), ftex->GetImgHeight(),GL_RGBA, GL_FLOAT, p);
				ps = p;
				for(int k = 0;  k < _levelFeatureNum[idx]; k++, ps+=4)
				{
					ps[0] = os*(ps[0]-0.5f) + offset;	//x
					ps[1] = os*(ps[1]-0.5f) + offset;	//y
					sigma = os*ps[3]; 
					ps[3] = (float)fmod(twopi-ps[2], twopi);	//orientation, mirrored
					ps[2] = sigma;  //scale

				}
				p+= 4* _levelFeatureNum[idx];
			}
		}
	}
}

void PyramidNaive::CopyFeatureVector(float*keys, float *descriptors)
{
	if(keys)		memcpy(keys, &_keypoint_buffer[0], 4*_featureNum*sizeof(float));
	if(descriptors)	memcpy(descriptors, &_descriptor_buffer[0], 128*_featureNum*sizeof(float));
}

void PyramidNaive::SaveSIFT(std::ostream & out)
{

	if (_featureNum <=0) return;
	float * pk = &_keypoint_buffer[0];
	if(GlobalUtil::_ComputeDescriptors)
	{
		float * pd = &_descriptor_buffer[0] ;
		out<<_featureNum<<" 128"<<endl;

		for(int i = 0; i < _featureNum; i++)
		{
			//in y, x, scale, orientation order
			out<<pk[1]<<" "<<pk[0]<<" "<<pk[2]<<" " << pk[3]<< endl; //endl suggested by Martin Schneider
			pk+=4;
			for(int k = 0; k < 128; k ++, pd++) 
			{
				out<< ((unsigned int)floor(0.5+512.0f*(*pd)))<<" ";

				if ( (k+1)%20 == 0 ) out<<endl; //suggested by Martin Schneider
			}
			out<<endl;

		}
	
	}else
	{
		out<<_featureNum<<" 0"<<endl;
		for(int i = 0; i < _featureNum; i++)
		{
			out<<pk[1]<<" "<<pk[0]<<" "<<pk[2]<<" " << pk[3]<<endl;
		}
	}
}