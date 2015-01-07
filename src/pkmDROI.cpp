#include "pkmDROI.h"

#ifndef round
#define round(x) ((int)((float)x+0.5f))
#endif

void pkmDROI::setup(int initx, int inity, int initw, int inith, int numframes, int obj, string name) 
{
	printf("pkmDROI::setup() - hello!\n");
	
	obj_Type = obj;

	// colors for drawing the shape
	r_idle = 100; g_idle = 0; b_idle = 0; a_idle = 100;
	r_over = 255; g_over = 0; b_over = 0; a_idle = 120;
	r_down = 255; g_over = 0; b_over = 0; a_over = 100;

	r_idle_play = 0; g_idle_play = 255; b_idle_play = 0; a_idle_play = 100;

	// this is for updating how much the shape has moved inbetween
	// successive mouse events
	delta_x = 0; delta_y = 0;
	initial_x = 0; initial_y = 0;
	initial_x_world = 0; initial_y_world = 0;

	// bounds are like the bounding box of the shape
	bounds_x = initx;
	bounds_y = inity;
	bounds_x2 = initx+initw;
	bounds_y2 = inity+inith;
	setPosAndSize(bounds_x, bounds_y, bounds_x2, bounds_y2);

	// freehand drawing of shape
	initializing = true;
	mouse_x = 0;
	mouse_y = 0;

	// this is the current dragged vertex
	dragging_vertex_idx = -1;

	num_frames = numframes;
	current_frame = 0;

	vertices = new vector<CvPoint>[numframes];
	updatedByMouse = false;

	my_name = name;

	state = 0;

	disableAllEvents();
}

void pkmDROI::reinitialize(int initx, int inity, int initw, int inith)
{
	if(state == 2)
	{
		if(vertices[current_frame].size())
			vertices[current_frame].clear();
		initializing = true;
		dragging_vertex_idx = -1;
		mouse_x = 0;
		mouse_y = 0;
		delta_x = 0; delta_y = 0;
		initial_x = 0; initial_y = 0;
		initial_x_world = 0; initial_y_world = 0;
		// bounds are like the bounding box of the shape
		bounds_x = initx;
		bounds_y = inity;
		bounds_x2 = initx+initw;
		bounds_y2 = inity+inith;
		setPosAndSize(bounds_x, bounds_y, bounds_x2, bounds_y2);
	}

}

void pkmDROI::exit() 
{
	//printf("pkmDROI::exit() - goodbye!\n");
}


void pkmDROI::update() 
{
	//		x = ofGetWidth()/2 + cos(ofGetElapsedTimef() * 0.2) * ofGetWidth()/4;
	//		y = ofGetHeight()/2 + sin(ofGetElapsedTimef() * 0.2) * ofGetHeight()/4;
}


void pkmDROI::draw() 
{
	// if the user is still drawing the shape
	if( initializing )
	{
		if(obj_Type == obj_Freehand)
		{
			ofLine(mouse_x, bounds_y, mouse_x, bounds_y2);
			ofLine(bounds_x, mouse_y, bounds_x2, mouse_y);
			int i = 0;
			if(vertices[current_frame].size() > 0)
				ofEllipse((vertices[current_frame])[i].x, (vertices[current_frame])[i].y, radius_size, radius_size);
			for( i = 1; i < vertices[current_frame].size(); i++ )
			{
				ofEllipse((vertices[current_frame])[i].x, (vertices[current_frame])[i].y, radius_size, radius_size);
				ofLine((vertices[current_frame])[i].x, (vertices[current_frame])[i].y, 
					(vertices[current_frame])[i-1].x, (vertices[current_frame])[i-1].y);
			}
		}
		else
		{
			ofLine(mouse_x, bounds_y, mouse_x, bounds_y2);
			ofLine(bounds_x, mouse_y, bounds_x2, mouse_y);

			if(vertices[current_frame].size() > 0)
			{
				ofEllipse((vertices[current_frame])[0].x, (vertices[current_frame])[0].y, radius_size, radius_size);
				ofLine((vertices[current_frame])[0].x, (vertices[current_frame])[0].y, (vertices[current_frame])[0].x, mouse_y);
				ofLine((vertices[current_frame])[0].x, mouse_y, mouse_x, mouse_y);
				ofLine(mouse_x, mouse_y, mouse_x, (vertices[current_frame])[0].y);
				ofLine(mouse_x, (vertices[current_frame])[0].y, (vertices[current_frame])[0].x, (vertices[current_frame])[0].y);

			}
		}
	}
	// else show the full shape
	else 
	{
		if(state > 0)
		{
			ofEnableAlphaBlending();
			ofSetPolyMode(OF_POLY_WINDING_NONZERO);
			ofBeginShape();
			vector<CvPoint>::iterator it = (vertices[current_frame]).begin();
			while( it != (vertices[current_frame]).end() )
			{					
				if(state == 2)
				{
					// draw the corners
					ofSetColor(r_over, g_over, b_over, a_over);
					ofEllipse(it->x, it->y, radius_size, radius_size);
					// draw the shape boundaries and fill with the proper color
					if(isMouseDown()) 
						ofSetColor(r_down, g_down, b_down, a_down);
					else if(isMouseOver()) 
						ofSetColor(r_over, g_over, b_over, a_over);
					else 
						ofSetColor(r_idle, g_idle, b_idle, a_idle);
					ofVertex(it->x,it->y);
				}
				else
				{
					// draw the corners
					ofSetColor(r_over, g_over, b_over, a_over);
					ofEllipse(it->x, it->y, radius_size, radius_size);
					ofSetColor(r_idle_play, g_idle_play, b_idle_play, a_idle_play);
					ofVertex(it->x,it->y);
				}
				++it;
			}
			ofEndShape();

			if(dragging_vertex_idx != -1) 
			{
				ofSetColor(r_down, g_down, b_down, a_down);
				ofEllipse((vertices[current_frame])[dragging_vertex_idx].x, 
					(vertices[current_frame])[dragging_vertex_idx].y, 
					radius_size, radius_size);
			}
			ofDisableAlphaBlending();
		}
	}
}

void pkmDROI::onRollOver(int _x, int _y) 
{
	printf("pkmDROI::onRollOver(x: %i, y: %i)\n", _x, _y);
}

 void pkmDROI::onRollOut() 
{
	//printf("pkmDROI::onRollOut()\n");
}

 void pkmDROI::onMouseMove(int _x, int _y)
{
	printf("pkmDROI::onMouseMove(x: %i, y: %i)\n", _x, _y);
	mouse_x = (float)_x;
	mouse_y = (float)_y;
}

 void pkmDROI::onDragOver(int _x, int _y, int button) 
{
	//printf("pkmDROI::onDragOver(x: %i, y: %i, button: %i)\n", 
	//	_x, _y, button);
			
	if(initializing)
	{

	}
	else
	{
		delta_x = initial_x - (float)_x; 
		delta_y = initial_y - (float)_y;
		initial_x = (float)_x;
		initial_y = (float)_y;

		// if they are dragging a vertex
		if(dragging_vertex_idx != -1)
		{
			(vertices[current_frame])[dragging_vertex_idx].x = _x;
			(vertices[current_frame])[dragging_vertex_idx].y = _y;
			updatedByMouse = true;
		}
		// they were in fact trying to drag the object
		else
		{
			// update the objects vertices
			vector<CvPoint>::iterator it = vertices[current_frame].begin();
			while( it != vertices[current_frame].end() )
			{
				it->x -= delta_x;
				it->y -= delta_y;
				it++;
				updatedByMouse = true;
			}
		}
		
		updateBounds();
	}
}

 void pkmDROI::onDragOutside(int _x, int _y, int button) 
{
	//printf("pkmDROI::onDragOutside(x: %i, y: %i, button: %i)\n", x, y, button);
			
	if(initializing)
	{

	}
	else
	{
		delta_x = initial_x - (float)_x; 
		delta_y = initial_y - (float)_y;
		initial_x = (float)_x;
		initial_y = (float)_y;

		// if they are dragging a vertex
		if(dragging_vertex_idx != -1)
		{
			(vertices[current_frame])[dragging_vertex_idx].x = _x;
			(vertices[current_frame])[dragging_vertex_idx].y = _y;
			updatedByMouse = true;
		}
		// they were in fact trying to drag the object
		else
		{
			// update the objects vertices
			vector<CvPoint>::iterator it = vertices[current_frame].begin();
			while( it != vertices[current_frame].end() )
			{
				it->x -= delta_x;
				it->y -= delta_y;
				it++;
				updatedByMouse = true;
			}
		}

		updateBounds();
	}
}

 void pkmDROI::onPress(int _x, int _y, int button) 
{
	//printf("pkmDROI::onPress(x: %i, y: %i, button: %i)\n", _x, _y, button);
	// x,y on the object coords
	initial_x = (float)_x;
	initial_y = (float)_y;
	// x,y of the world coords
	initial_x_world = x;
	initial_y_world = y;


	if(initializing)
	{
		if(obj_Type == obj_Freehand)
		{
			// check to see if the user has selected the last point
			if( vertices[current_frame].size() > 2 && 
				WITHIN(_x,(vertices[current_frame])[0].x,radius_size) && 
				WITHIN(_y,(vertices[current_frame])[0].y,radius_size) )
			{
				initializing = false;
			}
			else
			{
				CvPoint v = cvPoint(_x, _y);
				vertices[current_frame].push_back(v);
			}
		}
		else
		{
			if(vertices[current_frame].size() > 0)
			{
				CvPoint v = cvPoint((vertices[current_frame])[0].x, _y);
				vertices[current_frame].push_back(v);
				v = cvPoint(_x, _y);
				vertices[current_frame].push_back(v);
				v = cvPoint(_x, (vertices[current_frame])[0].y);
				vertices[current_frame].push_back(v);
				initializing = false;
			}
			else
			{
				CvPoint v = cvPoint(_x, _y);
				vertices[current_frame].push_back(v);
			}
		}
		updatedByMouse = false;
	}
	else
	{
		// we have to loop through all the vertices to see if they are dragging a vertex
		int idx = 0;
		vector<CvPoint>::iterator it = vertices[current_frame].begin();
		while( it != vertices[current_frame].end() )
		{
			// if the user is trying to move a vertex
			if( WITHIN(_x,it->x,radius_size) && WITHIN(_y,it->y,radius_size) )
			{
				dragging_vertex_idx = idx;
			}
			it++; idx++;
		}
	}
}

 void pkmDROI::onRelease(int x, int y, int button) 
{
	//printf("pkmDROI::onRelease(x: %i, y: %i, button: %i)\n", x, y, button);


	if(initializing)
	{

	}
	else
	{
		// if the object was being moved by a vertex
		if(dragging_vertex_idx != -1)
		{
			// no longer moving the vertex
			dragging_vertex_idx = -1;
		}
		updateBounds();
	}
}

 void pkmDROI::onReleaseOutside(int x, int y, int button) {
	//printf("pkmDROI::onReleaseOutside(x: %i, y: %i, button: %i)\n", x, y, button);
	if(initializing)
	{

	}
	else
	{			
		// if the object was being moved by a vertex
		if(dragging_vertex_idx != -1)
		{
			// no longer moving the vertex
			dragging_vertex_idx = -1;
		}
		updateBounds();
	}
	
}

 void pkmDROI::keyPressed(int key) {
	//printf("pkmDROI::keyPressed(key: %i)\n", key);
	switch(key)
	{
	case OF_KEY_BACKSPACE:
		if(initializing && vertices[current_frame].size())
			vertices[current_frame].pop_back();
		break;
	}
}

 void pkmDROI::keyReleased(int key) {
	//printf("pkmDROI::keyReleased(key: %i)\n", key);
}

// update the mouse bounds (not the vertex bounds!)
void pkmDROI::setBounds(int _x, int _y, int width, int height) {
	bounds_x = _x;
	bounds_y = _y;
	bounds_x2 = _x + width;
	bounds_y2 = _y + height;
}

// updates the interactive bounds for mouse hittests
// (not the actual object drawn bounds)
void pkmDROI::updateBounds()
{
	// update the bounding box
	vector<CvPoint>::iterator it = vertices[current_frame].begin();
	bounds_x = it->x; bounds_x2 = it->x;
	bounds_y = it->y; bounds_y2 = it->y;
	while( ++it != vertices[current_frame].end() )
	{
		bounds_x = MIN(bounds_x, it->x);
		bounds_y = MIN(bounds_y, it->y);
		bounds_x2 = MAX(bounds_x2, it->x);
		bounds_y2 = MAX(bounds_y2, it->y);
	}

	// these are for the hitTest on the interactiveObject...
	// these bounds control the mouse listeners
	// that is why i add a radius size to them, so we can 
	// still move the vertices a bit outside the bounding box
	x = bounds_x - 2*radius_size;
	y = bounds_y - 2*radius_size;
	width = bounds_x2 - bounds_x + 2*radius_size;
	height = bounds_y2 - bounds_y + 2*radius_size;
}

// return in the passed image a convex polygon.  the size of the image
// should be greater than or equal to the maximum of any vertex element.
// the image will be a binary mask with 0's set to background and 1's
// set to the foreground
//
// img should be allocated already
void pkmDROI::getConvexPoly(CvArr* img)
{
	if(initializing)
	{

	}
	else
	{
		int npts = vertices[current_frame].size()+1;
		CvPoint *pts = new CvPoint[npts];
		int idx = 0;
		vector<CvPoint>::iterator it = vertices[current_frame].begin();
		while(it != vertices[current_frame].end())
		{
			CvPoint p = cvPoint(it->x, it->y);
			pts[idx] = p;
			it++; idx++;
		}
		it = vertices[current_frame].begin();
		CvPoint p = cvPoint(it->x, it->y);
		pts[idx] = p;

		cvSet(img, cvScalarAll(0), 0);
		cvFillConvexPoly( img, pts, npts, cvScalarAll(255) );
		/*
		cvFillConvexPoly( CvArr* img, CvPoint* pts, int npts, CvScalar color, int line_type=8, int shift=0 );
		img
			Image. 
		pts
			Array of pointers to a single polygon. 
		npts
			Polygon vertex counter. 
		color
			Polygon color. 
		line_type
			Type of the polygon boundaries, see cvLine description. 
		shift
			Number of fractional bits in the vertex coordinates. 

		The function cvFillConvexPoly fills convex polygon interior. 
		This function is much faster than The function cvFillPoly and 
		can fill not only the convex polygons but any monotonic polygon, 
		i.e. a polygon whose contour intersects every horizontal line 
		(scan line) twice at the most.
		*/

		delete [] pts;
	}
}

void pkmDROI::getBounds(int &x, int &y, int &x2, int &y2)
{
	vector<CvPoint>::iterator it = vertices[current_frame].begin();
	x = it->x;
	y = it->y;
	x2 = x;
	y2 = y;
	while(it != vertices[current_frame].end())
	{
		x = MIN(it->x, x);
		y = MIN(it->y, y);
		x2 = MAX(it->x, x2);
		y2 = MAX(it->y, y2);
		it++;
	}
}

vector<CvPoint> * pkmDROI::getVertices()
{
	return &vertices[current_frame];
}

void pkmDROI::setVertices(vector<CvPoint> newVerts)
{
	vertices[current_frame].clear();
	vertices[current_frame].resize(newVerts.size());
	std::copy(newVerts.begin(), newVerts.end(), 
		vertices[current_frame].begin());
}

void pkmDROI::updateTo(int frame)
{
	if(state == 2)
	{
		setFrame(frame);
	}
	else
	{
		setFrameWithoutRecord(frame);
	}
}

void pkmDROI::setFrame(int frame)
{
	// if the frame is a valid frame
	if(frame >= 0 && frame < num_frames)
	{
		// if the frame hasn't been allocated then let's set it up
		if(vertices[frame].size() == 0)
		{
			// find the most recent frame with vertices
			int frameToCopy = frame;
			while(vertices[frameToCopy].size() == 0 && frameToCopy > 0)
			{
				frameToCopy--;
			}

			// if there were no recent frames behind, try ahead
			if(frameToCopy == 0 && vertices[frameToCopy].size() == 0)
			{
				frameToCopy = frame;
				while(vertices[frameToCopy].size() == 0 && frameToCopy < num_frames)
				{
					frameToCopy++;
				}
			}

			if(vertices[frameToCopy].size() && frameToCopy != num_frames)
			{
				// copy it
				vertices[frame].resize(vertices[frameToCopy].size());
				std::copy(vertices[frameToCopy].begin(), vertices[frameToCopy].end(), 
					vertices[frame].begin());				
			}
			else
			{
				printf("ERROR: No frames to copy the object from\n");
			}
		}
		// else nothing to do
		
		current_frame = frame;
		//updatedByMouse = false;
	}
}

void pkmDROI::setFrameWithoutRecord(int frame)
{
	// if the frame is a valid frame
	if(frame >= 0 && frame < num_frames)
	{
		current_frame = frame;
		//updatedByMouse = false;
	}
}

// this method will copy vertices into a new frame 
// and then increment the frame number (record method)
// if it does not find frames from the previous frame, it will 
// continue to search back to the first frame until it finds vertices
void pkmDROI::setNextFrame()
{
	// if the next frame is a valid frame
	if(current_frame+1 < num_frames)
	{
		// clear it if we have already set it before
		if(vertices[current_frame+1].size())
			vertices[current_frame+1].clear();

		// find a frame with vertices
		int frame = current_frame;
		while(vertices[frame].size() == 0 && frame > 0)
		{
			frame--;
		}
		
		if(vertices[frame].size())
		{
			// copy the previous frame into the next frame
			vertices[current_frame+1].resize(vertices[frame].size());
			std::copy(vertices[frame].begin(), vertices[frame].end(), 
				vertices[current_frame+1].begin());
		}
		// go to the next frame
		current_frame = current_frame + 1;
		updatedByMouse = false;
	}
}

// this is like setNextFrame except no recording is done.
void pkmDROI::setNextFrameWithoutRecord()
{
	// if the next frame is a valid frame
	if(current_frame+1 < num_frames)
	{
		// go to the next frame
		current_frame = current_frame + 1;
		updatedByMouse = false;
	}
}

// remove the vertices in this frame (if set to record)
void pkmDROI::clearCurrentFrame()
{
	if(state == 2)
	{
		if(vertices[current_frame].size())
			vertices[current_frame].clear();
	}
}

// translation by _x and _y
void pkmDROI::moveMe(int _x, int _y)
{
	// update the objects vertices
	vector<CvPoint>::iterator it = vertices[current_frame].begin();
	while( it != vertices[current_frame].end() )
	{
		it->x += _x;
		it->y += _y;
		it++;
	}
	updateBounds();
	updatedByMouse = false;
}

void pkmDROI::moveMeTo(int _x, int _y, int _w, int _h)
{
	int delta_x = _x - bounds_x;
	int delta_y = _y - bounds_y;

	//float scale_w = (_x + _w) / (bounds_x2+delta_x);
	//float scale_h = (_y + _h) / (bounds_y2+delta_y);

	// update the objects vertices
	vector<CvPoint>::iterator it = vertices[current_frame].begin();
	while( it != vertices[current_frame].end() )
	{
		it->x = (it->x + delta_x);// * ((it->x - bounds_x)/(bounds_x2 - bounds_x)) * scale_w;
		it->y = (it->y + delta_y);// * ((it->y - bounds_y)/(bounds_y2 - bounds_y)) * scale_h;
		it++;
	}
	updateBounds();
	updatedByMouse = false;

}

// this method will reset the DROI to have 4 vertices:
// a rectangle of the given parameters (_x,_y) = (left,top)
// (0,0) is top-left
void pkmDROI::setVerticesToRectangle(int _x, int _y, int _w, int _h)
{
	if(vertices[current_frame].size())
		vertices[current_frame].clear();
	CvPoint pt;
	pt.x = _x;
	pt.y = _y;
	vertices[current_frame].push_back(pt);
	pt.x = _x+_w;
	vertices[current_frame].push_back(pt);
	pt.y = _y+_h;
	vertices[current_frame].push_back(pt);
	pt.x = _x;
	vertices[current_frame].push_back(pt);

	// make sure the mouse does not initialize the points 
	initializing = false;

	// update the interactive bounds
	updateBounds();		
}

bool pkmDROI::wasUpdatedByMouse()
{
	return updatedByMouse;
}

void pkmDROI::clearUpdatedByMouseFlag()
{
	updatedByMouse = false;
}

void pkmDROI::setRecordState(int flag)
{
	if(flag==2)
	{
		enableMouseEvents();
		enableKeyEvents();
	}
	else
	{
		disableMouseEvents();
		disableKeyEvents();
	}
	state = (int)flag;
}

void pkmDROI::setCurrentFrameToKeyFrame()
{
	// if the current frame is not already a keyframe
	if(!isKeyFrame())
	{
		// check if another frame is the same 
		bool alreadyExists = false;
		vector<int>::iterator it = keyframes.begin();
		while(it != keyframes.end())
		{
			alreadyExists = current_frame == (*it);
		}
		if(!alreadyExists)
			keyframes.push_back(current_frame);
	}
	is_current_frame_keyframe = true;
}

void pkmDROI::setFrameToKeyFrame(int frame)
{
	if(frame == current_frame)
		setCurrentFrameToKeyFrame();
	else
	{
		// check if another frame is the same 
		bool alreadyExists = false;
		vector<int>::iterator it = keyframes.begin();
		while(it != keyframes.end())
		{
			alreadyExists = current_frame == frame;
		}
		if(!alreadyExists)
			keyframes.push_back(frame);
	}	
}

bool pkmDROI::isKeyFrame()
{
	return is_current_frame_keyframe;
}

bool pkmDROI::outputToIAS(string filename, string movie_name, int offset_x, int offset_y, int object_num, int fps)
{
	ofstream fileStr(filename.c_str());
	if(!fileStr.is_open())
		return false;

	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	fileStr << "# Eyelink Interest Area Set created by CARPE on \n#\t" 
		<< asctime(timeinfo) 
		<< "# For the clip:\n#\t" 
		<< movie_name.c_str() << "\n\n";

	for( int frame_num = 0; frame_num < num_frames; frame_num++ )
	{
		if ( vertices[frame_num].size() > 2 )
		{
			fileStr << (int)round(-(frame_num-1) / (float)fps * 1000.f) << " " 
				<< (int)round(-(frame_num) / (float)fps * 1000.f)+1
				<< " RECTANGLE " << object_num << " ";
			int left = vertices[frame_num].begin()->x, 
				right = vertices[frame_num].begin()->x, 
				top = vertices[frame_num].begin()->y, 
				bottom = vertices[frame_num].begin()->y;
			for( vector<CvPoint>::iterator it = vertices[frame_num].begin();
				it != vertices[frame_num].end(); ++it )
			{
				left = min(left, it->x);
				right = max(right, it->x);
				top = min(top, it->y);
				bottom = max(bottom, it->y);
			}
			fileStr << offset_x + left << " " << offset_y + top 
				<< " " << offset_x + right << " " << offset_y + bottom << " ";
			fileStr << my_name.c_str() << "\n";
		}
		
	}
	fileStr.close();
	return true;
}