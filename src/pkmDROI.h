// Parag K. Mital
// Nov. 2009
// Copyright DIEM, University of Edinburgh, Henderson's Visual Cognition Lab, 2009-2010

#pragma once

#include "ofxMSAInteractiveObject.h"
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <direct.h>

#define		IDLE_COLOR		0xFFFFFF
#define		OVER_COLOR		0x00FF00
#define		DOWN_COLOR		0xFF0000

#define radius_size 10
#define WITHIN(pt, pt2, radius) (((pt) > ((pt2) - (radius))) && ((pt) < ((pt2) + (radius))))

class pkmDROI : public ofxMSAInteractiveObject {

public:
	void setup(int initx, int inity, int initw, int inith, int numframes, int obj, string name);
	void reinitialize(int initx, int inity, int initw, int inith);

	void exit();
	void update();
	void draw();

	virtual void onRollOver(int _x, int _y);
	virtual void onRollOut();
	virtual void onMouseMove(int _x, int _y);
	virtual void onDragOver(int _x, int _y, int button);
	virtual void onDragOutside(int _x, int _y, int button);
	virtual void onPress(int _x, int _y, int button);
	virtual void onRelease(int x, int y, int button);	
	virtual void onReleaseOutside(int x, int y, int button);

	virtual void keyPressed(int key);
	virtual void keyReleased(int key);

	// update the mouse bounds (not the vertex bounds!)
	void setBounds(int _x, int _y, int width, int height);

	// updates the interactive bounds for mouse hittests
	// (not the actual object drawn bounds)
	void updateBounds();

	// return in the passed image a convex polygon.  the size of the image
	// should be greater than or equal to the maximum of any vertex element.
	// the image will be a binary mask with 0's set to background and 1's
	// set to the foreground
	//
	// img should be allocated already
	void getConvexPoly(CvArr* img);
	void getBounds(int &x, int &y, int &x2, int &y2);

	vector<CvPoint> * getVertices();
	void setVertices(vector<CvPoint> newVerts);

	void updateTo(int frame);	

	// this method will copy vertices into a new frame 
	// and then increment the frame number (record method)
	// if it does not find frames from the previous frame, it will 
	// continue to search back to the first frame until it finds vertices
	void setNextFrame();
	
	void setNextFrameWithoutRecord();	// this is like setNextFrame except no recording is done.
	
	// like the above but for a specified frame
	void setFrame(int frame);
	void setFrameWithoutRecord(int frame);
	
	void clearCurrentFrame();			// remove the vertices in this frame (if set to record)

	// translation by _x and _y
	void moveMe(int _x, int _y);
	void moveMeTo(int _x, int _y, int _w, int _h);

	void setRecordState(int flag);
	
	// this method will reset the DROI to have 4 vertices:
	// a rectangle of the given parameters (_x,_y) = (left,top)
	// (0,0) is top-left
	void setVerticesToRectangle(int _x, int _y, int _w, int _h);

	bool wasUpdatedByMouse();
	void clearUpdatedByMouseFlag();
	
	// for editing the droi with an outer bounding box: 
	// provides affine movement and tweening between keyframes (not yet though)
	void setCurrentFrameToKeyFrame();
	void setFrameToKeyFrame(int frame);
	bool isKeyFrame();
	vector<int> getKeyFrames()	{ return keyframes; }

	bool outputToIAS(string filename, string movie_name, int offset_x, int offset_y, int object_num, int fps = 30);

	unsigned char r_idle, g_idle, b_idle, a_idle,
				r_down, g_down, b_down, a_down,
				r_over, g_over, b_over, a_over,
				r_idle_play, g_idle_play, b_idle_play, a_idle_play;

	float		delta_x, delta_y;				// how much the object has moved
	float		initial_x, initial_y;
	float		initial_x_world, initial_y_world;

	float		bounds_x, bounds_y,				// the bounding box of the object
				bounds_x2, bounds_y2;

	bool		initializing;					// if the shape has not been drawn
	float		mouse_x, mouse_y;				// the current mouse x,y (world)

	int			dragging_vertex_idx;			// idx of the dragged vertex (set to -1 if none)

	int			current_frame;
	int			num_frames;
	vector<int>	keyframes;
	bool		is_current_frame_keyframe;

	int			obj_Type;
	enum		{obj_Rectangle, obj_Freehand};

	bool		updatedByMouse;

	string		my_name;

	int			state;			// record or play state
};

/*

getConvexPoly:
If I can reimplement the vector of ofPoint's to use the CvPoint, then I can save a O(n) op
in setting a new structure of CvPoints...  but then I will have to reimplement how I declare
which vertex is being dragged.  cvpoint has no boolean flag, just an x,y.  Thus, I could keep
the index of the vertex as an integer instead of a boolean, and check for the subscript 
instead of using an iterator.


*/