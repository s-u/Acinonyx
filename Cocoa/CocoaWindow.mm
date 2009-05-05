//
//  CocoaWindow.m
//  Acinonyx
//
//  Created by Simon Urbanek
//  Copyright 2008 Simon Urbanek. All rights reserved.
//

#import "CocoaWindow.h"
#import "AWindow.h"

class ACocoaWindow : public AWindow {
	CocoaWindow *window;
public:
	ACocoaWindow(CocoaWindow *window, ARect frame) : AWindow(frame) {
		this->window = window;
	}
	
	virtual void redraw() {
		[window redraw];
	}
};

@implementation CocoaWindow

- (id) initWithContentRect: (NSRect) rect visual: (AVisual*) aVisual
{
	self = [super initWithContentRect:rect
							styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask
							  backing:NSBackingStoreRetained
								defer:NO];
	if (self) {
		aWindow = new ACocoaWindow(self, AMkRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height));
		NSRect frame = [self contentRectForFrameRect:[self frame]];
		view = [[CocoaView alloc] initWithFrame:frame visual:aVisual];
		[self setContentView:view];
		[view setAWindow:aWindow];
	}
	return self;
}

- (void) redraw
{
	NSLog(@"%@: request redraw", self);
	if (view) [view setNeedsDisplay:YES];
}

- (void) dealloc
{
	NSLog(@"%@: dealloc", self);
	if (view) [view release];
	if (aWindow) aWindow->release();
	[super dealloc];
}


@end