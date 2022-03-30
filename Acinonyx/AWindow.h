/*
 *  AWindow.h - abtract Acinonyx window class
 *  Acinonyx
 *
 *  Created by Simon Urbanek
 *  Copyright 2008 Simon Urbanek. All rights reserved.
 *
 *  lang: C++
 */

#ifndef A_WINDOW_H
#define A_WINDOW_H

#include "ATypes.h"
#include "AObject.h"

#ifdef DEBUG
#define GLC(X) { X; int ec = glGetError(); if (ec != GL_NO_ERROR) ALog("*** %s: GL error: %s\n", describe(), gluErrorString(ec)); }
#else
#define GLC(X) X
#endif

typedef GLuint texture_t;

static int first_gl = 1;

// this is static for now -- 0 = plot, 1 = objects, 2 = interaction (not saved as layer)
#define _max_layers LAYER_TRANS

// IDEA: use window-global query object
class AWindow : public AObject
{
protected:
	AObject *modalOwner;
	AObject *_rootVisual;
	ARect _frame, _gl_frame;
	
	unsigned int _layers, _redraw_layer;
	texture_t _layer[_max_layers];
	bool _layers_initialized;
	AColor text_color;
	vsize_t dirtyFlagLayer; ///< if the dirty flag triggers a redraw this variable specifies the redraw layer to use
public:
	int *dirtyFlag;

	AWindow(ARect frame) : modalOwner(0), _rootVisual(0), _frame(frame), dirtyFlag(0) {
		_redraw_layer = _layers = 0;
		_gl_frame = _frame;
		dirtyFlagLayer = LAYER_TRANS; /* by default dirty flag triggers the transient layer only - use setDirtyFlagLayer() to change this */
		text_color = AMkColor(0.0, 0.0, 0.0, 1.0);
		OCLASS(AWindow)
	};

	virtual ~AWindow() {
		if (_rootVisual) _rootVisual->release();
		DCLASS(AWindow)
	}

	// this function is called before the first OpenGL call is made but after at least one context has been created
	// it can detect extensions and setup the behavior ...
	void first_gl_call() {
		ALog("OpenGL version: %s", glGetString(GL_VERSION));
		ALog("OpenGL vendor: %s", glGetString(GL_VENDOR));
		ALog("OpenGL renderer: %s", glGetString(GL_RENDERER));
		ALog("OpenGL extensions: %s", glGetString(GL_EXTENSIONS));
		first_gl = 0;
	}

	void setPixelSize(AFloat x, AFloat y) {
		_gl_frame.width = x;
		_gl_frame.height = y;
	}

	bool freezeLayer(vsize_t layer) {
		glDisable(GL_SCISSOR_TEST);
		if (layer >= _max_layers) return false;
		GLC(glBindTexture(A_TEXTURE_TYPE, _layer[layer]));
		ALog(" -> freeze layer %d (texture %d)", layer, _layer[layer]);
		int width = _gl_frame.width, height = _gl_frame.height;
/*
		int dbuf, rbuf;
		glGetIntegerv(GL_DRAW_BUFFER, &dbuf);
		glGetIntegerv(GL_READ_BUFFER, &rbuf);
		printf("draw buffer: %d, read buffer: %d (back=%d, front=%d)\n", dbuf, rbuf, GL_FRONT, GL_BACK);
 */
		
#if ! A_EXACT_TEXTURE
		int rw = width, rh = height;
		width = height = 32;
		while (width < rw) width <<= 1;
		while (height < rh) height <<= 1;
		ALog("%s: creating POTS texture %d x %d", describe(), width, height);
#endif

		GLC(glCopyTexImage2D(A_TEXTURE_TYPE, 0, GL_RGBA, 0, 0, width, height, 0));
		
#ifdef DEBUG
		unsigned char *foo = (unsigned char*) AAlloc(width * height * 4);
		if (foo) {
			GLC(glGetTexImage(A_TEXTURE_TYPE, 0, GL_RGBA, GL_UNSIGNED_BYTE, foo));
			printf("texture [");
			for (vsize_t i = 0; i < 32; i++) printf("%02x%s", (int) foo[i + width * 40 + 40], ((i & 3) == 3) ? " " : "-");
			printf("]\n");
			AFree(foo);
		}
#endif
#if ! A_EXACT_TEXTURE
		glTexParameteri(A_TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(A_TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
		_redraw_layer = layer;
		if (layer >= _layers) _layers = layer + 1;
		return true;
	}
	
	bool recallLayer(vsize_t layer) {
		if (_layers == 0) return false;
		if (layer >= _layers) layer = _layers - 1; // recall the topmost layer if layer is too high
		ALog(" <- recall layer %d (texture %d)", layer, _layer[layer]);
		glDisable(GL_SCISSOR_TEST);
		
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glDisable(GL_BLEND); // make sure all gets replaced, no blending
		glDisable(GL_DEPTH_TEST);
		glEnable(A_TEXTURE_TYPE);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		GLC(glBindTexture(A_TEXTURE_TYPE, _layer[layer]));
		glBegin(GL_QUADS);
		int width = _frame.width - 1, height = _frame.height - 1;
		int txwidth = _gl_frame.width - 1, txheight = _gl_frame.height - 1;

#if ! A_EXACT_TEXTURE
		int tw = 32, th = 32;
		while (tw < width) tw <<= 1;
		while (th < height) th <<= 1;
		double tpw = (double) width / (double) tw;
		double tph = (double) height / (double) th;
		ALog("%s: recalling POTS texture %d x %d", describe(), tw, th);
#endif
		
#if A_EXACT_TEXTURE
#define TOX - 0.5
#ifdef PFA_SHIFT_Y
#define TOY + 0.5
#else
#define TOY - 0.5
#endif
		glTexCoord2i(0, 0);
		glVertex3f(TOX, TOY, 0);
		glTexCoord2i(0, txheight);
		glVertex3f(TOX, height TOY, 0);
		glTexCoord2i(txwidth, txheight);
		glVertex3f(width TOX, height TOY, 0);
		glTexCoord2i(txwidth, 0);
		glVertex3f(width TOX, TOY, 0);
#undef TO
#else
		glTexCoord2d(0.0, 0.0);
		glVertex3f(-0.5, -0.5, 0);
		glTexCoord2d(0.0, tph);
		glVertex3f(-0.5, height - 0.5, 0);
		glTexCoord2d(tpw, tph);
		glVertex3f(width - 0.5, height - 0.5, 0);
		glTexCoord2d(tpw, 0.0);
		glVertex3f(width - 0.5, -0.5, 0);
#endif
		glEnd();
		glPopAttrib();		
		
		_redraw_layer = layer + 1; // no need to redraw ourself anymore
		return true;
	}
	
	/** specifies the bottom-most layer that needs to be re-drawn in response to the next redraw event. (I.e., 0 means that everything needs to be cleared, 1 means that layer 0 can be restored etc.). Note that between redraws the redraw layer is never increased, so specifying a higher value than the current redraw layer does not change the redraw layer.
	    @param layer that needs to be re-drawn on next redraw event
	 */
	void setRedrawLayer(vsize_t layer) {
		ALog("%s: setRedrawLayer(%d)", describe(), layer);
		if (_redraw_layer > layer)
			_redraw_layer = layer;
	}
	
	/** set the color of subsequent text drawing operations
	    @param col color */
	void setTextColor(AColor col) {
		text_color = col;
	}
	
	/** get the color currently used by text drawing operations
	    @return current text color */
	AColor textColor() { return text_color; }
	
	/** this method is called to setup the OpenGL environemnt on a redraw. */
	virtual void begin() {
		current_frame++;
		profStart()
		if (_layers == 0) {
			if (first_gl)
				first_gl_call();
			GLC(glGenTextures(_max_layers, _layer));
			_layers = _max_layers;
		}
		ALog("%s: begin, redraw layer = %d", describe(), _redraw_layer);
		glViewport(0.0f, 0.0f, _gl_frame.width, _gl_frame.height);
        //glClearColor(0.0, 0.0, 0.0, 0);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0);
		if (_redraw_layer == 0)
			glClear(GL_COLOR_BUFFER_BIT);

#ifndef WIN32
		// glEnable(GL_MULTISAMPLE);
#endif
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
		glHint (GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
		glHint (GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
		// paint the background (for alpha's sake clean buffer is not sufficient)
		glColor4f(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0);
		glEnable(GL_QUADS);
		glVertex3f(-1.0, -1.0, 0.0);
		glVertex3f(-1.0, 1.0, 0.0);
		glVertex3f(1.0, 1.0, 0.0);
		glVertex3f(1.0, -1.0, 0.0);
		glEnd();
#endif
				
		glPushMatrix();
		// change coordinates to graphics coords
		glLoadIdentity();
		glOrtho(0.0f, _frame.width, 0.0f, _frame.height, 0.0f, 1.0f);
		// anti-aliasing trick
		glTranslatef(0.5, 0.5, 0.0); // subpixel shift to make sure that lines don't wash over two integration regions. However, filled areas are probably still in trouble

		if (_redraw_layer > 0)
			recallLayer(_redraw_layer - 1);

		// glColor4f(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0);
		_prof(profReport("$window.begin"))
	}

	/** this method is called to draw in the currently prepared OpenGL scene. It essentially dispatches drawing to different layers by calling the draw method of the root visual for each layer necessary. */
	void draw();
	
	/** this method is called after the drawing in the current context of the window has been finished. */
	void end() {
		_prof(profReport("^window.end"))
		glPopMatrix();
		glFlush();
		_prof(profReport("$window.end"))
	}
	
	/** returns the current redraw layer.
	    @return redraw layer */
	vsize_t redrawLayer() { return _redraw_layer; }

	/** returns the current root visual.
	    @return root visual object on NULL if there is none */
	AObject *rootVisual() { return _rootVisual; }
	
	/** returns the current window frame describing the position and size of the window (without decorations).
	    @return a rectangle describing the drawing plane in screen coordinates */
	ARect frame() { return _frame; }

	/** this method sets the desired frame and resizes all children without triggering any window-system events. Use moveAndResize() instead to trigger an acutal move and/or resize of the visible window.
	    @param frame new location of the window and size of the root plane */
	void setFrame(ARect frame);
	
	/** replaces the current root visual object with another one. Each window contains exactly one root visual object which spans the entire drawing plane of the window. In the root visual object is set to NULL only the window background is painted. This method does NOT perform a redraw or call setWindow!
	    @param rv new root visual or NULL to just remove the current one. The visual is retained by the window while in use. */
	void setRootVisual(AObject *rv) {
		if (_rootVisual) _rootVisual->release();
		_rootVisual = rv;
		if (rv) rv->retain();
	}
	
	// enter modal mode with the given owner. returns false if owner is NULL or if already in modal mode by other owner, true otherwise
	virtual bool enterModal(AObject *owner) {
		if (owner && owner == modalOwner) return true;
		if (modalOwner || !owner) return false;
		modalOwner = owner;
		owner->retain();
		return true;
	}
	
	virtual bool leaveModal(AObject *owner) {
		if (modalOwner && modalOwner == owner) {
			modalOwner->release();
			modalOwner = NULL;
			return true;
		}
		return false;
	}
	
	/** inform the window subsystem that the window needs to be redrawn. The actual redrawing is asynchronous, i.e. several redraw events can be combined before the actual redraw occurs. This method needs to be implemented by every subclass to trigger a redraw, it is a no-op in the AWindow class. */
	virtual void redraw() {};
	
	/* the following are rendering methods that are off-loaded to the window implementation since they are not part of OpenGL */
	virtual void glstring(APoint pt, APoint adj, AFloat rot, const char *txt) {}; //  we need some implementation help here since gl cannot draw text
	virtual void glfont(const char *name, AFloat size) {}
	virtual ASize glbbox(const char *txt) { return AMkSize(strlen(txt) * 5.6, 10.0); } // jsut a very crude fallback

	/* convert points to pixels - needed for some OpenGL functions that operate on pixels such as glScissor */
	virtual ARect xy2pixel(ARect what) {
		if (_gl_frame.width == _frame.width &&
		    _gl_frame.height == _gl_frame.height) return what;
		what.x *= _gl_frame.width / _frame.width;
		what.width *= _gl_frame.width / _frame.width;
		what.y *= _gl_frame.height / _frame.height;
		what.height *= _gl_frame.height / _frame.height;
		return what;
	}

	/** specify a dirty flag - a location in memory that can be used to trigger redraws asynchronously. This flag is checked continuously by the heartbeat thread of the window and a value of 1 will trigger a redraw.
	 @param newDF new dirty flag location - must either point to a location capable of holding an int or be NULL (to disable the dirty flag). Note that the caller is responsible for making sure that the memory location is accessible for the entire lifetime of the window (or until replaced). */
	void setDirtyFlag(int *newDF) { dirtyFlag = newDF; };
	void setDirtyFlagLayer(vsize_t layer) { dirtyFlagLayer = layer; }
	bool isDirty() { return (dirtyFlag && dirtyFlag[0] != 0); }

	bool event(AEvent event);

	virtual void setTitle(const char *txt) {};

	virtual bool canClose() { return true; }
	virtual void close() {};

	virtual void moveAndResize(ARect frame) { setFrame(frame); }
	
	virtual void setVisible(bool flag) {};
	virtual bool visible() { return false; }
};

#endif
