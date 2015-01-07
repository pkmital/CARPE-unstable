#include "imageVectorizer.h"
#include "cv.h"			
#include "cxcore.h"	
//#include "highgui.h"
#include "ofConstants.h"
#include <vector>
#include <math.h>
//#include "ml.h"
#include <stdlib.h>
#include "ms.h"


imageVectorizer::imageVectorizer(
	unsigned char *img, 
	int width, 
	int height,
	int depth,
	float scale,
	int channels)
	:	n_width(width),
		n_height(height),
		n_depth(depth),
		n_scale(scale),
		n_channels(channels)
{
	// Copy input image into the IplImage
	cvImg = cvCreateImage(cvSize(width, height), depth, channels);
	for( int i = 0; i < height; i++ ) {
		memcpy( cvImg->imageData+(i*cvImg->widthStep), img+(i*width*channels), width*channels);
	}
	
	useEdison = true;
	
	// Scale image using bilinear interpolation
	cvImgSc = cvCreateImage(cvSize(width/scale, height/scale), depth, channels);
	cvImgMs = cvCreateImage(cvSize(width/scale, height/scale), depth, channels);
	cvResize(cvImg, cvImgSc); // CV_INTER_NN
	
	// Allocate memory for the filtered image
	//cvImgMs = cvCreateImage(cvSize(cvImgSc->width, cvImgSc->height), depth, channels);

	if(useEdison)
	{
		imgDataSc = new byte[cvImgSc->width*cvImgSc->height*channels];

		// Copy into byte array
		for( int i = 0; i < cvImgMs->height; i++ )
		{
			memcpy( imgDataSc+(i*cvImgSc->width*channels), cvImgSc->imageData+(i*cvImgMs->widthStep), cvImgSc->width*channels);
		}
	}
}

imageVectorizer::~imageVectorizer()
{
	cvReleaseImage(&cvImg);
	cvReleaseImage(&cvImgSc);
	cvReleaseImage(&cvImgMs);
}

void imageVectorizer::segment()
{
	if(useEdison)
	{
		msImgFilterer.DefineImage(imgDataSc, COLOR, cvImgSc->height, cvImgSc->width);
		msImgFilterer.Segment(2, 2, 29, HIGH_SPEEDUP);

		IplImage *msResult = cvCreateImage(cvSize(cvImgMs->width, cvImgMs->height), cvImgMs->depth, n_channels);
		msImgFilterer.GetResults(imgDataSc);

		// Copy from byte array
		for( int i = 0; i < cvImgMs->height; i++ )
		{
			memcpy( cvImgMs->imageData+(i*cvImgMs->widthStep), imgDataSc+(i*cvImgMs->width*n_channels), cvImgMs->width*n_channels);
		}
	}
	else
	{
		// Mean shift filter
		//cvDilate(cvImgSc, cvImgSc, NULL, 3);
		//cvErode(cvImgSc, cvImgMs, NULL, 3);
		cvPyrMeanShiftFiltering(cvImgSc, cvImgMs, 20, 40, 3, cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,10,1));
	}
}

void imageVectorizer::getResult(IplImage *img)
{
	//IplImage *tempImg = cvCreateImage(cvSize(img->width,img->height), n_depth, n_channels);
	cvResize(cvImgMs, img, 1);
}

void imageVectorizer::getResultUnscaled(IplImage *img)
{
	cvResize(img,cvImgMs);
}


// find edges of labeled regions
void imageVectorizer::findLines()
{
	if(useEdison)
	{
		//msImgFilterer.GetBoundaries()
	}
}

// group lines into polygons
void imageVectorizer::findPolygons()
{
	int numRegions = 0;
	if(useEdison)
	{
		//numRegions = msImgFilterer.GetRegions(<#int ** #>, <#float ** #>, <#int ** #>)
	}

}