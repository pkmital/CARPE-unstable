////////////////////////////////////////////////////////////////////////////
//	File:		SiftPyramid.h
//	Author:		Changchang Wu
//	Description : interface for the SiftPyramid class.
//		SiftPyramid:	data storage for SIFT
//						base class of PyramidNaive and PyramidPacked
//		PyramidNaive:	Unpacked version of SIFT storage
//		PyramidPacked:	packed version of SIFT storage (not implemented yet)
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



#ifndef _SIFT_PYRAMID_H
#define _SIFT_PYRAMID_H
#include <vector>
#include <ostream>
using namespace std;


class GLTexImage;
class SiftParam;
class ProgramGPU;
class FilterProgram;
class ShaderMan;
class GlobalUtil;

/////////////////////////////////////////////////////////////////////////////
//class SiftPyramid
//description: virutal class of SIFT data pyramid
//			   provides functions for SiftPU to run steps of GPU SIFT
//			   currently, class PyramidNaive is the only real implementation
/////////////////////////////////////////////////////////////////////////////


class SiftPyramid : public GlobalUtil
{
public:
	enum{
		DATA_GAUSSIAN,
		DATA_DOG,
		DATA_GRADX,
		DATA_GRADY,
		DATA_KEYPOINT,
		DATA_AUX,
	};
protected:
	SiftParam&	 param;
	int			 _hpLevelNum;
	GLTexImage * _histoPyramidTex;
	GLTexImage * _featureTex;
	GLTexImage * _featureTexAux;
	GLTexImage * _fHistoTex;
	GLTexImage * _tmpFeatureTex;
	GLTexImage * _orientationTex;
	int			_fhLevel;
	int			_fwLevel;
	int*		_levelFeatureNum;
	int			_featureNum;
	int			_featureNumEX;
	unsigned int* _descriptorPBO;
	unsigned int* _featureDisplayVBO;
	unsigned int* _featurePointVBO;

	GLuint		   *_depthTex;
	int			 _download_result;
public:
	//
	float		_timing[7];
	//image size related
	//first octave
	int			_octave_min;
	//how many octaves
	int			_octave_num;
	//pyramid storage
	int			_pyramid_octave_num;
	int			_pyramid_octave_first;
	int			_pyramid_width;
	int			_pyramid_height;
	int			_down_sample_factor;
	int			_allocated; 
public:

	virtual void RunSIFT(GLTexImage*input){};
	virtual void DestroyPyramid(){};
	virtual void SaveSIFT(std::ostream & out){};
	virtual void CopyFeatureVector(float*keys, float *descriptors){};
	int			 GetFeatureNum(){return _featureNum;}
	virtual void DownloadKeypoints()=0;
	virtual void PrepareBuffer()=0;
	virtual void GetFeatureDescriptors()=0;
	virtual void GenerateFeatureListCPU()=0;
	virtual void ReshapeFeatureList()=0;
	virtual void ReshapeFeatureListCPU()=0;
	virtual void GenerateFeatureList()=0;
	virtual void GenerateFeatureDisplayVBOonCPU(){};//CPU version
	virtual void GenerateFeatureDisplayVBO()=0;
	virtual GLTexImage* GetLevelTexture(int octave, int level)=0;
	virtual GLTexImage* GetBaseLevel(int octave, int dataName = DATA_GAUSSIAN)=0;
	virtual void BuildPyramid(GLTexImage * input)=0;
	virtual void ResizePyramid(int w, int h) = 0;
	virtual void InitPyramid(int w, int h, int ds = 0)=0;
	virtual void DetectKeypoints()=0;
	virtual void DetectKeypointsEX(){};
	virtual void GetFeatureOrienations(){};
	virtual void GetSimplifiedOrientation(){};
	int GetHistLevelNum(){return _hpLevelNum;}
	const unsigned int * GetFeatureDipslayVBO(){return _featureDisplayVBO;}
	const unsigned int * GetPointDisplayVBO(){return _featurePointVBO;}
	const int * GetLevelFeatureNum(){return _levelFeatureNum;}
	void EnableDownload(int enable) {_download_result = enable;}
	int  IsDownloadEnabled(){return _download_result;}
	SiftPyramid(SiftParam&sp):param(sp)
	{
		_featureNum = 0;
		_featureNumEX = 0;
		_featureDisplayVBO = 0;
		_featurePointVBO = 0;
		_descriptorPBO = 0;
		_levelFeatureNum = NULL;
		_featureTex = NULL;
		_orientationTex = NULL;
		_featureTexAux = NULL;
		_tmpFeatureTex = NULL;
		_fHistoTex = NULL;
		_histoPyramidTex = NULL;
		_hpLevelNum = 0;
		_fhLevel = 0;
		_fwLevel = 0;
		_depthTex = NULL;



		//image size
		_octave_num = 0;
		_octave_min = 0;
		_pyramid_octave_num = _pyramid_octave_first = 0;
		_pyramid_width = _pyramid_height = 0;
		_allocated = 0;
		_download_result = GlobalUtil::_DownloadDefault;
		_down_sample_factor = 0;



	}
	virtual ~SiftPyramid()	{	}
	void		GetPyramidTiming(float * timing)
	{
		for(int i = 0; i < 7; i++) timing[i] = _timing[i];
	}
};
class PyramidNaive:public SiftPyramid, public ShaderMan  
{
protected:
	GLTexImage * _texPyramid;
	GLTexImage * _auxPyramid;
	unsigned int _vBufferID;
	int		     _alignment;
	vector<float>	_keypoint_buffer;
	vector<float>	_descriptor_buffer;
public:
	virtual void RunSIFT(GLTexImage*input);
	void SaveSIFT(std::ostream & out);
	void CopyFeatureVector(float*keys, float *descriptors);
	void DownloadKeypoints();
	void PrepareBuffer();
	void DestroyPyramid();
	void DestroyPyramidData();
	virtual void GetFeatureDescriptors();
	void GetSimplifiedOrientation();
	void SetLevelFeatureNum(int idx, int num);
	void ReallocateFeatureHistoTex(int fw, int fh);
	void SetLevelFeatureNumAux(int idx, int num);
	void GetTextureStorageSize(int num, int &fw, int& fh);
	void GetAlignedStorageSize(int num, int align, int &fw, int &fh);
	void ReshapeFeatureListCPU();
	void ReshapeFeatureList();
	void GenerateFeatureListCPU();
	virtual void GetFeatureOrienations();
	virtual void GenerateFeatureDisplayVBO();
	virtual void GenerateFeatureList();
	void DetectKeypoints();
	void DetectKeypointsEX();
	void BuildVertexBuffer(int count);
	GLTexImage* GetLevelTexture(int octave, int level);
	GLTexImage* GetBaseLevel(int octave, int dataName = DATA_GAUSSIAN);
	void DrawVertexBuffer(int count);
	void BuildPyramid(GLTexImage * input);
	inline void InitPyramid(int w, int h, int ds);
	void FitPyramid(int w, int h);
	void ResizePyramid(int w, int h);
	PyramidNaive(SiftParam & sp);
	~PyramidNaive();
};


//this version is not done yet
/*
class PyramidPacked:public SiftPyramid, public ShaderMan
{
	GLTexPacked * _texPyramid;
	GLTexPacked * _auxPyramid;
public:
};
*/
#endif 
