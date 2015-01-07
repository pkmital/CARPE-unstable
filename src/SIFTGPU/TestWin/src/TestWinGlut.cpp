////////////////////////////////////////////////////////////////////////////
//	File:		TestWinGlut.cpp
//	Author:		Changchang Wu
//	Description : Implementation of TestWinGlut Class
//
//
//
//	Copyright (c) 2007 University of North Carolina at Chapel Hill
//	All Rights Reserved
//
//	Permission to use, copy, modify and distribute this software and its
//	documentation for educational, research and non-profit purposes, without
//	fee, and without a written agreement is hereby granted, provided that the
//	above copyright notice and the following paragraph appear in all copies.
//	
//	The University of North Carolina at Chapel Hill make no representations
//	about the suitability of this software for any purpose. It is provided
//	'as is' without express or implied warranty. 
//
//	Please send BUG REPORTS to ccwu@cs.unc.edu
//
////////////////////////////////////////////////////////////////////////////


#include "GL/glut.h"
#include <stdio.h>
#include <stdlib.h>
#include "BasicTestWin.h"
#include "TestWinGlut.h"

//LIBRARIES 


#ifdef _WIN32
	#ifdef _DEBUG
		#pragma comment(lib, "../lib/testbase_d.lib")
		#pragma comment(lib, "../lib/siftgpu_d.lib")
	#else
		#pragma comment(lib, "../lib/testbase.lib")
		#pragma comment(lib, "../lib/siftgpu.lib")
	#endif
#endif


///main etry

int main(int argc, char**argv)
{
	//
	////uncomment this if you want to parse glut parameters

	glutInit(&argc, argv);


	//create the glut window
	TestWinGlut twg(argc, argv);
//	TestWinGlut twg2(0, NULL);


	//run the glut main loop to display all the TestWinGlut Windows
	TestWinGlut::Run();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// TestWinGlut Class
//////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
///Global Variables
//////////////////////////////////////////////////////////////////////
TestWinGlut* TestWinGlut::_win[TestWinGlut::MAX_TEST_WIN_GLUT];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



TestWinGlut::TestWinGlut(int argc, char**argv)

{
	//enable console output
	SetVerbose();

	//create glut window
	CreateGLUT();

	//parse parameters and run sift 
	RunSiftGPU(argc, argv);


}

TestWinGlut::~TestWinGlut()
{

}



void TestWinGlut::CreateGLUT()
{
	int id;
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize (600,450);
	id = glutCreateWindow ("SIFT_GPU");
	if(id>0)
	{
		if(id >=MAX_TEST_WIN_GLUT) exit(0);//should not happen...
		
		//
		glutDisplayFunc (display);
		glutKeyboardFunc(keyboard);
		glutReshapeFunc (reshape);
		glutIdleFunc(idle);
		glutMotionFunc(motion);
		glutMouseFunc(button);
		//save a pointer to the stucture
		_win[id] = this;
	}

}

void TestWinGlut::idle() 
{
	int id = glutGetWindow();
	_win[id]->OnIdle();
}

void TestWinGlut::keyboard(unsigned char key, int x, int y)
{
	int id = glutGetWindow();

	_win[id]->KeyInput(key);
	  glutPostRedisplay();
}
void TestWinGlut::reshape(int w, int h)
{
	int id = glutGetWindow();
	_win[id]->ReShape(w, h); 
	glutPostRedisplay();
}
void TestWinGlut::display()
{
	static int firstcall=1;
	int id = glutGetWindow();
	_win[id]->Display();
	glutSwapBuffers();
	if(firstcall ==0)
	{
	}else
	{
		//if it is the first display call, redraw
		firstcall = 0;
		glutPostRedisplay();
	}
}

void TestWinGlut::Run()
{
	glutMainLoop();
}

void TestWinGlut::motion(int x, int y)
{
	int id = glutGetWindow();
	_win[id]->MoveMouse(x, y);
}

void TestWinGlut::SetWindowTitle(char *title)
{
	glutSetWindowTitle(title);
}

void TestWinGlut::button(int button, int state,int x, int y)
{
	int id = glutGetWindow();
	if (button ==  GLUT_LEFT_BUTTON)
	{
		if(state == GLUT_DOWN)
			_win[id]->StartMotion(x, y);
		else if (state == GLUT_UP)
			_win[id]->EndMotion();
	}
}

void TestWinGlut::UpdateDisplay()
{
		glutPostRedisplay();
}

void TestWinGlut::SetDisplaySize(int w, int h)
{
	glutReshapeWindow(w, h);
}
