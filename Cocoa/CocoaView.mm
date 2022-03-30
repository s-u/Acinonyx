//
//  CocoaView.m
//  Acinonyx
//
//  Created by Simon Urbanek
//  Copyright 2008 Simon Urbanek. All rights reserved.
//

#import "CocoaView.h"

#include "ATypes.h"
#include "AScatterPlot.h"

/* for pre-10.5 compatibility */
#ifndef NSINTEGER_DEFINED
#if __LP64__ || NS_BUILD_32_LIKE_64
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif
#define NSINTEGER_DEFINED 1
#endif

// conversion between Cocoa events and AEvents
static int NSEvent2AEFlags(NSEvent *e) {
	int flags = 0;
	NSUInteger ef = [e modifierFlags];
	if (ef & NSShiftKeyMask) flags |= AEF_SHIFT;
	if (ef & NSControlKeyMask) flags |= AEF_CTRL;
	if (ef & NSAlternateKeyMask) flags |= AEF_ALT;
	if (ef & NSCommandKeyMask) flags |= AEF_META;
	return flags;
}

static APoint NSEventLoc2AEPoint(NSEvent *e) {
	NSPoint pt = [e locationInWindow];
	return AMkPoint(pt.x, pt.y);
}

@implementation CocoaView

- (id)initWithFrame:(NSRect)frame visual: (AVisual*) aVisual {
	unsigned int attrs[] = {
		NSOpenGLPFAAccelerated,
//		NSOpenGLPFAColorSize, 24,
//		NSOpenGLPFAAlphaSize, 16,
		NSOpenGLPFANoRecovery,
#ifdef PFA
		 NSOpenGLPFASampleBuffers, 1, NSOpenGLPFASamples, 4,
#endif
		/* <- anti-aliasing */
	0 };

	self = [super initWithFrame:frame pixelFormat:[[NSOpenGLPixelFormat alloc] initWithAttributes:(NSOpenGLPixelFormatAttribute*)attrs]];
	//NSLog(@"***=== CocoaView: initWithFrame: %f,%f,%f,%f", frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
	if (self) {
		// Should we use hi-res on Retina?
		[self setWantsBestResolutionOpenGLSurface:YES];
		visual = (AVisual*) aVisual->retain();
	}
	return self;
}

- (void)drawRect:(NSRect)rect {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSRect frame = [self frame];
	/*NSLog(@" CocoaView frame = %f,%f - %f x %f\n", frame.origin.x,frame.origin.y,frame.size.width,frame.size.height); */

	/*NSLog(@"OpenGL:\n - vendor = '%s'\n - renderer = '%s'\n - version = '%s'\n - exts = '%s'",
	glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_EXTENSIONS)); */
	
	ARect nvframe = AMkRect(frame.origin.x,frame.origin.y,frame.size.width,frame.size.height);
	 /*
	  ARect vframe = visual->frame();
	  if (!ARectsAreEqual(nvframe, vframe))
	  visual->moveAndResize(nvframe);
	 */

	AWindow *win = visual->window();
	if (win) {
		ARect wframe = win->frame();
		if (!ARectsAreEqual(nvframe, wframe))
			win->setFrame(nvframe);
		win->begin();
		win->draw();
		win->end();
	}

	[pool release];
}

- (void) setAWindow: (AWindow*) aWin
{
	if (visual) visual->setWindow(aWin);
}

- (void)mouseDown:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_DOWN, NSEvent2AEFlags(theEvent) | AEF_BUTTON1, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_DOWN, NSEvent2AEFlags(theEvent) | AEF_BUTTON2, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_MOVE, NSEvent2AEFlags(theEvent), 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_MOVE, NSEvent2AEFlags(theEvent) | AEF_BUTTON1, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_MOVE, NSEvent2AEFlags(theEvent) | AEF_BUTTON2, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)mouseUp:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_UP, NSEvent2AEFlags(theEvent) | AEF_BUTTON1, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_MOUSE_UP, NSEvent2AEFlags(theEvent) | AEF_BUTTON2, 0, NSEventLoc2AEPoint(theEvent)));
}

- (void)keyDown:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_KEY_DOWN, NSEvent2AEFlags(theEvent), [theEvent keyCode], NSEventLoc2AEPoint(theEvent)));
}

- (void)keyUp:(NSEvent *)theEvent
{
	visual->event(AMkEvent(AE_KEY_UP, NSEvent2AEFlags(theEvent), [theEvent keyCode], NSEventLoc2AEPoint(theEvent)));
}

- (BOOL)isOpaque { return YES; }

/*
- (BOOL) performKeyEquivalent: (NSEvent*) event
{
	NSLog(@"%@: performKeyEquivalent: %@", self, event);
	return NO;
}*/

- (void) dealloc
{
	NSLog(@"%@: dealloc", self);
	if (visual) visual->release();
	[super dealloc];
}

@end
