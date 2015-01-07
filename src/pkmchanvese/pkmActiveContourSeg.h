#pragma once

#include "sparse3c.h"
#include <cv.h>
#include <cxcore.h>
#include <cvaux.h>
#include <highgui.h>

class pkmActiveContourSeg {

public:
	pkmActiveContourSeg()
	{
		lambda = 0.3;
		iter = 20;
	}
	double * segment(unsigned char *img, int width, int height, unsigned char *mask)
	{
		long dims[5];
		dims[2] = 1;
		dims[1] = width;
		dims[0] = height;
		dims[3] = dims[0]*dims[1]; 
		dims[4] = dims[0]*dims[1]*dims[2];

		cvNamedWindow("img", CV_WINDOW_AUTOSIZE);
		IplImage *img8 = cvCreateImageHeader(cvSize(width,height), IPL_DEPTH_8U, 3);
		img8 = cvInitImageHeader(img8, cvSize(width,height), IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4);
		cvSetData(img8, img, img8->widthStep);
		IplImage *img64 = cvCreateImage(cvSize(width, height), IPL_DEPTH_64F, 3);
		cvConvertScale(img8, img64, 1/255.);
			cvShowImage("img", img64); cvWaitKey(0);

		cvNamedWindow("mask", CV_WINDOW_AUTOSIZE);
		IplImage *mask8 = cvCreateImageHeader(cvSize(width,height), IPL_DEPTH_8U, 1);
		mask8 = cvInitImageHeader(mask8, cvSize(width,height), IPL_DEPTH_8U, 1, IPL_ORIGIN_TL, 4);
		cvSetData(mask8, mask, mask8->widthStep);
		IplImage *mask64 = cvCreateImage(cvSize(width, height), IPL_DEPTH_64F, 1);
		cvConvertScale(mask8, mask64, 1/255.);
			cvShowImage("mask", mask64); cvWaitKey(0);

		IplImage *phi64 = cvCreateImage(cvSize(width,height), IPL_DEPTH_64F, 1);
		IplImage *label64 = cvCreateImage(cvSize(width,height), IPL_DEPTH_64F, 1);
		double *img_d, *mask_d, *phi_d, *label_d;
		img_d = (double *)img64->imageData;
		mask_d = (double *)mask64->imageData;
		phi_d = (double *)phi64->imageData;
		label_d = (double *)label64->imageData;

		//create linked lists
		Lz  = ll_create();
		Ln1 = ll_create();
		Ln2 = ll_create();
		Lp1 = ll_create();
		Lp2 = ll_create();
		Lin2out = ll_create();
		Lout2in = ll_create();

		//initialize lists, phi, and labels
		ls_mask2phi3c(mask_d,phi_d,label_d,dims,Lz,Ln1,Ln2,Lp1,Lp2);

		//compute iterations of chanvese energy
		chanvese(img_d,phi_d,label_d,dims,
			Lz,Ln1,Lp1,Ln2,Lp2,Lin2out,Lout2in,
			iter,lambda);

		//destroy linked lists
		ll_destroy(Lz);
		ll_destroy(Ln1);
		ll_destroy(Ln2);
		ll_destroy(Lp1);
		ll_destroy(Lp2);
		ll_destroy(Lin2out);
		ll_destroy(Lout2in);

		// copy phi and release it...
		
		cvNamedWindow("img", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("mask", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("phi", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("label", CV_WINDOW_AUTOSIZE);

		cvShowImage( "img", img64 );  
		cvShowImage( "mask", mask64 );  
		cvShowImage( "phi", phi64 );  
		cvShowImage( "label", label64 );  
		
		cvWaitKey(0);
		
		cvReleaseImageHeader(&img8);
		cvReleaseImageHeader(&mask8);
		cvReleaseImage(&img64);
		cvReleaseImage(&mask64);
		cvReleaseImage(&phi64);
		cvReleaseImage(&label64);

		return phi_d;

	}

	void chanvese(double *img, double *phi, double *label, long *dims,
		LL *Lz, LL *Ln1, LL *Lp1, LL *Ln2, LL *Lp2, LL *Lin2out, LL *Lout2in,
		int iter,double lambda){
			double *F;
			double scale[1]; scale[0] = 0;
			int countdown;

			//initialize datastructures and statistics
			en_chanvese_init(img,phi,dims);
			for(int i=0;i<iter;i++){
				//compute force
				F = en_chanvese_compute(Lz,phi,img,dims,scale,lambda);
				//perform iteration
				ls_iteration(F,phi,label,dims,Lz,Ln1,Lp1,Ln2,Lp2,Lin2out,Lout2in);
				//update statistics
				en_chanvese_update(img, dims, Lin2out, Lout2in);
			}
	}

private:
	double lambda;
	int    iter;

	LL *Lz, *Ln1, *Ln2, *Lp1, *Lp2;
	LL *Sz, *Sn1, *Sn2, *Sp1, *Sp2;
	LL *Lin2out, *Lout2in;
};