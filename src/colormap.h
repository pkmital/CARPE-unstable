/** Adapted from the Qtk to QT translation interface
 http://savannah.nongnu.org/projects/viewergtkqt/
 Translation of an OpenGL Viewver from gtk to qt
   
 Modified by Parag K. Mital for use without class definitions
 Copyright 2008
 **/

#ifndef _COLORMAP_H_
#define _COLORMAP_H_
typedef void (*colorMapFunc)(unsigned char*,float,float,float);

	void colorMap(unsigned char *rgb,float value,float min,float max);
	void hotColorMap(unsigned char *rgb,float value,float min,float max);
	void coldColorMap(unsigned char *rgb,float value,float min,float max);
	void jetColorMap(unsigned char *rgb,float value,float min,float max);
	void cyclicColorMap(unsigned char *rgb,float value,float min,float max);
	void randColorMap(unsigned char *rgb,float value,float min,float max);
	void grayColorMap(unsigned char *rgb,float value,float min,float max);
	void blueColorMap(unsigned char *rgb,float value,float min,float max);
	static colorMapFunc selectColorMap(int cmp);

#endif
