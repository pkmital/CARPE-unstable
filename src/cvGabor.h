//cvGabor.h

//-------------------BEGIN -------------------------
#ifndef CVGABOR_H
#define CVGABOR_H

#include <iostream>

#ifndef round
#define round(x) ((int)((float)x+0.5f))
#endif

#include <cv.h>
#include <highgui.h>

#define PI 3.14159265
#define CV_GABOR_REAL 1
#define CV_GABOR_IMAG 2
#define CV_GABOR_MAG  3
#define CV_GABOR_PHASE 4

/**
@author Mian Zhou
*/
class CvGabor{
public:
	CvGabor();
	~CvGabor();

	CvGabor(int iMu, int iNu, double dSigma);
	CvGabor(int iMu, int iNu, double dSigma, double dF);
	CvGabor(double dPhi, int iNu);
	CvGabor(double dPhi, int iNu, double dSigma);
	CvGabor(double dPhi, int iNu, double dSigma, double dF);

	bool IsInit();

	long mask_width();
	IplImage* get_image(int Type);
	bool IsKernelCreate();
	long get_mask_width();

	void Init(int iMu, int iNu, double dSigma, double dF);
	void Init(double dPhi, int iNu, double dSigma, double dF);

	void output_file(const char *filename, int Type);
	CvMat* get_matrix(int Type);
	void show(int Type);
	void conv_img(IplImage *src, IplImage *dst, int Type);
	CvGabor(int iMu, int iNu);
	void normalize( const CvArr* src, CvArr* dst, double a, double b, int norm_type, const CvArr* mask );
	void conv_img_a(IplImage *src, IplImage *dst, int Type);

protected:
	double Sigma;
	double F;
	double Kmax;
	double K;
	double Phi;
	bool bInitialised;
	bool bKernel;
	long Width;
	CvMat *Imag;
	CvMat *Real;

private:
	void creat_kernel();


};

#endif 

/*
The example is

Include the header first

#include "cvgabor.h"

To create a Gabor with the orientation PI/4 and the scale is 3.

double Sigma = 2*PI;
double F = sqrt(2.0);
CvGabor *gabor1 = new CvGabor;
gabor1->Init(PI/4, 3, Sigma, F);

Get the real part of the gabor and display it

IplImage *kernel = cvCreateImage( cvSize(gabor1->get_mask_width(), gabor1->get_mask_width()), IPL_DEPTH_8U, 1);
kernel = gabor1->get_image(CV_GABOR_REAL);
cvNamedWindow("Gabor Kernel", 1);
cvShowImage("Gabor Kernel", kernel);
cvWaitKey(0);

Load an Image and display it.

IplImage *img = cvLoadImage( "/local/FaceDB/CMU/rotated/217.jpg", CV_LOAD_IMAGE_GRAYSCALE );
cvNamedWindow("Original Image", 1);
cvShowImage("Original Image", img);
cvWaitKey(0);

Get the real response of gabor filtering from the loaded Image and display the real response

IplImage *reimg = cvCreateImage(cvSize(img->width,img->height), IPL_DEPTH_8U, 1);
gabor1->conv_img(img, reimg, CV_GABOR_REAL);
cvNamedWindow("Real Response", 1);
cvShowImage("Real Response",reimg);
cvWaitKey(0);
cvDestroyWindow("Real Response");

Get the imaginary response of gabor filtering from the loaded image and display it

IplImage *reimg = cvCreateImage(cvSize(img->width,img->height), IPL_DEPTH_8U, 1);
gabor1->conv_img(img, reimg, CV_GABOR_IMAG);
cvNamedWindow("Imaginary Response", 1);
cvShowImage("Imaginary Response",reimg);
cvWaitKey(0);
cvDestroyWindow("Imaginary Response");

Get the maginitude response of gabor filtering from the loaded image and display it

IplImage *reimg = cvCreateImage(cvSize(img->width,img->height), IPL_DEPTH_8U, 1);
gabor1->conv_img(img, reimg, CV_GABOR_MAG);
cvNamedWindow("Magnitude Response", 1);
cvShowImage("Magnitude Response",reimg);
cvWaitKey(0);
cvDestroyWindow("Magnitude Response");

This response has been sampled into 8-bit greyscale image. If you want to the original float point data, you can do

IplImage *reimg = cvCreateImage(cvSize(img->width,img->height), IPL_DEPTH_32F, 1);
gabor1->conv_img(img, reimg, CV_GABOR_MAG);

However, the image can not be displayed, but it can be saved into an XML file.

cvSave( "reimg.xml", (IplImage*)reimg, NULL, NULL, cvAttrList(0,0));
*/