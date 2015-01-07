#ifndef __IMAGEVECTORIZER_H
#define __IMAGEVECTORIZER_H

#include "cv.h"			
#include "ofxCvColorImage.h"
#include "cxcore.h"		
#include "ofConstants.h"
#include <vector>
#include <math.h>
//#include "ml.h"
#include <stdlib.h>
#include "msImageProcessor.h"


class imageVectorizer
{
public:
	// receieve as input the image to vectorize
	imageVectorizer(unsigned char *img, int width, int height, int depth = 8, float scale = 1, int channels = 3);
	//imageVectorizer(IplImage * img, int width, int height, int depth = 8, int channels = 3);
	~imageVectorizer();

	// calls the mean-shift segment routine
	void segment();
	
	// get the IplImage (must be allocated)
	void getResult(IplImage *img);
	void getResultUnscaled(IplImage *img);

	// find edges of labeled regions
	void findLines();

	// group lines into polygons
	void findPolygons();


	IplImage				*cvImg;						// The input image
	IplImage				*cvImgSc;					// Scaled input image
	IplImage				*cvImgMs;					// The filtered image

	byte					*imgDataSc;					// Scaled Image Data as an unsigned char
	ofxCvColorImage			ofxCvImg;

	msImageProcessor		msImgFilterer;

	int						n_width, n_height;			// Image dimensions
	int						n_depth, n_channels;		// depth = 8 for char, channels = 3 for rgb
	float					n_scale;
	
	bool					useEdison;

};

#endif