// Written by Parag K Mital
// Nov. 2008

#define _USEUNSTABLE 0

#ifndef _USEUNSTABLE
	#pragma comment(lib, "src/CUDA/lib/cufft.lib")
	#pragma comment(lib, "src/CUDA/lib/cublas.lib")
	#pragma comment(lib, "src/CUDA/lib/cuda.lib")
	#pragma comment(lib, "src/CUDA/lib/cudart.lib")
	#pragma comment(lib, "src/SIFTGPU/SiftGPU/lib/SIFTGPU.lib")
	#pragma comment(lib, "src/SIFTGPU/SiftGPU/lib/cg.lib")
	#pragma comment(lib, "src/FlowLib/lib/vm.lib")
	#pragma comment(lib, "src/FlowLib/lib/common_static.lib")
	#pragma comment(lib, "src/FlowLib/lib/flow.lib")

	#include "GPUFlow.h"
	#include "pkmDROITracker.h"
	#include "pkmBoostingTracker.h"
	#include "pkmDROI.h"
#endif

#include <iostream>
#include <fstream>
#include <vector>

#include "pkmActiveContourSeg.h"

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxGuiTypes.h"
#include "ofQtVideoSaver.h"
#include <ofxOpenCv.h>

// GUI stuff:


// be sure to change this to at least the total!
const int total_gui_elements = 100;
const string cluster_tags[] = {"1", "2", "3", "4", "8", "1-2", "1-4", "1-8", "2-4", "4-8"};
const string scalar_tags[] = {"1","2","4","6","8"};
const string res_tags[] = {"640x480","800x600","1024x768","1280x960","1440x900","1600x1200"};

class diemDROI : public ofBaseApp, public ofxGuiListener 
{

#ifndef _USEUNSTABLE
	enum { cam_shift, dense_optical_flow, boosting };
#endif

enum
{
	gui_Loader,
	gui_LoaderSlider,

	gui_ObjectsPanel,
	gui_NewFHObjectTrigger,
	gui_NewRectObjectTrigger,

	gui_OptionsPanel,
	gui_EnableItemsPanel,
	gui_EnableEyes,
	gui_EnableSaccades,
	gui_EnableSubjectNames,
	gui_EnableAllFixations,
	gui_EnableMeanBinocular,
	gui_EnableMovie,
	gui_EnableHistogram,
	gui_EnableAlphaScreen,
	gui_EnableClustering,
	gui_EnableHeatmap,
	gui_EnableContours,
	gui_EnableNormalized,
	gui_EnableRealTime,
	gui_CovarianceSwitch,
	gui_EyefilesSwitch,

	gui_ExportPanel,
	gui_EnableRecording,
	gui_EnableSaveMovieImages,
	gui_ImportFromIAS,
	gui_ExportAllObjects,

	gui_TrackPanel,
	gui_TrackObjectBtn,
	gui_TrackAdaptiveBtn,
	gui_BinDROIs,
	gui_InitDROIs,
	gui_TrackCAMShift,
	gui_TrackFlow,
	gui_TrackBoosting,
	
	gui_MovieTimeHolder,
	gui_MovieTimeSlider,

	gui_ScalarsSwitch,
	gui_SigmaSwitch,
	gui_ResSwitch,
	gui_ClustersSwitch,

	gui_VisualizationPanel,
	gui_DrawOpticalFlowBtn,
	gui_DrawFlicker,
	gui_DrawEdges
};

public:
	//////////////////////////////
	diemDROI();
	~diemDROI();
	//////////////////////////////

	//////////////////////////////
	void update();
	//////////////////////////////
		void updateEyesForCurrentFrame();
		void updateEdges();
		void updateFlicker();
#ifndef _USEUNSTABLE
		void updateColorFlow();
		void trackObject();
#endif
	//////////////////////////////
		
	//////////////////////////////
	void draw();
	//////////////////////////////
		void drawEyes();
		void drawMovieControls();
		void drawEdges();
		void drawFlicker();
#ifndef _USEUNSTABLE
		void drawColorFlow();
#endif
		void drawHistogram(IplImage *src);
	//////////////////////////////
		
	//////////////////////////////
	void setup();
	//////////////////////////////
		void initializeOptions();
		void loadEyeTrackingMovie();
		void initializeGui();
		void loadEyeTrackingData();
		void initializeMovieOutput();
	//////////////////////////////
	
	//////////////////////////////
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased();
	//////////////////////////////
	
	// load the ias file into a vector of objects...
	void importFromIAS();

	//////////////////////////////
	void handleGui(int paramterId, int task, void* data, int length);
	ofxGui*					gui;
	ofxGuiPanel*			panel1;
	ofxGuiPanel*			panel2;
	ofxGuiPanel*			panel3;
	ofxGuiPanel*			panel4;
	ofxGuiPanel*			panel5;
	ofxGuiPanel*			panel6;
	//////////////////////////////
	
	//////////////////////////////
	int						frameCounter;		// garbage variable for the text input
	float					movie_time;			// for drawing the movie time
	string					movie_name;			// for loading the movie and eye data
	ofVideoPlayer			mov;				// the movie

	int						movhr, 
							movmin, 
							movsec;				// printing the time
	bool					isPaused;			// are we paused
	bool					updatedFrame;		// did the frame change
	//////////////////////////////

	
	//////////////////////////////
	ofstream				filePtr;			// for writing stats to a file
	//////////////////////////////
	
	//////////////////////////////
	float					movieSpeed;			// current movie speed
	int						frameNumber;		// current frame number
	int						lastFrameNumber;	// previous frame number
	int						FPS, numFrames;		// fps.. total frames
	//////////////////////////////
	
	//////////////////////////////
	ofxDirList				myDIR;				// defaults to the directory ./eyefiles
	//////////////////////////////

	int						mouse_state_down;	// is the mouse pressed down?

#ifndef _USEUNSTABLE
	//////////////////////////////
	bool					newObjectTrigger;	// new droi
	vector<ofxGuiFiles *>	gui_Objects;		// this displays the objects as a selectable list
	vector<pkmDROI *>		objs;				// all the objects
	int						current_obj;		// index of current obj
	bool					loading_obj;		// ensure that you can only load one object at a time
	//////////////////////////////

	//////////////////////////////
	ofxGuiButton *			trackButton;		// callback to the object track button
	bool					trackCurrentObject; // are we tracking?
	int						tracker_type;		// deprecated
	bool					useAdaptTrack;		// type of tracker to use
	bool					binDROIs;			// when true, record objects are set to 0 vertices
	//////////////////////////////

	//////////////////////////////
	GPUFlow *				myflowlib;			// calculates optical flow
	GPUFlow *				frameflowlib;		// drawing the whole frame's motion
	//////////////////////////////

	//////////////////////////////
	vector<pkmBoostingTracker *> trackers;
	pkmActiveContourSeg segmenter;
	//////////////////////////////
#endif

	//////////////////////////////
	ofxCvColorImage			movCvImg;			// used for the RGB histogram
	//////////////////////////////



	// keeps x,y,dil data on each eye file
	class eye
	{
	public:
		// ptr our eye-tracking files
		ifstream			eyeFile;
		// values for coordinates
		vector<float>		eye_x,		eye_y;
		vector<float>		b_eye_x,	b_eye_y;			// the prefix 'b_' for the binocular eye
		vector<float>		m_eye_x,	m_eye_y;			// the prefix 'm_' for the mean of the mono and bino eye
		vector<float>		dilation,	b_dilation, m_dilation;
		vector<int>			left_event,	right_event;
		vector<bool>		is_fixation;
		string				prefix;							// subject prefix from filename
	};	
	eye						*eyeList;
	int						eyeListSize, b_eyeListSize, m_eyeListSize;		// number of eye files for the current frame w/o blinks
	int						maxEyeListSize;									// because of blinks, eyeListSize may be smaller

	int						*xs, *b_xs, *m_xs;
	int						*ys, *b_ys, *m_ys;				// dynamic storage to plot x,y
	int						*prev_m_xs, *prev_m_ys;			// previous mean x,y
	float					*scale_eyes;						// enlarge the eyes as they stay in the same area
	
	double					*inputModelMap;		// for the clustering algorithm
	enum					{COV_SPHERE, COV_DIAG, COV_GEN};
	int						cov_type;
	
	bool					loadedFiles;		// flag for when the eye-tracking files are finished loading
	bool					doneRecording;		// if the movie is over
	
	int						eyeCounter;			// counter for eyeListSize

	int						offset_x, offset_y; // movie offset based on screen res
	int						map_scalar;
	
	ofTexture				heatmap_tex;		// displaying a heatmap
	ofTexture				readBackTex;
	ofTexture				movieAndAlpha;		// alpha heatmap
	ofTexture				alphaScreen;

	ofxCvGrayscaleImage 	heatmap;			// drawing the opengl version of the heatmap
	ofxCvColorImage			heatmap3;			// drawing the opengl version of the colorized heatmap

	unsigned char*			eyePtsMap;			// original eye locations
	unsigned char*			colorized;			// colorized heatmap image
	unsigned char*			rgb;				// rgb triple from the colormap computation
	unsigned int*			unnormalized;		// unnormalized heatmap image
	unsigned char*			movieOut;			// image for recording the movie output
	
	ofQtVideoSaver			saver;				// for recording the output movie
	ofImage					saverImg;			// for saving the movie as images
	
	ofxCvGrayscaleImage		red, green, blue;
	
	ofxCvContourFinder		me;					// contour finder
	
	GLuint*					pboIds;				// IDs of PBOs
	
	float gaussDist(int &x,int mean);
	float gaussDist(float x, float y, float meanx, float meany, float sigmax, float sigmay);
	float gaussDist(float x, float meanx, float sigmax);
	float sigma;
	float A;
	float B;
	
	// variables for drawing
	bool					showEyes, showMovie, showHeatmap, showRecording, showAlphaScreen,
							showContours, showNormalized, showRealTime, showClustering, showHistogram, 
							showMeanBinocular, showSaccades, showAllFixations, showSubjectNames;
#ifndef _USEUNSTABLE
	bool					drawOpticalFlow;	// are we drawing the optical flow?
	ofxCvColorImage			motion_img;
	bool					enableMotion;
#endif

	bool					showFlicker;
	ofxCvGrayscaleImage		flicker_img;
	ofxCvGrayscaleImage		flicker_this_img;
	ofxCvGrayscaleImage		flicker_prev_img;

	bool					showEdges;
	ofxCvGrayscaleImage		edge_img;

	bool					loadBinocular, 
							saveMovieImages;


	//const cluster_tags[] = {"1", "2", "4", "6", "8", "1-2", "1-4", "1-8", "2-4", "4-8"};
	//panel1->addSwitch(gui_ClustersSwitch, "Clustering Kernels", 
	int minClusterComponents, maxClusterComponents;


	// detects if the gui changed, and to update the display
	bool					reset;
};

