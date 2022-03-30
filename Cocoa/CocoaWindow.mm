//
//  CocoaWindow.m
//  Acinonyx
//
//  Created by Simon Urbanek
//  Copyright 2008 Simon Urbanek. All rights reserved.
//

#import "CocoaWindow.h"
#import "AWindow.h"
#import "GLString.h"

static const char *default_font = "Futura"; //"Futura Condensed ExtraBold"; //"Trebuchet MS Bold"; // "Futura"

class ACocoaWindow : public AWindow {
	CocoaWindow *window;
	NSFont *font;
	char *font_name;
	double font_size;
public:
	ACocoaWindow(CocoaWindow *window, ARect frame) : AWindow(frame) {
		this->window = window;
		font_name = strdup(default_font);
		font_size = 10.0;
		font = [NSFont fontWithName:[NSString stringWithUTF8String:font_name] size:font_size];
		if (!font) font = [NSFont userFontOfSize:font_size];
		if (font) [font retain];
	}

	virtual ~ACocoaWindow() {
		if (font) [font release];
		if (font_name) AFree(font_name);
	}
	
	virtual void redraw() {
		[window redraw];
	}

	virtual void setTitle(const char *txt) {
		[window setTitle:[NSString stringWithUTF8String:txt]];
	}

	virtual void begin() {
		/* check the backing bounds to support Retina correctly */
		NSRect backingBounds = [[window contentView] convertRectToBacking:[[window contentView] bounds]];

		GLsizei backingPixelWidth  = (GLsizei)(backingBounds.size.width),
		        backingPixelHeight = (GLsizei)(backingBounds.size.height);

		/* set _gl_frame before calling begin() so it can use the correct viewport */
		setPixelSize(backingPixelWidth, backingPixelHeight);
		AWindow::begin();
	}

	virtual void glstring(APoint pt, APoint adj, AFloat rot, const char *txt) {
		NSColor *c = [NSColor colorWithDeviceRed:text_color.r green:text_color.g blue:text_color.b alpha:text_color.a];
		NSDictionary *attr = [[NSDictionary alloc] initWithObjectsAndKeys:font, NSFontAttributeName, c, NSForegroundColorAttributeName, nil];
		// NSLog(@"glstring - text_color = %g,%g,%g,%g = %@", text_color.r, text_color.g, text_color.b, text_color.a, c);
		GLString *gs = [[GLString alloc] initWithString:[NSString stringWithUTF8String:txt] withAttributes:attr color:c];
		NSPoint loc = NSMakePoint(pt.x, pt.y), adjp = NSMakePoint(adj.x, adj.y);
		if (_gl_frame.width != _frame.width)
			[gs genTextureWithScale: _gl_frame.width / _frame.width];
		else
			[gs genTexture];

		[gs drawAtPoint:loc withAdjustment:adjp rotation:rot scale:1.0f];
		[gs release];
		[attr release];
	}
	
	virtual void glfont(const char *name, AFloat size) {
		bool changed = false;
		if (size > 0.0 && size != font_size) {
			changed = true;
			font_size = size;
		}
		if (name && !*name) name = default_font;
		if (name && strcmp(name, font_name)) {
			AFree(font_name);
			font_name = strdup(name);
			changed = true;
		}
		if (changed) {
			NSFont *candidate = nil;
			if (*font_name)
				candidate = [NSFont fontWithName:[NSString stringWithUTF8String:font_name] size: font_size];
			if (!candidate) candidate = [NSFont userFontOfSize: font_size];
			if (candidate) {
#ifdef DEBUG
				NSLog(@"new font: %@ replacing %@", candidate, font);
#endif
				[font release];
				font = [candidate retain];
			}
		}
	}
	
	virtual ASize glbbox(const char *txt) {
		NSDictionary *attr = [[NSDictionary alloc] initWithObjectsAndKeys:font, NSFontAttributeName, nil];
		NSString *ns = [[NSString alloc] initWithUTF8String:txt];
		NSAttributedString *str = [[NSAttributedString alloc] initWithString:ns attributes:attr];
		NSSize ss = [str size];
		[str release];
		[attr release];
		[ns release];
		return AMkSize(ss.width, ss.height);
	}
};

@implementation CocoaWindow

- (id) initWithContentRect: (NSRect) rect visual: (AVisual*) aVisual
{
	self = [super initWithContentRect:rect
							styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask
							  backing:NSBackingStoreBuffered//NSBackingStoreRetained
								defer:YES];
	if (self) {
		aWindow = new ACocoaWindow(self, AMkRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height));
		aWindow->setRootVisual(aVisual);
		NSRect frame = [self contentRectForFrameRect:[self frame]];
		view = [[CocoaView alloc] initWithFrame:frame visual:aVisual];
		[view setAWindow:aWindow];
		[self setOpaque:YES];
		[self setContentView:view];
		[self makeFirstResponder:view];
		[self setAcceptsMouseMovedEvents:YES];
		[self setContentMinSize:NSMakeSize(150.0, 100.0)];
		heartbeatTimer = [[NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(heartbeat:) userInfo:nil repeats:YES] retain];
	}
	return self;
}

- (void) heartbeat: (id) sender
{
	//NSLog(@"%@ heartbeat: aWindow=%p, view=%p, isDirty=")
	int *df;
	if (aWindow && view && (df = aWindow->dirtyFlag) && df[0]) {
		df[0]++;
		if (df[0] > 2)
			[view setNeedsDisplay:YES];
	}
}

- (void) redraw
{
#ifdef DEBUG
	NSLog(@"%@: request redraw", self);
#endif
    // FIXME: [view display] is synchronous and needed for idev() if used with dev.hold()/flush(), however, it's unclear we we are not better off using [view setNeedsDisplay:YES] which is asynchronous
	if (view) [view display];
}

- (void) dealloc
{
#ifdef DEBUG
	NSLog(@"%@: dealloc", self);
#endif
	[heartbeatTimer invalidate];
	[heartbeatTimer release];
	if (view) [view release];
	if (aWindow) aWindow->release();
	[super dealloc];
}

@end
