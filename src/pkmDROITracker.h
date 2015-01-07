#pragma once

#include "Patches.h"
#include "ImageRepresentation.h"
#include "StrongClassifierDirectSelection.h"
#include "StrongClassifierStandard.h"
#include "Detector.h"
#include "BoostingTracker.h"
#include "ImageHandler.h"
#include <cv.h>
#include <cxcore.h>
#include <vector>

#define TRACKER BoostingTracker

class pkmDROITracker
{
public:
	pkmDROITracker(unsigned char *pixels, int img_w, int img_h, 
		int roi_x, int roi_y, int roi_w, int roi_h, vector<CvPoint> * pts);
	~pkmDROITracker();

	void trackNextFrame(unsigned char *pixels);
	void getCurrentVertices(vector<CvPoint> *pts);

private:
	ImageRepresentation *	curFrameRep;

	TRACKER **				tracker;

	boosting::Rect			trackingRect;
	boosting::Size			trackingRectSize;

	boosting::Rect			wholeImage;


	int numVertices;
	int radius;
	float searchFactor;
	float overlap;
	bool *trackerLost;
};