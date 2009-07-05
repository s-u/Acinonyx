/*
 *  AWin32Window.cpp
 *  Acinonyx
 *
 *  Created by Simon Urbanek on 7/1/09.
 *  Copyright 2009 Simon Urbanek. All rights reserved.
 *
 */

#include "AWin32Window.h"

#ifdef WIN32

extern "C" {
	AWin32Window *AWin32_CreateWindow(AVisual *visual, APoint position);
}

AWin32Window *AWin32_CreateWindow(AVisual *visual, APoint position)
{
	ARect frame = AMkRect(position.x, position.y, visual->frame().width, visual->frame().height);
	AWin32Window *win = new AWin32Window(frame);
	win->setRootVisual(visual);
	visual->setWindow(win);
	return win;
}

static void HelpClose(window w)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	if (win) win->close();
}

static void HelpExpose(window w, rect r)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	if (win) win->expose();
}

static void HelpResize(window w, rect r)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	if (win) win->resize(AMkRect(0.0, 0.0, r.width, r.height));
}

static unsigned int geventFlags(int button b) {
	unsigned int f = 0;
	if (b == 1) f |= AEF_BUTTON1;
	if (b == 2) f |= AEF_BUTTON2;
	if (b == 3) f |= AEF_BUTTON3;
	return f;
}

static void HelpMouseClick(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
}

static void HelpMouseMove(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	win->event(AMkEvent(AE_MOUSE_MOVE, geventFlags(button), 0, AMkPoint(pt.x, pt.y)));
}

static void HelpMouseUp(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	win->event(AMkEvent(AE_MOUSE_UP, geventFlags(button), 0, AMkPoint(pt.x, pt.y)));
}

static void HelpMouseDown(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	win->event(AMkEvent(AE_MOUSE_DOWN, geventFlags(button), 0, AMkPoint(pt.x, pt.y)));
}

static void HelpKeyDown(control w, int key)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	win->event(AMkEvent(AE_KEY_DOWN, geventFlags(0), key, AMkPoint(0.0, 0.0)));
}

static void HelpKeyAction(control w, int key)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
}

static void SetupPixelFormat(HDC hDC)
{
	int nPixelFormat;
	
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
#ifndef USE_GDI
		| PFD_DOUBLEBUFFER
#endif
		,
		PFD_TYPE_RGBA,
		32,                                     //32 bit color mode
		0, 0, 0, 0, 0, 0,                       //ignore color bits
		0,                                      //no alpha buffer
		0,                                      //ignore shift bit
		0,                                      //no accumulation buffer
		0, 0, 0, 0,                             //ignore accumulation bits
		16,                                     //16 bit z-buffer size
		0,                                      //no stencil buffer
		0,                                      //no aux buffer
		PFD_MAIN_PLANE,                         //main drawing plane
		0,                                      //reserved
		0, 0, 0 
	};                              //layer masks ignored
	
	nPixelFormat = ChoosePixelFormat(hDC, &pfd);
#ifdef DEBUG
	Rprintf("ChoosePixelFormat(%x): %x\n", (int) hDC, (int) nPixelFormat);
#endif
	SetPixelFormat(hDC, nPixelFormat, &pfd);
}

#endif