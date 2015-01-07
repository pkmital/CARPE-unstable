#pragma once

#include "Patches.h"
#include "ImageRepresentation.h"
#include "StrongClassifierDirectSelection.h"
#include "StrongClassifierStandard.h"
#include "Detector.h"
#include "BoostingTracker.h"
#include "SemiBoostingTracker.h"
#include "ImageHandler.h"
#include <cv.h>
#include <cxcore.h>
#include <vector>

#define TRACKER BoostingTracker

class pkmBoostingTracker
{
public:
	pkmBoostingTracker(unsigned char *pixels, int img_w, int img_h, 
		int roi_x, int roi_y, int roi_w, int roi_h, bool useAdaptive = false);
	//pkmBoostingTracker(unsigned char *pixels, int img_w, int img_h, 
	//	int roi_x, int roi_y, int roi_w, int roi_h, vector<CvPoint> * pts);
	~pkmBoostingTracker();

	void trackNextFrame(unsigned char *pixels);
	void getCurrentROI(int &roi_x, int &roi_y, int &roi_w, int &roi_h);

	void useAdaptiveTracking(bool flag);

private:
	ImageRepresentation *	curFrameRep;

	BoostingTracker *		boosting_tracker;
	SemiBoostingTracker *	semi_boosting_tracker;

	boosting::Rect			trackingRect;
	boosting::Size			trackingRectSize;

	boosting::Rect			wholeImage;

	boosting::Rect			newPatch;

	float searchFactor;
	float overlap;
	bool trackerLost;
	bool useAdaptTrack;
};