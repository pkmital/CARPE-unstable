
#include "Patches.h"
#include "ImageRepresentation.h"
#include "StrongClassifierDirectSelection.h"
#include "StrongClassifierStandard.h"
#include "Detector.h"
#include "BoostingTracker.h"
#include "SemiBoostingTracker.h"
#include "ImageHandler.h"
#include "pkmBoostingTracker.h"

pkmBoostingTracker::pkmBoostingTracker(unsigned char *pixels, int img_w, int img_h, 
		int roi_x, int roi_y, int roi_w, int roi_h, bool useAdaptive)
{
	useAdaptTrack = useAdaptive;
	trackingRect.left = roi_x;
	trackingRect.upper = roi_y;
	trackingRect.width = roi_w;
	trackingRect.height = roi_h;

	boosting::Size img_size(img_h, img_w);
	printf("allocating img rep: %d,%d...", img_h, img_w);
	curFrameRep = new ImageRepresentation(pixels, img_size); 
	wholeImage = img_size;

		/*
			% classifier (boosting)
			numSelectors = 100

			% search region (size and resolution)
			overlap = 0.99
			searchFactor = 2
		*/
	


	int numClassifers = 100;
	printf("allocate tracker...");
	if(useAdaptTrack)
		semi_boosting_tracker = new SemiBoostingTracker(curFrameRep, trackingRect, wholeImage, numClassifers);
	else
		boosting_tracker = new BoostingTracker(curFrameRep, trackingRect, wholeImage, numClassifers);
	printf("X ");
		
	trackingRectSize = trackingRect;
	
	searchFactor = 2;
	overlap = 0.95f;
	trackerLost = false;
}
//
//pkmBoostingTracker::pkmBoostingTracker(unsigned char *pixels, int img_w, int img_h, 
//		int roi_x, int roi_y, int roi_w, int roi_h, vector<CvPoint> * pts, bool useAdaptive)
//{
//	delete curFrameRep;
//	if(useAdaptiveTracking)
//		delete semi_boosting_tracker;
//	else
//		delete tracker;
//	trackingRect.left = roi_x;
//	trackingRect.upper = roi_y;
//	trackingRect.width = roi_w;
//	trackingRect.height = roi_h;
//
//	boosting::Size img_size(img_h, img_w);
//	curFrameRep = new ImageRepresentation(pixels, img_size); 
//	wholeImage = img_size;
//
//		/*
//			% classifier (boosting)
//			numSelectors = 100
//
//			% search region (size and resolution)
//			overlap = 0.99
//			searchFactor = 2
//		*/
//	
//	int numClassifers = 100;
//	tracker = new TRACKER(curFrameRep, trackingRect, wholeImage, numClassifers);
//	
//	trackingRectSize = trackingRect;
//	
//	searchFactor = 2;
//	overlap = 0.99f;
//	trackerLost = false;
//}

pkmBoostingTracker::~pkmBoostingTracker()
{
	delete curFrameRep;
	if(useAdaptTrack)
		delete semi_boosting_tracker;
	else
		delete boosting_tracker;
}

void pkmBoostingTracker::trackNextFrame(unsigned char *pixels)
{
	// continue tracking
	
	//calculate the patches within the search region
	Patches *trackingPatches;	
	boosting::Rect searchRegion;

	if (!trackerLost)
	{
		printf("tracker was not lost, finding search regions\n");
		if(useAdaptTrack)
			searchRegion = semi_boosting_tracker->getTrackingROI(searchFactor);
		else
			searchRegion = boosting_tracker->getTrackingROI(searchFactor);
		printf("found %d, %d, %d, %d\n", 
			searchRegion.left, searchRegion.upper, 
			searchRegion.left+searchRegion.width, searchRegion.upper+searchRegion.height);
		trackingPatches = new PatchesRegularScan(searchRegion, wholeImage, trackingRectSize, overlap);
		printf("found new tracking patches\n");
	}
	else
	{
		printf("lost tracker, finding search regions\n");
		//extend the search region (double size or full window)		
		if(useAdaptTrack)
			searchRegion = semi_boosting_tracker->getTrackingROI(searchFactor*2.0f);
		else
			searchRegion = boosting_tracker->getTrackingROI(searchFactor*2.0f);
		printf("found %d, %d, %d, %d\n", 
			searchRegion.left, searchRegion.upper, 
			searchRegion.left+searchRegion.width, searchRegion.upper+searchRegion.height);
		//searchRegion = wholeImage;
		trackingPatches = new PatchesRegularScan(searchRegion, wholeImage, trackingRectSize, overlap);
		printf("found new tracking patches\n");
	}
	curFrameRep->setNewImageAndROI(pixels, searchRegion);

	printf("calling track(...)...");
	if(useAdaptTrack)
	{
		if (!semi_boosting_tracker->track(curFrameRep, trackingPatches))	{
			trackerLost = true;
		}
		else	{
			trackerLost = false;
		}
	}
	else
	{
		if (!boosting_tracker->track(curFrameRep, trackingPatches))	{
			trackerLost = true;
		}
		else	{
			trackerLost = false;
		}
	}
	
	delete trackingPatches;
	printf("done.\n");
}

void pkmBoostingTracker::getCurrentROI(int &x, int &y, int &w, int &h)
{
	boosting::Rect newPatch;
	if(useAdaptTrack)
		newPatch = semi_boosting_tracker->getTrackedPatch();
	else
		newPatch = boosting_tracker->getTrackedPatch();
	x = newPatch.left;
	y = newPatch.upper;
	w = newPatch.width;
	h = newPatch.height;
	printf("%i, %i\n", w, h);
}

void pkmBoostingTracker::useAdaptiveTracking(bool flag)
{
	// Nothing to change
	if(flag && useAdaptTrack)
	{
		return;
	}
	else if(!flag && !useAdaptTrack)
	{
		return;
	}
	
	// Else we need to change trackers...
	// First get the ROI to set the new tracker
	boosting::Rect trackingRect;
	if(useAdaptTrack)
		trackingRect = semi_boosting_tracker->getTrackedPatch();
	else
		trackingRect = boosting_tracker->getTrackedPatch();

	// Delete the old tracker
	if(flag)
	{
		useAdaptTrack = true;
		if(boosting_tracker)
		{
			delete boosting_tracker;
		}
	}
	
	else
	{
		useAdaptTrack = false;
		if(semi_boosting_tracker)
		{
			delete semi_boosting_tracker;
		}
	}

	// Now create the new one
	int numClassifers = 100;
	if(useAdaptTrack)
		semi_boosting_tracker = new SemiBoostingTracker(curFrameRep, trackingRect, wholeImage, numClassifers);
	else
		boosting_tracker = new BoostingTracker(curFrameRep, trackingRect, wholeImage, numClassifers);
}