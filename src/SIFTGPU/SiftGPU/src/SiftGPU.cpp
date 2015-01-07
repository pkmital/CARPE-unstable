////////////////////////////////////////////////////////////////////////////
//	File:		SiftGPU.cpp
//	Author:		Changchang Wu
//	Description :	Implementation of the SIFTGPU classes.
//					SiftGPU:	The SiftGPU Tool.  
//					SiftGPUEX:	SiftGPU + viewer
//					SiftParam:	Sift Parameters
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
#include <algorithm>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
using namespace std;
#include "stdlib.h"
#include "time.h"
#include "GlobalUtil.h"
#include "SiftGPU.h"
#include "IL/il.h"
#include "GL/glut.h"
#include "GLTexImage.h"
#include "ShaderMan.h"
#include "FrameBufferObject.h"
#include "SiftPyramid.h"
#include "math.h"


#if defined(HIGH_RESOLUTION_TIME) && defined(_WIN32)
	#include <windows.h>
	#include "mmsystem.h"
#endif

////
#ifdef _WIN32
	#include "direct.h"
	#include "imdebug/imdebuggl.h"
	#pragma comment(lib, "../lib/DevIL.lib")
	#pragma warning (disable : 4786) 
#else
	//compatible with linux
	#define _stricmp strcasecmp
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//
//just want to make this class invisible
class ImageList:public std::vector<std::string> {};

SiftGPU::SiftGPU(int np)
{ 
	_texImage = new GLTexImage;
	_imgpath[0] = 0;
	_outpath[0] = 0;
	_initialized = 0;
	_image_loaded = 0;
	 GlobalUtil::_UseSiftGPUEX = 0;
	_current = 0;
	_glut_id = 0;
	_list = new ImageList();
	
	_nPyramid = max(1, np);
	_pyramids = new SiftPyramid*[_nPyramid];
	for(int i = 0; i < _nPyramid; i++)
	{
		_pyramids[i] = new PyramidNaive(*this);
	}
	_pyramid =  _pyramids[0];
	_download_result = GlobalUtil::_DownloadDefault;

}

SiftGPUEX::SiftGPUEX() 
{
	_view = _sub_view = _view_abs_diff = 0;
	_view_debug = 0;
	_download_result = 0; //do not download result by default;
	GlobalUtil::_UseSiftGPUEX = 1;
	srand(time(NULL));
	RandomizeColor();
}

void SiftGPU::SetActivePyramid(int index)
{
	if(index >=0 && index < _nPyramid)	
	{
		_pyramid = _pyramids[index];
		_pyramid->EnableDownload(_download_result);
	}
}

void SiftGPUEX::RandomizeColor()
{
	//
	float hsv[3] = {0, 0.8, 1.0};
	for(int i = 0; i < COLOR_NUM*3; i+=3)
	{
		hsv[0] = (rand()%100)*0.01; //i/float(COLOR_NUM);
		HSVtoRGB(hsv, _colors+i);		
	}
}
SiftGPU::~SiftGPU()
{
	for(int i = 0; i < _nPyramid; i++)
	{
		delete _pyramids[i];
	}
	delete _pyramids;
	delete _texImage;
	delete _list;

	if(_initialized)
	{
		//destroy all the shaders?
		ShaderMan::DestroyShaders(_sigma_num);
		//shutdown iamge loader
		ilShutDown();
	}

	if(_glut_id >0) glutDestroyWindow(_glut_id);
}


inline void SiftGPU::InitSiftGPU()
{
	if(_initialized || GlobalUtil::_HaveGlSupport ==0) return;

	ilInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
	///

	//init opengl parameters
	InitGLParam();

	if(!GlobalUtil::_SupportFP40)
	{
		GlobalUtil::_FilterWidthFactor = 3.0; // for running limited demo on my laptop
	}

	//sifr parameters
	ParseSiftParam();

	if(GlobalUtil::_HaveGlSupport)
	{
		//load shaders..
		GlobalUtil::StartTimer("Load Fixed Shaders");
		ShaderMan::InitializeShaders();
		GlobalUtil::StopTimer();

		GlobalUtil::StartTimer("Load dynamic Shaders");
		ShaderMan::LoadDogShaders(_dog_threshold, _edge_threshold);
		ShaderMan::LoadGenListShader(_dog_level_num, 0);
		GlobalUtil::StopTimer();

		GlobalUtil::StartTimer("Make Gaus. Filters");
		ShaderMan::CreateGaussianFilters(*this);
		GlobalUtil::StopTimer();


		if(GlobalUtil::_InitPyramidWidth >0 && GlobalUtil::_InitPyramidHeight >0)
		{
			GlobalUtil::StartTimer("Initialize Pyramids");
			for(int i = 0; i < _nPyramid; i++)
			{
				_pyramid[i].InitPyramid(GlobalUtil::_InitPyramidWidth,
										GlobalUtil::_InitPyramidHeight, 0);
			}
			GlobalUtil::StopTimer();
		}
	}


#if defined(HIGH_RESOLUTION_TIME) && defined(_WIN32)
	timeBeginPeriod(1);
#endif

	_initialized = 1;

//	std::cout<<setprecision(5);
}

int	 SiftGPU::RunSIFT(int index)
{
	if(_list->size()>0 )
	{
		index = index % _list->size();
		if(strcmp(_imgpath, _list->at(index).data()))
		{
			strcpy(_imgpath, _list->at(index).data());
			_image_loaded = 0;
			_current = index;
		}
		return RunSIFT();
	}else
	{
		return 0;
	}

}
int  SiftGPU::RunSIFT( int width,  int height, unsigned char * data, unsigned int gl_format)
{

	if(GlobalUtil::_HaveGlSupport ==0 ) return 0;
	if(!_initialized) InitSiftGPU();
	if(GlobalUtil::_HaveGlSupport ==0 ) return 0;

	if(width > 0 && height >0 && data != NULL)
	{
		_imgpath[0] = 0;
		//try downsample on CPU
		int ds ; 
		GlobalUtil::StartTimer("Upload Image data");
		if(_texImage->SetImageData(width, height, data, gl_format, ds))
		{
			_image_loaded = 2; //gldata;
			GlobalUtil::StopTimer();
			_timing[0] = GlobalUtil::GetElapsedTime();
			
			//if the size of image is different
			//pyramid need to be reallocated.
			GlobalUtil::StartTimer("Initialize Pyramid");
			_pyramid->InitPyramid(width, height, ds);
			GlobalUtil::StopTimer();
			_timing[1] = GlobalUtil::GetElapsedTime();

			return RunSIFT();
		}else
		{
			return 0;
		}
	}else
	{
		return 0;
	}



}
int  SiftGPU::RunSIFT(char * imgpath)
{
	if(imgpath && imgpath[0])
	{
		if(strcmp(_imgpath, imgpath))
		{
			//set the new image
			strcpy(_imgpath, imgpath);
			_image_loaded = 0;
		}
		return RunSIFT();
	}else
	{
		return 0;
	}


}

int SiftGPU::RunSIFT()
{
	if(_imgpath[0]==0 && _image_loaded == 0) return 0;
	if(GlobalUtil::_HaveGlSupport ==0 ) return 0;

	ClockTimer timer("\0");

	//initialize SIFT GPU for once
	if(!_initialized)InitSiftGPU();

	timer.StartTimer("RUN SIFT");
	//process input image file
	if( _image_loaded ==0)
	{
		//load image
		GlobalUtil::StartTimer("Load Input Image");

		//try to down-sample on cpu
		int width, height, ds; 

		if(_texImage->LoadImageFile(_imgpath, width, height, ds)==0)
		{
			return 0;
		}

		_image_loaded = 1;

		GlobalUtil::StopTimer();
		_timing[0] = GlobalUtil::GetElapsedTime();

		//make sure the pyrmid can hold the new image.
		GlobalUtil::StartTimer("Initialize Pyramid");
		_pyramid->InitPyramid(width, height, ds);
		GlobalUtil::StopTimer();
		_timing[1] = GlobalUtil::GetElapsedTime();

	}else
	{
		//change some global states
		GlobalUtil::FitViewPort(1,1);
		_texImage->FitTexViewPort();
		if(_image_loaded == 1)
		{
			_timing[0] = _timing[1] = 0;
		}else
		{//2
			_image_loaded = 1; 
		}
	}

	if(_pyramid->_allocated ==0 ) return 0;

	///prepare the buffer for dowloading data to cpu
	int is_downloading = _pyramid->IsDownloadEnabled();
	if(is_downloading ==0 && _outpath[0]) _pyramid->EnableDownload(1);

	//process the image
	_pyramid->RunSIFT(_texImage);
	_pyramid->GetPyramidTiming(_timing + 2); //

	//write output once if there is only one input
	if(_outpath[0] )
	{
		SaveSIFT(_outpath);
		_outpath[0] = 0;
		_pyramid->EnableDownload(is_downloading);
		
		//if you just want to call TestWin(Glut) as a sift processor
		if(GlobalUtil::_ExiftAfterSIFT && GlobalUtil::_UseSiftGPUEX) exit(0); 
	}


	if(GlobalUtil::_UseSiftGPUEX)
	{
		GlobalUtil::StartTimer("Gen. Display VBO");
		_pyramid->GenerateFeatureDisplayVBO();
		GlobalUtil::StopTimer();
		_timing[9] = GlobalUtil::GetElapsedTime();
	}else
	{
		_timing[9] = 0;
	} 

	GLTexImage::UnbindMultiTex(3);
	ShaderMan::UnloadProgram();
	FrameBufferObject::DeleteGlobalFBO();
	timer.StopTimer();
	if(GlobalUtil::_verbose)std::cout<<endl;




	return 1;
}




void SiftGPUEX::DisplayInput() 
{
	if(_texImage==NULL) return;
	_texImage->BindTex();
	_texImage->DrawQuad();
	_texImage->UnbindTex();

}

void SiftGPU::GetImageDimension( int &w,  int &h)
{
	w = _texImage->GetImgWidth();
	h = _texImage->GetImgHeight();

}

void SiftGPU::EnableOutput(int enable)
{
	_pyramid->EnableDownload( enable);
	_download_result = enable;
}

void SiftGPU::SetVerbose(int verbose)
{
	if(verbose ==-1)
	{
		//Loop between verbose level 0, 1, 2, 3
		if(GlobalUtil::_verbose)
		{
			GlobalUtil::_verbose  = GlobalUtil::_verboseS;
			GlobalUtil::_verboseS = 0;
			GlobalUtil::_verboseO = 0;
			GlobalUtil::_verboseL = 0;
			GlobalUtil::_verboseT = 0;
			return;
		}else
		{
			verbose = 2;
		}
	}
	GlobalUtil::_verbose = verbose>0;
	GlobalUtil::_verboseS = verbose>1;
	GlobalUtil::_verboseO = verbose>2;
	GlobalUtil::_verboseL = verbose>3;
	GlobalUtil::_verboseT = verbose>4;
}


void SiftGPU::InitGLParam()
{
	if(GlobalUtil::_HaveGlSupport != -1) return;

	glewInit();
	const char * vendor = (const char * )glGetString(GL_VENDOR);
	GlobalUtil::_IsATI = strstr(vendor, "ATI") !=NULL? 1:0;
	std::cout<<"GPU Vendor: "<< vendor <<endl;

	if (glewGetExtension("GL_ARB_fragment_shader")    != GL_TRUE ||
		glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
		glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
		glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
	{
		std::cerr<<"Shader is not well supported by your hardware\n";
		GlobalUtil::_HaveGlSupport = 0;
		return;
	}

	if (glewGetExtension("GL_EXT_framebuffer_object") != GL_TRUE) 
	{
		std::cerr<< "Driver does not support Framebuffer Objects (GL_EXT_framebuffer_object)"<<endl;
		GlobalUtil::_HaveGlSupport = 0;
		return;
	}
	
	if(glewGetExtension("GL_ARB_texture_rectangle")==GL_TRUE)
	{
		GlobalUtil::_texTarget =  GL_TEXTURE_RECTANGLE_ARB;
		glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &GlobalUtil::_texMaxDimGL);
		std::cout<<"GL_TEXTURE_RECTANGLE_ARB\n";
		std::cout<<"GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT: "<<GlobalUtil::_texMaxDimGL<<"x"<< GlobalUtil::_texMaxDimGL<<endl;

		if(GlobalUtil::_texMaxDim == 0 || GlobalUtil::_texMaxDim > GlobalUtil::_texMaxDimGL)
		{
			GlobalUtil::_texMaxDim = GlobalUtil::_texMaxDimGL; 
		}
		glEnable(GlobalUtil::_texTarget);
	}else
	{
		std::cerr<<"GL_ARB_texture_rectangle is not supported\n";
		GlobalUtil::_HaveGlSupport = 0;
		return;
	}

	if(glewGetExtension("GL_ARB_vertex_buffer_object")==GL_TRUE)
	{
		std::cout<<("GL_ARB_vertex_buffer_object\n");
	}

	//if(glewGetExtension("GL_NV_fence")==GL_TRUE)
	//{
	//	std::cout<<("GL_NV_fence\n");
	//}
	if(glewGetExtension("GL_EXT_geometry_shader4")==GL_TRUE)
	{
		GlobalUtil::_SupportGeometryShader = 1;
		std::cout<<"GL_EXT_geometry_shader4\n";
	}else if(glewGetExtension("GL_NV_geometry_shader4")==GL_TRUE)
	{
		GlobalUtil::_SupportGeometryShader = 1;
		std::cout<<"GL_NV_geometry_shader4\n";
	} 
	int value;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &value);
	std::cout<<"GL_MAX_TEXTURE_UNITS:\t\t"<<value<<endl;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &value);
	std::cout<<"GL_MAX_TEXTURE_UNITS_ARB:\t"<<value<<endl;
	glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &value);
	std::cout<<"GL_MAX_TEXTURE_COORDS_ARB:\t"<< value<<endl;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &value);
	GlobalUtil::_MaxDrawBuffers = value;
	std::cout<<"GL_MAX_DRAW_BUFFERS_ARB:\t"<< value<<endl;
	
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &value);
	std::cout<<"GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:\t\t"<<value<<endl;
	GlobalUtil::_MaxInstruction = value;

	std::cout<<"\n[Shader Language]:\t"<<(GlobalUtil::_UseGLSL?"GLSL" : "CG")<<"\n\n";

	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT, GL_FILL);
	//change some global parameter according to hardware


	//
	ShaderMan::InitShaderContext();
	//

	GlobalUtil::SetTextureParameter();

	GlobalUtil::_HaveGlSupport = 1;
}
SiftParam::SiftParam()
{

	_level_min = -1;
	_dog_level_num  = 3;
	_level_max = 0;
	_sigma0 = 0;
	_sigman = 0;
	_edge_threshold = 0;
	_dog_threshold =  0;


}

float SiftParam::GetInitialSmoothSigma(int octave_min)
{
	float	sa = _sigma0 * powf(2.0f, float(_level_min)/float(_dog_level_num)) ; 
	float   sb = _sigman / powf(2.0f,   octave_min) ;//
	float sigma_skip0 = sa>sb+ 0.001?sqrt(sa*sa - sb*sb): 0.0f;
	return sigma_skip0; 
}

void SiftParam::ParseSiftParam()
{ 

	if(_dog_level_num ==0) _dog_level_num = 3;
	if(_level_max ==0) _level_max = _dog_level_num + 1;
	if(_sigma0 ==0.0f) _sigma0 = 1.6 * powf(2.0f, 1.0f / _dog_level_num) ;
	if(_sigman == 0.0f) _sigman = 0.5f;


	_level_num = _level_max -_level_min + 1;

	_level_ds  = _level_min + _dog_level_num;
	if(_level_ds > _level_max ) _level_ds = _level_max ;


	///
	float _sigmak = powf(2.0f, 1.0 / _dog_level_num) ;
	float dsigma0 = _sigma0 * sqrt (1.0f - 1.0f / (_sigmak*_sigmak) ) ;
	float sa, sb;

 
	sa = _sigma0 * powf(_sigmak, _level_min) ; 
	sb = _sigman / powf(2.0f,   GlobalUtil::_octave_min_default) ;//

	_sigma_skip0 = sa>sb+ 0.001?sqrt(sa*sa - sb*sb): 0.0f;

    sa = _sigma0 * powf(_sigmak, _level_min      ) ;
    sb = _sigma0 * powf(_sigmak, _level_ds - _dog_level_num ) ;

	_sigma_skip1 = sa>sb + 0.001? sqrt(sa*sa - sb*sb): 0.0f;

	_sigma_num = _level_max - _level_min;
	_sigma = new float[_sigma_num];

	for(int i = _level_min + 1; i <= _level_max; i++)
	{
		_sigma[i-_level_min -1] =  dsigma0 * powf(_sigmak, i) ;
	}

	if(_dog_threshold ==0)	_dog_threshold      = 0.02f / _dog_level_num ;
	if(_edge_threshold==0) _edge_threshold		= 10.0f;
}


void SiftGPUEX::DisplayOctave(int i)
{
	if(_pyramid == NULL)return;
	const int grid_sz = (int)ceil(_level_num/2.0);
	double scale = 1.0/grid_sz ;
	int gx=0, gy=0;

	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));


	i = i% _pyramid->_octave_num;  //
	if(i<0 ) i+= _pyramid->_octave_num;

	scale *= ( 1<<(i));

	GLTexImage * tex = _pyramid->GetBaseLevel(i+_pyramid->_octave_min);

	int dx = tex->GetImgWidth();
	int dy = tex->GetImgHeight();


	ShaderMan::UseShaderDisplayGaussian();

	glPushMatrix();
	glScalef(scale, scale, scale);
	for(int level = _level_min; level<= _level_max; level++, tex++)
	{
		glPushMatrix();

		glTranslatef(dx*gx, dy*gy, 0);

		tex->BindTex();

		tex->DrawQuad();
		tex->UnbindTex();

		glPopMatrix();

		gx++;
		if(gx>=grid_sz) 
		{
			gx =0;
			gy++;
		}

	}

	glPopMatrix();
	ShaderMan::UnloadProgram();
}

void SiftGPUEX::DisplayPyramid( void (*UseDisplayShader)(), int dataName, int nskip1, int nskip2)
{

	if(_pyramid == NULL)return;
	int grid_sz = (_level_num -nskip1 - nskip2);
	if(grid_sz > 4) grid_sz = (int)ceil(grid_sz*0.5);
	double scale = 1.0/grid_sz;
	int stepx, stepy, dx, dy=0, nstep;

	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));


	glPushMatrix();
	glScalef(scale, scale, scale);

	for(int i = _pyramid->_octave_min; i < _pyramid->_octave_min+_pyramid->_octave_num; i++)
	{
		GLTexImage * tex = _pyramid->GetBaseLevel(i, dataName);
		tex += nskip1;
		stepx = tex->GetImgWidth();
		stepy = tex->GetImgHeight();		
		nstep = i==_pyramid->_octave_min? grid_sz: _level_num;
		dx = 0;
		UseDisplayShader();
		for(int j = _level_min + nskip1; j <= _level_max-nskip2; j++, tex++)
		{
			if(j == _level_min + nskip1 + nstep)
			{
				dy += stepy;
				dx = 0;
			}

			glPushMatrix();
			glTranslatef(dx, dy, 0);
			tex->BindTex();
			tex->DrawQuad();
			tex->UnbindTex();
			glPopMatrix();

			dx += stepx;

		}

		ShaderMan::UnloadProgram();

		dy+= stepy;
	}

	glPopMatrix();
}


void SiftGPUEX::DisplayLevel(int i)
{
	if(_pyramid == NULL)return;

	i = i%(_level_num * _pyramid->_octave_num);
	if (i<0 ) i+= (_level_num * _pyramid->_octave_num);
	int octave = _pyramid->_octave_min + i/_level_num;
	int level  = _level_min + i%_level_num;
	double scale = 1.0;

	if(octave >0) scale *= (1<<octave);
	else if(octave < 0) scale /= (1<<(-octave));


	GLTexImage * tex = _pyramid->GetLevelTexture(octave, level);


	ShaderMan::UseShaderDisplayGaussian();

	glPushMatrix();
	glScalef(scale, scale, scale);
	tex->BindTex();
	tex->DrawQuad();
	tex->UnbindTex();
	glPopMatrix();
	ShaderMan::UnloadProgram();
}

void SiftGPUEX::DisplaySIFT()
{
	if(_view_debug)
	{
		DisplayDebug();
		return;
	}
	switch(_view)
	{
	case 0:
		DisplayInput();
		DisplayFeatureBox(_sub_view);
		break;
	case 1:
		DisplayPyramid(ShaderMan::UseShaderDisplayGaussian, SiftPyramid::DATA_GAUSSIAN);
		break;
	case 2:
		DisplayOctave(_sub_view);	
		break;
	case 3:
		DisplayLevel(_sub_view);
		break;
	case 4:
		DisplayPyramid(_view_abs_diff? ShaderMan::UseShaderDisplayDOGABS: ShaderMan::UseShaderDisplayDOG, 
			SiftPyramid::DATA_DOG, 1);
		break;
	case 5:
		DisplayPyramid(_view_abs_diff? ShaderMan::UseShaderDisplayGradXABS: ShaderMan::UseShaderDisplayGradX,
			SiftPyramid::DATA_GRADX, 1);
		break;
	case 6:
		DisplayPyramid(_view_abs_diff? ShaderMan::UseShaderDisplayGradYABS: ShaderMan::UseShaderDisplayGradY,
			SiftPyramid::DATA_GRADY, 1);
		break;
	case 7:
//		DisplayPyramid(ShaderMan::UseShaderDisplayGaussian, 2, 1);
		DisplayPyramid(ShaderMan::UseShaderDisplayDOG, SiftPyramid::DATA_DOG,2, 1);
		DisplayPyramid(ShaderMan::UseShaderDisplayKeypoints, SiftPyramid::DATA_KEYPOINT, 2,1);
	}
}
void SiftGPUEX::ChangeViewDiff()
{
	_view_abs_diff = ! _view_abs_diff;
}

void SiftGPUEX::SetView(int view, int sub_view, char *title)
{
	const char* view_titles[] =
	{
		"Original Image",
		"Gaussian Pyramid",
		"Octave Images",
		"Level Image",
		"Difference of Gaussian",
		"Horizontal Gradient",
		"Vertical Gradient",
		"Keypoints"
	};
	const int view_num = 8;
	_view = view % view_num;
	if(_view <0) _view +=view_num;
	_sub_view = sub_view;

	if(_view_debug)
		strcpy(title, "Debug...");
	else
		strcpy(title, view_titles[_view]);

}


void SiftGPU::PrintUsage()
{
	std::cout
	<<"SiftGPU Usage:\n"
	<<"-h -help       : Help\n"
	<<"-i <filename>  : Input image\n"
	<<"-il <filename> : Input a file that contains a list of filenames\n"
	<<"-o <filename>  : Output file\n"
	<<"-f <Factor>    : Filter width will be 2*factor+1\n"
	<<"                 (default : 3.0)\n"
	<<"-w  <factor>   : Orientation window will be 2*factor\n"
	<<"                 (default : 3.0)\n"
	<<"-dw  <factor>  : Descriptor grid size (default : 3.0)\n"
	<<"-fo <first>    : First Octave (default: 0)\n"
	<<"-no <num>      : Maximum number of Octaves (default: no limit)\n"
	<<"-d <dog levels>: DOG levels in an octave (default: 3)\n"
	<<"-t <threshold> : DOG threshold (default: 0.02/3)\n"
	<<"-e <threshold> : Edge Threshold (default : 10.0)\n"
	<<"-m -mo <num>   : Multi Feature Orientations\n"
	<<"                 (default : single orientation)\n"
	<<"-s             : Sub-Pixel, Sub-Scale Localization\n"
	<<"-lcpu  -lc     : CPU Feature List Generation (defaut : gpu)\n"
	<<"-rcpu  -rc     : CPU Feature List Reshape (defaut : gpu)\n"
	<<"-prep  -ic     : Convert RGB to Luminace and down-sample on CPU before uploading to GPU\n"
	<<"                 (defaut : Upload raw data from file )\n"
	<<"-fbo1/-fbos    : Use single/Multiple FBOs for whole work\n"
	<<"-olddog        : Force using old version of detection code\n"
	<<"-sd            : Skip descriptor computation if specified\n"
	<<"-da            : Pre-allocate Depth Texture\n"  
	<<"-dg            : Use Geometry Shader for Descriptor \n" 
	<<"-fs <size>     : Block Size for freature storage <default : 1>\n"
	<<"-glsl          : Use GLSL instead of CG (default : CG)\n"
	<<"-tight         : Automatically resize pyramid to fit new images\n"
	<<"-p  WxH        : Inititialize the pyramids to size WxH\n"
	<<"-lm  <number>  : Maximum feature number for a level\n"
	<<"-lmp <percent> : Maximum feature percent for a level\n"
	<<"-v <verbose>   : Same as calling Setverbose() function\n"
	<<"-download      : Always download computation result\n"
	<<"-ofix          : Fixed orientation for all the features\n"
	<<"-loweo         : (0,0) at center of top-left pixel (defaut: corner)\n"
	<<"-maxd <size>   : Max working dimension (default : 2048)\n"
	<<"-exit          : Exit program after processing the input image\n"
	<<"\n";
}
void SiftGPU::ParseParam(int argc, char **argv)
{
	char* arg, *param;
	char* opt;
	int  HelpPrinted = 0;
	int  i = 0;
	for( i = 0; i< argc; i++)
	{
		arg = argv[i];
		if(arg[0]!='-')continue;
		opt = arg+1;
		param = argv[i+1];
		if(_stricmp(opt, "h")==0 || _stricmp(opt,"help")==0)
		{
			HelpPrinted = 1;
			PrintUsage();
		}else if(_stricmp(opt, "glsl")==0)
		{
			GlobalUtil::_UseGLSL = 1;
		}else if(_stricmp(opt, "pack")==0)
		{
			//use packed version ?
			//not supported yet
		}else if(_stricmp(opt, "lcpu")==0||_stricmp(opt, "lc")==0)
		{
			GlobalUtil::_ListGenGPU = 0;
		}else if(_stricmp(opt, "rcpu")==0||_stricmp(opt, "rc")==0)
		{
			GlobalUtil::_ListReshapeGPU = 0;
		}else if(_stricmp(opt, "prep")==0)
		{
			GlobalUtil::_PreProcessOnCPU = 1;
		}else if(_stricmp(opt, "noprep")==0)
		{
			GlobalUtil::_PreProcessOnCPU = 0;
		}else  if(_stricmp(opt, "fbo1")==0)
		{
			FrameBufferObject::UseSingleFBO =1;

		}else  if(_stricmp(opt, "fbos")==0)
		{
			FrameBufferObject::UseSingleFBO = 0;
		}
		else if(_stricmp(opt, "sd")==0)
		{
			GlobalUtil::_ComputeDescriptors =0;
		}else if(_stricmp(opt, "olddog")==0)
		{
			GlobalUtil::_UseNewDogAlgorithm = 0;
		}else if(_stricmp(opt, "tight")==0)
		{
			GlobalUtil::_ForceTightPyramid = 1;
		}else if(_stricmp(opt, "download")==0)
		{
			SiftGPU::_download_result = 1;
		}else if(_stricmp(opt, "exit")==0)
		{
			GlobalUtil::_ExiftAfterSIFT = 1;
		}
		else if(_stricmp(opt, "mrt8")==0)
		{
			GlobalUtil::_MRT8 =1;
		}else if(_stricmp(opt, "edge")==0)
		{
			GlobalUtil::_EdgeSupression = 0;
		}else if(_stricmp(opt, "ov")==0)
		{
			//for compatibility with old version myself//
			GlobalUtil::_GradientLevelOffset = 2; 
			GlobalUtil::_OrientationExtraFactor = 1.5;
			GlobalUtil::_OrientationGaussianFactor = 1.0; 
		
		}else if(_stricmp(opt, "da")==0)
		{
			if(i+1<argc && _stricmp(param, "0")==0)
			{
				GlobalUtil::_DepthTexAllocation = 0;
			}else
			{
				GlobalUtil::_DepthTexAllocation =1;
			}
		}else if(_stricmp(opt, "m")==0 || _stricmp(opt, "mo")==0)
		{
			int mo = 0;
			if(i+1 <argc)
			{
				sscanf(param, "%d", &mo);
			}
			//at least two orientation
			if(mo <2)mo = 2;
			GlobalUtil::_MaxOrientation = mo;
		}else if(_stricmp(opt, "s") ==0)
		{
			GlobalUtil::_SubpixelLocalization = 1;
		}else if(_stricmp(opt, "extra_downsample") ==0)
		{
			GlobalUtil::_ExtraDownSample = 1;
		}
		else if(_stricmp(opt, "dg")==0)
		{
			GlobalUtil::_UseGeometryShader =1;
		}else if(_stricmp(opt, "ofix")==0)
		{
			GlobalUtil::_FixedOrientation = 1;
		}else if(_stricmp(opt, "loweo")==0)
		{
			GlobalUtil::_LoweOrigin = 1;
		}else if(i+1>=argc)
		{
			//make sure there is the param			
		}else if(_stricmp(opt, "i")==0)
		{
			strcpy(_imgpath, param);
			i++;
			//get the file list..
			_list->push_back(param);
			while( i+1 < argc && argv[i+1][0] !='-')
			{
				_list->push_back(argv[++i]);
			}
		}else if(_stricmp(opt, "il")==0)
		{
			LoadImageList(param);
			i++;
		}else if( _stricmp(opt, "o")==0)
		{
			strcpy(_outpath, param);
			i++;
		}else if( _stricmp(opt, "f")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_FilterWidthFactor  = factor;
				i++;
			}
		}else if(_stricmp(opt, "w")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_OrientationWindowFactor  = factor;
				i++;
			}
		}else if(_stricmp(opt, "dw")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_DescriptorWindowFactor  = factor;
				i++;
			}
		}else if(_stricmp(opt, "fo")==0)
		{

			int first_octave;
			sscanf(param, "%d", &first_octave);
			if(first_octave >=-2 )
			{
				GlobalUtil::_octave_min_default = first_octave;
				i++;
			}

		}else if(_stricmp(opt, "no")==0)
		{

			int octave_num=-1;
			sscanf(param, "%d", &octave_num);
			if(octave_num<=0) octave_num = -1;

			if(octave_num ==-1 || octave_num >=1)
			{
				GlobalUtil::_octave_num_default = octave_num;
				i++;
			}

		}else if( _stricmp(opt, "t")==0)
		{

			float threshold;
			sscanf(param, "%f", &threshold);
			if(threshold >0 && threshold < 0.5)
			{
				SiftParam::_dog_threshold = threshold;
				i++;
			}
		}else if(_stricmp(opt, "e")==0 )
		{
			float threshold;
			sscanf(param, "%f", &threshold);
			if(threshold >0 )
			{
				SiftParam::_edge_threshold = threshold;
				i++;
			}
		}else if(_stricmp(opt, "d")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1 && num <=10)
			{
				SiftParam::_dog_level_num = num;
				i++;
			}

		}else if(_stricmp(opt, "fs")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1)
			{
				GlobalParam::_FeatureTexBlock = num;
				i++;
			}

		}else if(_stricmp(opt, "p")==0)
		{
			int w =0, h=0;
			sscanf(param, "%dx%d", &w, &h);
			if(w >0 &&  h>0)
			{
				GlobalParam::_InitPyramidWidth = w;
				GlobalParam::_InitPyramidHeight = h;
			}
		}else if(_stricmp(opt, "levelmax")==0 || _stricmp(opt, "lm")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1000)
			{
				GlobalParam::_MaxLevelFeatureNum = num;
				i++;
			}
		}else if(_stricmp(opt, "levelmaxpercent")==0 || _stricmp(opt, "lmp")==0)
		{
			float num;
			sscanf(param, "%f", &num);
			if(num >=0.001)
			{
				GlobalParam::_MaxFeaturePercent = num;
				i++;
			}
		}else if(_stricmp(opt, "v")==0 )
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=0 && num <=5)
			{
				SetVerbose(num);
			}
		}else if(_stricmp(opt, "maxd")==0 )
		{
			int num;
			sscanf(param, "%d", &num);
			if(num > 0)
			{
				GlobalUtil::_texMaxDim = num; 
				
			}
		}
	}
	if(_imgpath[0] ==0 )
	{
		if(HelpPrinted ==0)std::cout<<"Use -help to get instructions"<<endl;
	}
	//do not write result if there are more than one input images
	if(_outpath[0] && _list->size()>1)
	{
		_outpath[0] = 0;
	}
	if(GlobalUtil::_FixedOrientation) GlobalUtil::_MaxOrientation = 1;
} 

void SiftGPU::SetImageList(int nimage, char** filelist)
{
	_list->resize(0);
	for(int i = 0; i < nimage; i++)
	{
		_list->push_back(filelist[i]);
	}
	_current = 0;

}
void SiftGPU:: LoadImageList(char *imlist)
{
	char filename[_MAX_PATH];
	ifstream in(imlist);
	while(in>>filename)
	{
		_list->push_back(filename);
	}
	in.close();


	if(_list->size()>0)
	{
		strcpy(_imgpath, _list->at(0).data());
		strcpy(filename, imlist);
		char * slash = strrchr(filename, '\\');
		if(slash == 0) slash = strrchr(filename, '/');
		if(slash )
		{
			slash[1] = 0;
			chdir(filename);
		}
	}
	_image_loaded = 0;


}
float SiftParam::GetLevelSigma( int lev)
{
	return _sigma0 * powf( 2.0f,  double(lev) / double(_dog_level_num )); //bug fix 9/12/2007
}

void SiftGPUEX::DisplayFeatureBox(int view )
{
	view = view%3;
	if(view<0)view+=3;
	if(view ==2) return;
	int idx = 0;
	const int *fnum = _pyramid->GetLevelFeatureNum();
	const unsigned int *vbo = _pyramid->GetFeatureDipslayVBO();
	const unsigned int *vbop = _pyramid->GetPointDisplayVBO();
	int  nvbo = _dog_level_num * _pyramid->_octave_num;
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPushMatrix();
//	glTranslatef(0.0f, 0.0f, -1.0f);
	glPointSize(2.0f);

	float scale = 1.0f;
	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));
	glScalef(scale, scale, 1.0f);


	for(int i = 0; i < _pyramid->_octave_num; i++)
	{

		for(int j = 0; j < _dog_level_num; j++, idx++)
		{
			if(fnum[idx]>0)
			{
				if(view ==0)
				{
					glColor3f(0.2f, 1.0f, 0.2f);
					glBindBuffer(GL_ARRAY_BUFFER_ARB, vbop[idx]);
					glVertexPointer( 4, GL_FLOAT,4*sizeof(float), (char *) 0);
					glDrawArrays( GL_POINTS, 0, fnum[idx]);
					glFlush();
				}else
				{
						
					//glColor3f(1.0f, 0.0f, 0.0f);
					glColor3fv(_colors+ (idx%COLOR_NUM)*3);
					glBindBuffer(GL_ARRAY_BUFFER_ARB, vbo[idx]);
					glVertexPointer( 4, GL_FLOAT,4*sizeof(float), (char *) 0);
					glDrawArrays( GL_LINES, 0, fnum[idx]*10 );
					glFlush();
				}

			}
		
		}
		glTranslatef(-.5f, -.5f, 0.0f);
		glScalef(2.0f, 2.0f, 1.0f);

	}
	glPopMatrix();
	glDisableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPointSize(1.0f);
							
}

void SiftGPUEX::ToggleDisplayDebug()
{
	_view_debug = !_view_debug;
}

void SiftGPUEX::DisplayDebug()
{
	glPointSize(1.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	ShaderMan::UseShaderDebug();
	glBegin(GL_POINTS);
	for(int i = 0; i < 100; i++)
	{
		glVertex2f(i*4.0f+0.5f, i*4.0f+0.5f);
	}
	glEnd();
	ShaderMan::UnloadProgram();
}


int SiftGPU::CreateContextGL()
{
	//use GLUT to create an OpenGL Context?
	if(_glut_id == 0)
	{
		int argc = 1;
		char * argv = "-iconic";
		glutInit(&argc, &argv);
		glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
		glutInitWindowSize (600,450);
		_glut_id = glutCreateWindow ("SIFT_GPU");

		glutHideWindow();
	}

	if(0 == VerifyContextGL())//shaders and other stuff are not supported
	{
		glutDestroyWindow(_glut_id);
		_glut_id = 0;
		return 0;
	}else
	{
		return GlobalUtil::_HaveGlSupport + GlobalUtil::_SupportFP40;
	}
}

int SiftGPU::VerifyContextGL()
{
	GlobalUtil::_HaveGlSupport = -1;  //unknown
	InitGLParam();
	return GlobalUtil::_HaveGlSupport + GlobalUtil::_SupportFP40;
}

int SiftGPU::IsFullSupported()
{
	return GlobalUtil::_HaveGlSupport==1 && GlobalUtil::_SupportFP40==1;
}

void SiftGPU::SaveSIFT(char * szFileName)
{
	std::ofstream out(szFileName);
	_pyramid->SaveSIFT(out);
	out.close();
}

int SiftGPU::GetFeatureNum()
{
	return _pyramid->GetFeatureNum();
}

void SiftGPU::GetFeatureVector(SiftKeypoint * keys, float * descriptors)
{
//	keys.resize(_pyramid->GetFeatureNum());
	if(GlobalUtil::_ComputeDescriptors)
	{
	//	descriptors.resize(128*_pyramid->GetFeatureNum());
		_pyramid->CopyFeatureVector((float*) (&keys[0]), &descriptors[0]);
	}else
	{
		//descriptors.resize(0);
		_pyramid->CopyFeatureVector((float*) (&keys[0]), NULL);
	}
}

void SiftGPU::SetTightPyramid(int tight)
{
	GlobalUtil::_ForceTightPyramid = tight;
}

int SiftGPU::AllocatePyramid(int width, int height)
{
	_pyramid->_down_sample_factor = 0;
	_pyramid->_octave_min = GlobalUtil::_octave_min_default;
	if(GlobalUtil::_octave_min_default>=0)
	{
		width >>= GlobalUtil::_octave_min_default;
		height >>= GlobalUtil::_octave_min_default;
	}else
	{
		width <<= (-GlobalUtil::_octave_min_default);
		height <<= (-GlobalUtil::_octave_min_default);
	}
	_pyramid->ResizePyramid(width, height);
	return _pyramid->_pyramid_height == height && width == _pyramid->_pyramid_width ;
}
void SiftGPU::SetMaxDimension(int sz)
{
	if(sz < GlobalUtil::_texMaxDimGL)
	{
		GlobalUtil::_texMaxDim = sz;
	}
}
int SiftGPU::GetImageCount()
{
	return _list->size();
}
void SiftGPUEX::HSVtoRGB(float hsv[3],float rgb[3] )
{

	int i;
	float q, t, p;
	float hh,f, v = hsv[2];
	if(hsv[1]==0.0f)
	{
		rgb[0]=rgb[1]=rgb[2]=v;

	}
	else
	{
		//////////////
		hh =hsv[0]*6.0f ;   // sector 0 to 5
		i =(int)hh ;
		f = hh- i;   // factorial part of h
		//////////
		p=  v * ( 1 - hsv[1] );
		q = v * ( 1 - hsv[1] * f );
		t = v * ( 1 - hsv[1] * ( 1 - f ) );
		switch( i ) {
			case 0:rgb[0] = v;rgb[1] = t;rgb[2] = p;break;
			case 1:rgb[0] = q;rgb[1] = v;rgb[2] = p;break;
			case 2:rgb[0] = p;rgb[1] = v;rgb[2] = t;break;
			case 3:rgb[0] = p;rgb[1] = q;rgb[2] = v;break;
			case 4:rgb[0] = t;rgb[1] = p;rgb[2] = v;break;
			case 5:rgb[0] = v;rgb[1] = p;rgb[2] = q;break;
			default:rgb[0]= 0;rgb[1] = 0;rgb[2] = 0;
		}
	}

}

SiftGPU* CreateNewSiftGPU(int np)
{
	return new SiftGPU(np);
}