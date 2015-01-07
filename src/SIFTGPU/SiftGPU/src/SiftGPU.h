////////////////////////////////////////////////////////////////////////////
//	File:		SiftGPU.h
//	Author:		Changchang Wu
//	Description :	interface for the SIFTGPU class.
//					SiftGPU:	The SiftGPU Tool.  
//					SiftGPUEX:	SiftGPU + viewer
//					SiftParam:	Sift Parameters
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


#if !defined(GPU_SIFT_H)
#define GPU_SIFT_H

#ifdef _WIN32
	#ifdef SIFTGPU_DLL
		#ifdef DLL_EXPORT
			#define SIFTGPU_EXPORT __declspec(dllexport)
		#else
			#define SIFTGPU_EXPORT __declspec(dllimport)
		#endif
	#else
		#define SIFTGPU_EXPORT
	#endif

	#if _MSC_VER > 1000
		#pragma once
	#endif
#else
	#define SIFTGPU_EXPORT
#endif

#if !defined(_MAX_PATH)
	#if defined (MAX_PATH)
		#define _MAX_PATH MAX_PATH
	#else
		#define _MAX_PATH 512
	#endif
#endif
///////////////////////////////////////////////////////////////////
//clss SiftParam
//description: SIFT parameters
////////////////////////////////////////////////////////////////////
class GlobalUtil;
class SiftParam
{
public:
	float*		_sigma;
	float		_sigma_skip0; // 
	float		_sigma_skip1; //
	//sigma of the first level
	float		_sigma0;
	float		_sigman;
	int			_sigma_num;

	//how many dog_level in an octave
	int			_dog_level_num;
	int			_level_num;


	//starting level in an octave
	int			_level_min;
	int			_level_max;
	int			_level_ds;
	//dog threshold
	float		_dog_threshold;
	//edge elimination
	float		_edge_threshold;
	void		 ParseSiftParam();
public:
	float GetLevelSigma(int lev);
	float GetInitialSmoothSigma(int octave_min);
	SIFTGPU_EXPORT SiftParam();
};



class GLTexImage;
class ShaderMan;
class SiftPyramid;
class ImageList;
////////////////////////////////////////////////////////////////
//class SIftGPU
//description: Interface of SiftGPU lib
////////////////////////////////////////////////////////////////
class SiftGPU:public SiftParam
{
public:
	enum
	{
		SIFTGPU_NOT_SUPPORTED = 0,
		SIFTGPU_PARTIAL_SUPPORTED = 1,
		SIFTGPU_FULL_SUPPORTED = 2,
	};
	typedef struct SiftKeypoint
	{
		float x, y, s, o;
	}SiftKeypoint;
protected: 
	//when more than one images are specified
	//_current indicates the active one
	int		_current;
	//_initialized indicates if the shaders and OpenGL/SIFT parameters are initialized
	//they are initialized only once for one SiftGPU inistance
	//that is, SIFT parameters will not be changed
	int		_initialized;
	//if SIFT result is to be downloaded
	int		_download_result;
	//_image_loaded indicates if the current images are loaded
	int		_image_loaded;
	//the name of current input image
	char	_imgpath[_MAX_PATH];
	//_outpath containes the name of the output file
	char	_outpath[_MAX_PATH];
	//the list of image filenames
	ImageList *    _list;
	//the texture that holds loaded input image
	GLTexImage *   _texImage;
	//the SiftPyramid
	SiftPyramid *  _pyramid;
	SiftPyramid ** _pyramids;
	int			   _nPyramid;
	///
	int			   _glut_id;
	//

	//protected functions
	//initialize Opengl parameters
	static void InitGLParam();
	//print out the command line options
	static void PrintUsage();
	//Initialize OpenGL and SIFT paremeters, and create the shaders accordingly
	void InitSiftGPU();
	//load the image list from a file
	void LoadImageList(char *imlist);
public:
	//timing results for 10 steps
	float			_timing[10];
public:
	//set the image list for processing
	SIFTGPU_EXPORT virtual void SetImageList(int nimage, char** filelist);
	//get the number of SIFT features in current image
	SIFTGPU_EXPORT virtual int	GetFeatureNum();
	//save the SIFT result as a ANSCII file 
	SIFTGPU_EXPORT virtual void SaveSIFT(char * szFileName);
	//Copy the SIFT result to two vectors
	SIFTGPU_EXPORT virtual void GetFeatureVector(SiftKeypoint * keys, float * descriptors);
	//Enable downloading results to CPU. 
	SIFTGPU_EXPORT virtual void EnableOutput(int enable =1);
	//create a new OpenGL context for processing
	SIFTGPU_EXPORT virtual int CreateContextGL();
	//verify the current opengl context..
	//(for example, you call wglmakecurrent yourself and verify the current context)
	SIFTGPU_EXPORT virtual int VerifyContextGL();
	//check if all siftgpu functions are supported
	SIFTGPU_EXPORT virtual int IsFullSupported();
	//set verbose mode
	SIFTGPU_EXPORT virtual void SetVerbose(int verbose = 5);
	//set SiftGPU to brief display mode, which is faster
	inline void SetVerboseBrief(){SetVerbose(2);};
	//parse SiftGPU parameters
	SIFTGPU_EXPORT virtual void ParseParam(int argc, char **argv);
	//retrieve the size of current input image
	SIFTGPU_EXPORT virtual void GetImageDimension(int &w, int&h);
	//run SIFT on a new image given filename
	SIFTGPU_EXPORT virtual int  RunSIFT(char * imgpath);
	//run SIFT on an image in the image list given the file index
	SIFTGPU_EXPORT virtual int	RunSIFT(int index);
	//run SIFT on a new image given pixel data and format;
	SIFTGPU_EXPORT virtual int  RunSIFT(int width, int height, unsigned char * data, unsigned int gl_format);
	//run SIFT on current image (specified by arguments), or processing the current image again
	SIFTGPU_EXPORT virtual int  RunSIFT();
	//constructor, (np is the number of pyramid)
	SIFTGPU_EXPORT SiftGPU(int np = 1);
	//destructor
	SIFTGPU_EXPORT virtual ~SiftGPU();
	//set the active pyramid
	SIFTGPU_EXPORT virtual void  SetActivePyramid(int index);
	//retrieve the number of images in the image list
	SIFTGPU_EXPORT virtual int GetImageCount();
	//set parameter GlobalUtil::_ForceTightPyramid
	SIFTGPU_EXPORT virtual void SetTightPyramid(int tight = 1);
	//allocate pyramid for a given size of image
	SIFTGPU_EXPORT virtual int AllocatePyramid(int width, int height);
	//none of the texture in processing can be larger
	//automatic down-sample is used if necessary
	SIFTGPU_EXPORT virtual void SetMaxDimension(int sz);
private:

};


SIFTGPU_EXPORT SiftGPU * CreateNewSiftGPU(int np =1);
////////////////////////////////////////////////////////////////
//class SIftGPUEX
//description: add viewing extension to Interface of SiftGPU
////////////////////////////////////////////////////////////////

class SiftGPUEX:public SiftGPU
{
	//view mode
	int	_view;
	//sub view mode
	int _sub_view;
	//different view mode
	int _view_abs_diff;
	//whether display a debug view
	int _view_debug;
	//colors for SIFT feature display
	enum{COLOR_NUM = 36,};
	float _colors[COLOR_NUM*3];
	//display functions
	void DisplayInput();	//display gray level image of input image	
	void DisplayDebug();	//display debug view
	void DisplayFeatureBox(int i);	//display SIFT features
	void DisplayLevel(int i);		//display one level image
	void DisplayOctave(int i);		//display all images in one octave
	//display different content of Pyramid by specifying different data and display shader
	//the first nskip1 levels and the last nskip2 levels are skiped in display
	void DisplayPyramid( void (*UseDisplayShader)(), int dataName, int nskip1 = 0, int nskip2 = 0);
	//use HSVtoRGB to generate random colors
	static void 	HSVtoRGB(float hsv[3],float rgb[3] );
public:
	SIFTGPU_EXPORT SiftGPUEX();
	//change view mode
	SIFTGPU_EXPORT void SetView(int view, int sub_view, char * title);
	//display current view
	SIFTGPU_EXPORT void DisplaySIFT();
	//toggle debug mode on/off
	SIFTGPU_EXPORT void ToggleDisplayDebug();
	//absolute difference or offset difference
	SIFTGPU_EXPORT void ChangeViewDiff();
	//randomize the display colors
	SIFTGPU_EXPORT void RandomizeColor();
};


#endif 
