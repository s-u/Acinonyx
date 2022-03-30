// GLString is based on Apple's CocoaGL example

#import "GLString.h"

// The following is a NSBezierPath category to allow
// for rounded corners of the border

#pragma mark -
#pragma mark NSBezierPath Category

@implementation NSBezierPath (RoundRect)

+ (NSBezierPath *)bezierPathWithRoundedRect:(NSRect)rect cornerRadius:(float)radius {
	NSBezierPath *result = [NSBezierPath bezierPath];
	[result appendBezierPathWithRoundedRect:rect cornerRadius:radius];
	return result;
}

- (void)appendBezierPathWithRoundedRect:(NSRect)rect cornerRadius:(float)radius {
	if (!NSIsEmptyRect(rect)) {
		if (radius > 0.0) {
			// Clamp radius to be no larger than half the rect's width or height.
			float clampedRadius = MIN(radius, 0.5 * MIN(rect.size.width, rect.size.height));
			
			NSPoint topLeft = NSMakePoint(NSMinX(rect), NSMaxY(rect));
			NSPoint topRight = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
			NSPoint bottomRight = NSMakePoint(NSMaxX(rect), NSMinY(rect));
			
			[self moveToPoint:NSMakePoint(NSMidX(rect), NSMaxY(rect))];
			[self appendBezierPathWithArcFromPoint:topLeft     toPoint:rect.origin radius:clampedRadius];
			[self appendBezierPathWithArcFromPoint:rect.origin toPoint:bottomRight radius:clampedRadius];
			[self appendBezierPathWithArcFromPoint:bottomRight toPoint:topRight    radius:clampedRadius];
			[self appendBezierPathWithArcFromPoint:topRight    toPoint:topLeft     radius:clampedRadius];
			[self closePath];
		} else {
			// When radius == 0.0, this degenerates to the simple case of a plain rectangle.
			[self appendBezierPathWithRect:rect];
		}
	}
}

@end


#pragma mark -
#pragma mark GLString

// GLString follows

@implementation GLString

#pragma mark -
#pragma mark Deallocs

- (void) deleteTexture
{
	if (texName && cgl_ctx) {
		(*cgl_ctx->disp.delete_textures)(cgl_ctx->rend, 1, &texName);
		texName = 0; // ensure it is zeroed for failure cases
		cgl_ctx = 0;
	}
}

- (void) dealloc
{
	[self deleteTexture];
	[textColor release];
	[boxColor release];
	[borderColor release];
	[string release];
	[super dealloc];
}

#pragma mark -
#pragma mark Initializers

// designated initializer
- (id) initWithAttributedString:(NSAttributedString *)attributedString withTextColor:(NSColor *)text withBoxColor:(NSColor *)box withBorderColor:(NSColor *)border
{
	[super init];
	cgl_ctx = NULL;
	texName = 0;
	texSize.width = 0.0f;
	texSize.height = 0.0f;
	texScale = 1.0f;
	[attributedString retain];
	string = attributedString;
	[text retain];
	[box retain];
	[border retain];
	textColor = text;
	boxColor = box;
	borderColor = border;
	staticFrame = NO;
	antialias = YES;
	marginSize.width = 0.0f; // 4.0f; // standard margins
	marginSize.height = 0.0f; // 2.0f;
	cRadius = 4.0f;
	requiresUpdate = YES;
	// all other variables 0 or NULL
	return self;
}

- (id) initWithString:(NSString *)aString withAttributes:(NSDictionary *)attribs withTextColor:(NSColor *)text withBoxColor:(NSColor *)box withBorderColor:(NSColor *)border
{
	return [self initWithAttributedString:[[[NSAttributedString alloc] initWithString:aString attributes:attribs] autorelease] withTextColor:text withBoxColor:box withBorderColor:border];
}

// basic methods that pick up defaults
- (id) initWithAttributedString:(NSAttributedString *)attributedString;
{
	return [self initWithAttributedString:attributedString withTextColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:1.0f] withBoxColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f] withBorderColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f]];
}

- (id) initWithString:(NSString *)aString withAttributes:(NSDictionary *)attribs color:(NSColor*)color
{
	return [self initWithAttributedString:[[[NSAttributedString alloc] initWithString:aString attributes:attribs] autorelease] withTextColor:color withBoxColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f] withBorderColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f]];
}

- (id) initWithString:(NSString *)aString withAttributes:(NSDictionary *)attribs
{
	return [self initWithAttributedString:[[[NSAttributedString alloc] initWithString:aString attributes:attribs] autorelease] withTextColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:1.0f] withBoxColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f] withBorderColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:0.0f]];
}

- (void) genTexture {
	[self genTextureWithScale: 1.0f];
}

/* scale is pixels per point, typically 2 for Retina back-ends */
- (void) genTextureWithScale: (float) scale;
{
	NSBitmapImageRep * bitmap;

	NSSize previousSize = texSize;

	// make sure the frame size is valid
	[self frameSize];

	/* textures cannot have odd sizes, make sure the texture size is valid */
	unsigned int texW = (unsigned int) ((frameSize.width + 0.5) * scale);
	unsigned int texH = (unsigned int) ((frameSize.height + 0.5) * scale);
	if (texW < 4) texW = 4;
	if (texW & 1) texW++;
	if (texH < 4) texH = 4;
	if (texH & 1) texH++;

	/* create a new bitmap image rep to draw into */
	bitmap = [[NSBitmapImageRep alloc]
		     initWithBitmapDataPlanes:nil
				   pixelsWide:texW
				   pixelsHigh:texH
				bitsPerSample:8
			      samplesPerPixel:4
				     hasAlpha:true
				     isPlanar:false
			       colorSpaceName:NSCalibratedRGBColorSpace
				 bitmapFormat:0
				  bytesPerRow:texW * 4
				 bitsPerPixel:0];
#ifdef DEBUG
	NSLog(@"textColor=%@", textColor);
#endif
	/* set the logical size to match the frame */
	bitmap.size = frameSize;

	NSGraphicsContext *ctx = [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap];
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext: ctx];
	[textColor set]; 
	[string drawAtPoint:NSMakePoint (marginSize.width, marginSize.height)]; // draw at offset position
	[ctx flushGraphics];
	[NSGraphicsContext restoreGraphicsState];

	texSize.width = [bitmap pixelsWide];
	texSize.height = [bitmap pixelsHigh];
	texScale = scale;

#ifdef DEBUG
	unsigned char *foo = [bitmap bitmapData];
	NSLog(@"text: \"%@\", size: %f x %f -> %d x %d", string, frameSize.width, frameSize.height, texW, texH);
	printf("texture [");
	{ int i; for (i = 0; i < 32; i++) printf("%02x%s", (int) foo[i + texW * 6 * 4 + 8], ((i & 3) == 3) ? " " : "-"); }
	printf("]\n");
	/*
	 unsigned char R = ((unsigned char)([textColor redComponent] * 255.0));
	 unsigned char G = ((unsigned char)([textColor greenComponent] * 255.0));
	 unsigned char B = ((unsigned char)([textColor blueComponent] * 255.0));
	 #define SC(X) ((unsigned char)(((unsigned int)(X) * (unsigned int)(a)) >> 8))
	{ int i = 0, n = (int) [bitmap pixelsWide] * (int) [bitmap pixelsHigh]; for(; i < n; i++) { unsigned char a = foo[i * 4 + 3]; if (a) { foo[i * 4] = SC(R); foo[i * 4 + 1] = SC(G); foo[i * 4 + 2] = SC(B); } } }
	 */
#endif

	if ((cgl_ctx = CGLGetCurrentContext())) { // if we successfully retrieve a current context (required)
		glPushAttrib(GL_TEXTURE_BIT);
		if (0 == texName) glGenTextures (1, &texName);
		glBindTexture (GL_TEXTURE_RECTANGLE_EXT, texName);
		if (NSEqualSizes(previousSize, texSize)) {
			glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT,0,0,0,texSize.width,texSize.height,[bitmap hasAlpha] ? GL_RGBA : GL_RGB,GL_UNSIGNED_BYTE,[bitmap bitmapData]);
		} else {
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, texSize.width, texSize.height, 0, [bitmap hasAlpha] ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, [bitmap bitmapData]);
		}
		glPopAttrib();
	} else
		NSLog (@"StringTexture -genTexture: Failure to get current OpenGL context");
	
	[bitmap release];
	
	requiresUpdate = NO;
}

#pragma mark -
#pragma mark Accessors

- (GLuint) texName
{
	return texName;
}

- (NSSize) texSize
{
	return texSize;
}

#pragma mark Text Color

- (void) setTextColor:(NSColor *)color // set default text color
{
	[color retain];
	[textColor release];
	textColor = color;
	requiresUpdate = YES;
}

- (NSColor *) textColor
{
	return textColor;
}

#pragma mark Box Color

- (void) setBoxColor:(NSColor *)color // set default text color
{
	[color retain];
	[boxColor release];
	boxColor = color;
	requiresUpdate = YES;
}

- (NSColor *) boxColor
{
	return boxColor;
}

#pragma mark Border Color

- (void) setBorderColor:(NSColor *)color // set default text color
{
	[color retain];
	[borderColor release];
	borderColor = color;
	requiresUpdate = YES;
}

- (NSColor *) borderColor
{
	return borderColor;
}

#pragma mark Margin Size

// these will force the texture to be regenerated at the next draw
- (void) setMargins:(NSSize)size // set offset size and size to fit with offset
{
	marginSize = size;
	if (NO == staticFrame) { // ensure dynamic frame sizes will be recalculated
		frameSize.width = 0.0f;
		frameSize.height = 0.0f;
	}
	requiresUpdate = YES;
}

- (NSSize) marginSize
{
	return marginSize;
}

#pragma mark Antialiasing
- (BOOL) antialias
{
	return antialias;
}

- (void) setAntialias:(bool)request
{
	antialias = request;
	requiresUpdate = YES;
}


#pragma mark Frame

- (NSSize) frameSize
{
	if ((NO == staticFrame) && (0.0f == frameSize.width) && (0.0f == frameSize.height)) { // find frame size if we have not already found it
		frameSize = [string size]; // current string size
		frameSize.width += marginSize.width * 2.0f; // add padding
		frameSize.height += marginSize.height * 2.0f;
	}
	return frameSize;
}

- (BOOL) staticFrame
{
	return staticFrame;
}

- (void) useStaticFrame:(NSSize)size // set static frame size and size to frame
{
	frameSize = size;
	staticFrame = YES;
	requiresUpdate = YES;
}

- (void) useDynamicFrame
{
	if (staticFrame) { // set to dynamic frame and set to regen texture
		staticFrame = NO;
		frameSize.width = 0.0f; // ensure frame sizes will be recalculated
		frameSize.height = 0.0f;
		requiresUpdate = YES;
	}
}

#pragma mark String

- (void) setString:(NSAttributedString *)attributedString // set string after initial creation
{
	[attributedString retain];
	[string release];
	string = attributedString;
	if (NO == staticFrame) { // ensure dynamic frame sizes will be recalculated
		frameSize.width = 0.0f;
		frameSize.height = 0.0f;
	}
	requiresUpdate = YES;
}

- (void) setString:(NSString *)aString withAttributes:(NSDictionary *)attribs; // set string after initial creation
{
	[self setString:[[[NSAttributedString alloc] initWithString:aString attributes:attribs] autorelease]];
}


#pragma mark -
#pragma mark Drawing

- (void) drawWithBounds:(NSRect)bounds
{
	if (requiresUpdate)
		[self genTexture];
	if (texName) {
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT); // GL_COLOR_BUFFER_BIT for glBlendFunc, GL_ENABLE_BIT for glEnable / glDisable
		
		glDisable (GL_DEPTH_TEST); // ensure text is not remove by depth buffer test.
//		glEnable (GL_BLEND); // for text fading
//		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // ditto
		glDisable(GL_BLEND);
		glEnable (GL_TEXTURE_RECTANGLE_EXT);  
		
		glBindTexture (GL_TEXTURE_RECTANGLE_EXT, texName);
#if FLIP_TEX /* this was the original code but we need a flipped version */
		glBegin (GL_QUADS);
		glTexCoord2f (0.0f, 0.0f); // draw upper left in world coordinates
		glVertex2f (bounds.origin.x, bounds.origin.y);
		
		glTexCoord2f (0.0f, texSize.height); // draw lower left in world coordinates
		glVertex2f (bounds.origin.x, bounds.origin.y + bounds.size.height);
		
		glTexCoord2f (texSize.width, texSize.height); // draw upper right in world coordinates
		glVertex2f (bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height);
		
		glTexCoord2f (texSize.width, 0.0f); // draw lower right in world coordinates
		glVertex2f (bounds.origin.x + bounds.size.width, bounds.origin.y);
		glEnd ();
#else
		glBegin (GL_QUADS);
		glTexCoord2f (0.0f, 0.0f); // draw upper left in world coordinates
		glVertex2f (bounds.origin.x, bounds.origin.y + bounds.size.height);
		
		glTexCoord2f (0.0f, texSize.height); // draw lower left in world coordinates
		glVertex2f (bounds.origin.x, bounds.origin.y);
		
		glTexCoord2f (texSize.width, texSize.height); // draw upper right in world coordinates
		glVertex2f (bounds.origin.x + bounds.size.width, bounds.origin.y);
		
		glTexCoord2f (texSize.width, 0.0f); // draw lower right in world coordinates
		glVertex2f (bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height);
		glEnd ();
#endif
		glPopAttrib();
	}
}

- (void) drawAtPoint:(NSPoint)point withAdjustment: (NSPoint) adj rotation: (float) rot scale: (float) scale
{
	if (requiresUpdate)
		[self genTexture];
	if (texName) {
		NSPoint ll, lr, ul, ur;
        
		ll = point;
		double th = rot * M_PI / 180.0; // theta
		double cth = cos(th), sth = sin(th); // cos(theta), sin(theta)
		// base point in x (width) and y (height) direction (delta from point of text origin)
		lr.x = (texSize.width * scale / texScale) * cth;
		lr.y = (texSize.width * scale / texScale) * sth;
		ul.x = - (texSize.height * scale / texScale) * sth;
		ul.y = (texSize.height * scale / texScale) * cth;
		// diagonal point
		ur.x = lr.x + ul.x;
		ur.y = lr.y + ul.y;
		// multiply adj by the diagonal
		//adj.x *= - ur.x;
		//adj.y *= - ur.y;
		// adjust the origin

#ifdef DEBUG
		glBegin(GL_LINE_STRIP);
		glColor4f(0.0, 0.0, 1.0, 0.5);
		glVertex2f(ul.x + ll.x, ul.y + ll.y);
		glVertex2f(ll.x, ll.y);
		glColor4f(0.0, 1.0, 0.0, 0.5);
		glVertex2f(lr.x + ll.x, lr.y + ll.y);
		glEnd();
#endif
		
		ll.x += - adj.x * lr.x - adj.y * ul.x;
		ll.y += - adj.y * ul.y - adj.x * lr.y;
		// make sure the texture is pixel-aligned
		ll.x = round(ll.x) - 0.5;
		ll.y = round(ll.y) - 0.5;
		// adjust all other points according to the text origin
		lr.x += ll.x;
		lr.y += ll.y;
		ul.x += ll.x;
		ul.y += ll.y;
		ur.x += ll.x;
		ur.y += ll.y;

#ifdef DEBUG
		glBegin(GL_LINE_STRIP);
		glColor4f(1.0, 0.0, 0.0, 0.5);
		glVertex2f(ul.x, ul.y);
		glVertex2f(ll.x, ll.y);
		glColor4f(0.0, 1.0, 0.0, 0.5);
		glVertex2f(lr.x, lr.y);
		glEnd();
#endif
		
		glColor4f([textColor redComponent], [textColor greenComponent], [textColor blueComponent], [textColor alphaComponent]);
		// ALog("points: A(%g,%g) B(%g,%g), C(%g,%g), D(%g,%g) (th=%g, cth=%g, sth=%g)\n", ll.x, ll.y, lr.x, lr.y, ur.x, ur.y, ul.x, ul.y, th, cth, sth);
		
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT); // GL_COLOR_BUFFER_BIT for glBlendFunc, GL_ENABLE_BIT for glEnable / glDisable
		
		glDisable (GL_DEPTH_TEST); // ensure text is not remove by depth buffer test.
		glEnable (GL_BLEND); // for text fading
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // ditto
		glEnable (GL_TEXTURE_RECTANGLE_EXT);  
		
		glBindTexture (GL_TEXTURE_RECTANGLE_EXT, texName);
		glBegin (GL_QUADS);
		glTexCoord2f (0.0f, 0.0f); // draw upper left in world coordinates
		glVertex2f (ul.x, ul.y);
		
		glTexCoord2f (0.0f, texSize.height); // draw lower left in world coordinates
		glVertex2f (ll.x, ll.y);
		
		glTexCoord2f (texSize.width, texSize.height); // draw lower right in world coordinates
		glVertex2f (lr.x, lr.y);
		
		glTexCoord2f (texSize.width, 0.0f); // draw upper right in world coordinates
		glVertex2f (ur.x, ur.y);
		glEnd ();

		glPopAttrib();
	}		
}

- (void) drawAtPoint:(NSPoint)point
{
	if (requiresUpdate)
		[self genTexture]; // ensure size is calculated for bounds
	if (texName) // if successful
		[self drawWithBounds:NSMakeRect (point.x, point.y, texSize.width*0.5, texSize.height*0.5)];
}

@end
