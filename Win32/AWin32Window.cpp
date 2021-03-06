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

static int lastButtonState = 0;
static APoint lastMousePos;

#ifndef WGL_FONTS
AFreeType *sharedFT;
#endif

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


static unsigned int button2flags(int b) {
	unsigned int f = 0;
	if (b&1) f |= AEF_BUTTON1;
	if (b&4) f |= AEF_BUTTON2;
	if (b&2) f |= AEF_BUTTON3;
	return f;
}

static unsigned int update_key_state() {
	// reset modifier key states
	lastButtonState &= ~AEF_MKEYS;
	if (GetAsyncKeyState(VK_SHIFT) & 0xfff0)   lastButtonState |= AEF_SHIFT;
	if (GetAsyncKeyState(VK_CONTROL) & 0xfff0) lastButtonState |= AEF_CTRL;
	if (GetAsyncKeyState(VK_MENU) & 0xfff0)    lastButtonState |= AEF_ALT;
	return lastButtonState;
}

#define gPoint(X,Y) AMkPoint(X, _frame.height - Y) 

static void HelpMouseClick(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
}

static void HelpMouseMove(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	ARect _frame = win->frame();
	win->event(AMkEvent(AE_MOUSE_MOVE, update_key_state(), 0, lastMousePos = gPoint(pt.x, pt.y)));
}

static void HelpMouseUp(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	ARect _frame = win->frame();
#ifdef DEBUG
	Rprintf("mouseUp, button=%d, pt=%g,%g\n", button, (double)pt.x, (double)pt.y);
#endif
	unsigned int f = button2flags(button), g = f ^ lastButtonState;
	lastButtonState = g;
	update_key_state();
	win->event(AMkEvent(AE_MOUSE_UP, g, 0, lastMousePos = gPoint(pt.x, pt.y)));
}

static void HelpMouseDown(window w, int button, point pt)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
	ARect _frame = win->frame();
	lastButtonState |= button2flags(button);
	update_key_state();
#ifdef DEBUG
	Rprintf("mouseDown, button=%d, pt=%g,%g\n", button, (double)pt.x, (double)pt.y);
#endif
	win->event(AMkEvent(AE_MOUSE_DOWN, lastButtonState, 0, lastMousePos = gPoint(pt.x, pt.y)));
}

static void HelpKeyDown(control w, int key)
{
	AWin32Window *win = (AWin32Window*) getdata(w);
#ifdef DEBUG
	Rprintf("keyDown, key=%d\n", key);
#endif
	switch (key) {
		case '0': key = KEY_0; break;
		case 'a': key = KEY_A; break;
		case 's': key = KEY_S; break;
		case 'c': key = KEY_C; break;
		case 'h': key = KEY_H; break;
		case 'l': key = KEY_L; break;
		case 'u': key = KEY_U; break;
	}
	update_key_state();
	win->event(AMkEvent(AE_KEY_DOWN, lastButtonState, key, lastMousePos));
}

static void HelpKeyAction(control w, int key)
{
	int ac_key = 0;
	AWin32Window *win = (AWin32Window*) getdata(w);
#ifdef DEBUG
	Rprintf("keyAction, key=%d\n", key);
#endif
	switch (key) {
		case 8592: ac_key = KEY_LEFT; break;
		case 8593: ac_key = KEY_UP; break;
		case 8594: ac_key = KEY_RIGHT; break;
		case 8595: ac_key = KEY_DOWN; break;
	}
	update_key_state();
	if (ac_key) win->event(AMkEvent(AE_KEY_DOWN, lastButtonState, ac_key, lastMousePos));
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

void AWin32Window::heartbeat() {
	if (dirtyFlag && dirtyFlag[0]) {
		dirtyFlag[0]++;
		if (dirtyFlag[0] > 2)
			redraw();
	}
}
	
static DWORD WINAPI AWin32Heartbeat( LPVOID lpParam ) {
	AWin32Window *win = (AWin32Window*) lpParam;
	win->retain();
	while (win->active()) {
		Sleep(200);
		win->heartbeat();
	}
}

#endif
