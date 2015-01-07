////////////////////////////////////////////////////////////////////////////
//	File:		GLTestWnd.cpp
//	Author:		Changchang Wu
//	Description : implementation of the GLTestWnd class.
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


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <windowsx.h>
#include "GL/gl.h"
////
#include "stdlib.h"
#include "BasicTestWin.h"
#include "GLTestWnd.h"

#ifdef _WIN32
	#ifdef _DEBUG
		#pragma comment(lib, "../lib/testbase_d.lib")
		#pragma comment(lib, "../lib/siftgpu_d.lib")
	#else
		#pragma comment(lib, "../lib/testbase.lib")
		#pragma comment(lib, "../lib/siftgpu.lib")
	#endif
#endif
 
//

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	MSG msg;
	HACCEL hAccelTable = NULL;

	//create a window
	if(!GLTestWnd::CreateInstance(hInstance, lpCmdLine))
	{
		return FALSE;
	}


	//use accelerators if you want to
	//hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)....);
	
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
		
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//onidle
		if(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)==0)SendMessage(GetActiveWindow(), WM_MY_IDLE, 0, 0);
	}
	return msg.wParam;
}

////
HINSTANCE GLTestWnd::_hInst=NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


GLTestWnd::GLTestWnd(LPSTR cmd)
{
	//Parse Command Line and Run SIFT
	_cmd = cmd;
}
GLTestWnd::~GLTestWnd()
{

}
LRESULT CALLBACK GLTestWnd::___WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int nWnd=0;
	GLTestWnd *pWnd;
	if(message==WM_CREATE)
	{
		LPSTR cmd = (LPSTR)((LPCREATESTRUCT)lParam)->lpCreateParams;
		pWnd=new GLTestWnd (cmd);
		pWnd->_hWndMain = hWnd;
		SetWindowLong(hWnd,0,(long)(pWnd));	
		nWnd++;

	}else if(message== WM_DESTROY)
	{
		pWnd=(GLTestWnd*)GetWindowLong(hWnd,0);
		delete pWnd;
		SetWindowLong(hWnd,0,0);
		nWnd--;
		if(nWnd==0)
			PostQuitMessage(0);
		pWnd = NULL;
	}else
	{
		pWnd=(GLTestWnd*)GetWindowLong(hWnd,0);
	}
	if(pWnd)
		return pWnd->_WndProc(message,wParam,lParam);
	else
		return DefWindowProc(hWnd, message, wParam, lParam);
	
}


BOOL GLTestWnd::CreateInstance(HINSTANCE hInstance,  LPSTR lpCmdLine)
{
	HWND hWnd;
	WNDCLASSEX wcex;
	TCHAR szTitle[]="SIFT_GPU";								// The title bar text
	TCHAR szWindowClass[]="SIFT_GPU_WND";	


	_hInst = hInstance;
	///
	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)___WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 4;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL; //LoadIcon(hInstance, (LPCTSTR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//(LPCSTR);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;
	RegisterClassEx(&wcex);

	hWnd = CreateWindow(
		szWindowClass, 		szTitle, 
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, 600, 450, 
		NULL, NULL, 		hInstance, 		lpCmdLine	);
 
	if (!hWnd)
	{
      return FALSE;
	}

	ShowWindow(hWnd,  SW_SHOW);
	UpdateWindow(hWnd);

   return TRUE;
}
LRESULT GLTestWnd::_WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{


	switch (message) 
	{
		case WM_CREATE:
			{
				SetDisplaySize(600, 450);
				HDC hdc	=	GetDC(_hWndMain);
				glCreateRC(hdc);
				if(_hglrc == NULL) exit(0);
				ReleaseDC(_hWndMain,hdc);

			}
			break;
		case WM_SIZE:
			{
				glResize(LOWORD(lParam),HIWORD(lParam));
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(_hWndMain, &ps);
				///
				glPaint(hdc);
				///
				EndPaint(_hWndMain, &ps);
			}
			break;
		case WM_CHAR:
			KeyInput(wParam);
			InvalidateRect(_hWndMain, NULL, FALSE);
			break;
		case WM_LBUTTONDOWN:
			{
				int xPos = GET_X_LPARAM(lParam); 
				int yPos = GET_Y_LPARAM(lParam); 
				StartMotion(xPos, yPos);
			}
			break;
		case WM_LBUTTONUP:
			EndMotion();
			break;
		case WM_MOUSEMOVE:
			if( wParam & MK_LBUTTON)
			{
				int xPos = GET_X_LPARAM(lParam); 
				int yPos = GET_Y_LPARAM(lParam); 
				MoveMouse(xPos, yPos);
			}
		case WM_ERASEBKGND:
			return TRUE;
		case WM_MY_IDLE:
			OnIdle();
			return TRUE;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(_hWndMain, message, wParam, lParam);
	}
	return 0;
}
void GLTestWnd::glCreateRC(HDC hdc)
{
    int pixelformat;
	PIXELFORMATDESCRIPTOR pfd = 
	{
        sizeof(PIXELFORMATDESCRIPTOR),1,                             
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,		
        PFD_TYPE_RGBA,24,0, 0, 0, 0, 0, 0,0,0,0,0, 0, 0, 0,16,0,0,                          
        PFD_MAIN_PLANE,0,0, 0, 0                     
    };
    if ((pixelformat = ChoosePixelFormat(hdc, &pfd)) ==0)return ;
    if (SetPixelFormat(hdc, pixelformat, &pfd) == FALSE)return ;
	pixelformat =::GetPixelFormat(hdc);
	::DescribePixelFormat(hdc, pixelformat, sizeof(pfd), &pfd);

	//
    _hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, _hglrc);
//	glClearDepth(-1.0f);
//	glEnable(GL_DEPTH_TEST);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glEnable(GL_NORMALIZE);

//	HCURSOR cb = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
	RunSiftGPU(_cmd);
//	SetCursor(cb);
	//
	wglMakeCurrent(NULL, NULL);
 
}

void GLTestWnd::glResize(int w, int h)
{
	ReShape(w, h);
}

void GLTestWnd::glPaint(HDC hdc)
{
	wglMakeCurrent(hdc,HGLRC(_hglrc));
	Display();
	SwapBuffers(hdc);
}

void GLTestWnd::RunSiftGPU(LPSTR cmd)
{
	int argc=0;
	char**argv = new char*[256];
	if(*cmd == 0) return;
	do
	{
		while(*cmd ==' ') cmd++;
		if(*cmd)
		{
			argv[argc++] = cmd;
		}
		while(*cmd && *cmd != ' ') cmd++;
		if(*cmd==' ')	*cmd++ = 0;

	}while(*cmd && argc <256);
	BasicTestWin::RunSiftGPU(argc, argv);
}

void GLTestWnd::SetDisplaySize(int w, int h)
{
	RECT rc;	int dw,  dh;
	GetClientRect(_hWndMain, &rc);

	dw = w - rc.right;
	dh = h - rc.bottom;
	GetWindowRect(_hWndMain, &rc);
	
	MoveWindow(_hWndMain, rc.left, rc.top, rc.right - rc.left + dw,
		rc.bottom - rc.top + dh, TRUE);

}

void GLTestWnd::SetWindowTitle(char *title)
{
	SetWindowText(_hWndMain, title);
}

void GLTestWnd::UpdateDisplay()
{
	InvalidateRect(_hWndMain, NULL,0);
}
