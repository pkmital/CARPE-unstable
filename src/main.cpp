#define NOMINMAX 1
#include "ofMain.h"
#include "CARPE.h"

//#if _NDEBUG
//	#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 
//#endif
//========================================================================
int main( ){

	// setup the GL context
	ofSetupOpenGL(1024,768, OF_WINDOW);			
	
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	
	ofRunApp(new diemDROI());
	
}

// disparity of eyes and depth of screen?
// basic statistics (NATURAL HISTORY), pca
// 18 subjects minimum