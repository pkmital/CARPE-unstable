#include "Patches.h"
#include "ImageRepresentation.h"
#include "StrongClassifierDirectSelection.h"
#include "StrongClassifierStandard.h"
#include "Detector.h"
#include "BoostingTracker.h"
#include "ImageHandler.h"
#include "pkmDROITracker.h"

#ifndef MIN
	#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
	#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif


pkmDROITracker::pkmDROITracker(unsigned char *pixels, int img_w, int img_h, 
		int roi_x, int roi_y, int roi_w, int roi_h, vector<CvPoint> * pts)
{
	//delete curFrameRep;
	//delete tracker;

	radius = 15;

	boosting::Size img_size(img_h, img_w);
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
	numVertices = pts->size();
	trackerLost = new bool[numVertices];

	tracker = new TRACKER*[numVertices];
	for( int i = 0; i < numVertices; i++ )	{	
		trackingRect.left = MAX(pts->at(i).x - radius,1);
		trackingRect.upper = MAX(pts->at(i).y - radius,1);
		trackingRect.width = MIN(radius*2+1, img_w - trackingRect.left);
		trackingRect.height = MIN(radius*2+1, img_h - trackingRect.upper);
		tracker[i] = new TRACKER(curFrameRep, trackingRect, wholeImage, numClassifers);
		trackerLost[i] = false;
	}
	
	trackingRectSize = trackingRect;
	
	searchFactor = 2;
	overlap = 0.99f;
}

pkmDROITracker::~pkmDROITracker()
{
	delete curFrameRep;
	for(int i = 0; i < numVertices; i++)
	{
		delete tracker[i];
	}
	delete [] tracker;
	delete [] trackerLost;
}

void pkmDROITracker::trackNextFrame(unsigned char *pixels)
{
	// continue tracking
	for(int i = 0; i < numVertices; i++)
	{
		//calculate the patches within the search region
		Patches *trackingPatches;	
		boosting::Rect searchRegion;

		if (!trackerLost[i])
		{
			searchRegion = tracker[i]->getTrackingROI(searchFactor);
			trackingPatches = new PatchesRegularScan(searchRegion, wholeImage, trackingRectSize, overlap);
		}
		else
		{
			//extend the search region (double size or full window)
			searchRegion = tracker[i]->getTrackingROI(searchFactor*2.0f);
			//searchRegion = wholeImage;
			trackingPatches = new PatchesRegularScan(searchRegion, wholeImage, trackingRectSize, overlap);
		}
		curFrameRep->setNewImageAndROI(pixels, searchRegion);

		if (!tracker[i]->track(curFrameRep, trackingPatches))	{
			trackerLost[i] = true;
		}
		else	{
			trackerLost[i] = false;
		}
		
		delete trackingPatches;
	}
}

void pkmDROITracker::getCurrentVertices(vector<CvPoint> *pts)
{
	for( int i = 0; i < numVertices; i++ )
	{
		boosting::Rect newPatch = tracker[i]->getTrackedPatch();
		int x = newPatch.left;
		int y = newPatch.upper;
		int w = newPatch.width;
		int h = newPatch.height;
		CvPoint pt = cvPoint(x+radius, y+radius);
		pts->push_back(pt);
		printf("%i: %i, %i\t", pt.x, pt.y);
	}
	printf("\n");
}