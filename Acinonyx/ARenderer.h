/*
 *  ARenderer.h - class mapping rendering API to OpenGL
 *  Acinonyx
 *
 *  Created by Simon Urbanek
 *  Copyright 2008 Simon Urbanek. All rights reserved.
 *
 */

// coords: world -> screen -> window -> container

/* Notes:
  - layers could be implemented using display lists
 
 //	creating a display list
 GLuint listname = glGenLists( 1 );
 glNewList( listname, GL_COMPILE or GL_COMPILE_AND_EXECUTE );
 // glBegin/glEnd ...  
 glEndList();
 
 //	drawing with a display list
 glCallList( listname );
 
 //	disposing of a display list
 glDeleteLists( listname, 1 );
 
 http://developer.apple.com/graphicsimaging/opengl/optimizingdata.html */

#ifndef A_RENDERER_H_
#define A_RENDERER_H_

#include "AOpenGL.h"
#include "AObject.h"
#include "AWindow.h"
#include <math.h>

#define CIRCLE_STEPS 10

// possibly set to a custom obejcts tracking static renderer allocation domain
#define ARendererAllocationDomain NULL

static void tcbCombine(GLdouble coords[3], 
					   GLdouble *vertex_data[4],
					   GLfloat weight[4], GLdouble **dataOut )
{
	GLdouble *vertex;
	
	vertex = (GLdouble *) A_alloc(3, sizeof(GLdouble), ARendererAllocationDomain);
	AMEM(vertex);
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];
	/*
	for (int i = 3; i < 7; i++)
		vertex[i] = weight[0] * vertex_data[0][i] 
		+ weight[1] * vertex_data[1][i]
		+ weight[2] * vertex_data[2][i] 
		+ weight[3] * vertex_data[3][i];  */
	*dataOut = vertex;
}

static const AFloat sin_table8[] = {0,0.2225209,0.4338837,0.6234898,0.7818315,0.9009689,0.9749279,1};

class ARenderer : public AObject {
protected:
	ARect _frame; // in window coords
	AWindow *_window;
	int *dirtyFlag;
	int dirtyFlagLayer;
	GLUtesselator *tess;
public:

	ARenderer(AWindow *aWindow, ARect frame) : _frame(frame), _window(aWindow), dirtyFlag(0), dirtyFlagLayer(-1), tess(0) { OCLASS(ARenderer) }

	virtual ~ARenderer() {
		if (tess)
			gluDeleteTess(tess);
		DCLASS(ARenderer);
	}
	
	void setFrame(ARect frame) { _frame = frame; }
	ARect frame() { return _frame; }

	void setRedrawLayer(vsize_t layer) {
		if (_window) _window->setRedrawLayer(layer);
	}
	
	AWindow *window() { return _window; }
	virtual void setWindow(AWindow *win) { _window = win; if (dirtyFlag && win) { win->setDirtyFlag(dirtyFlag); if (dirtyFlagLayer >= 0) win->setDirtyFlagLayer(dirtyFlagLayer); } }
	
	bool containsPoint(APoint pt) { return (!((pt.x < _frame.x) || (pt.y < _frame.y) || (pt.x > _frame.x + _frame.width) || (pt.y > _frame.y + _frame.height))); }
	bool intersectsRect(ARect r) { return (!((r.x > _frame.x + _frame.width) || (r.x + r.width < _frame.x) || (r.y > _frame.y + _frame.height) || (r.y + r.height < _frame.y))); }
	
	void redraw() { if (_window) _window->redraw(); }
	void setDirtyFlag(int *df) { dirtyFlag=df; if (_window) _window->setDirtyFlag(df); }
	void setDirtyFlagLayer(vsize_t layer) { dirtyFlagLayer = layer; if (_window) _window->setDirtyFlagLayer(layer); }

	void color(AColor c) {
		glColor4f(c.r, c.g, c.b, c.a);
	}

	void color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
		glColor4f(r, g, b, a);
	}

	void color(GLfloat r, GLfloat g, GLfloat b) {
		glColor3f(r, g, b);
	}
	
	void color255(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
		glColor4b(r, g, b, a);
	}
	
/*	void color(const AFloat col[4]) {
		float f[4] = { col[0], col[1], col[2], col[3] };
		glColor4fv(f);
	}*/
	
	AColor txtcolor() {
		return _window ? _window->textColor() : AMkColor(0,0,0,1);
	}
	
	void txtcolor(AColor c) {
		if (_window) _window->setTextColor(c);
	}
	
	void txtcolor(AFloat r, AFloat g, AFloat b, AFloat a = 1.0) {
		if (_window) _window->setTextColor(AMkColor(r,g,b,a));
	}
	
	void rectV(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
		glVertex2f(x1, y1);
		glVertex2f(x2, y1);
		glVertex2f(x2, y2);
		glVertex2f(x1, y2);
	}

	void rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
		glBegin(GL_QUADS);
		rectV(x1, y1, x2, y2);
		glEnd();
	}

	void rect(ARect aRect) { rect(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height); }

	void roundRect(ARect aRect, AFloat cornerx = 5.0, AFloat cornery = -1.0) { roundRect(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height, cornerx, cornery); }
	void roundRectO(ARect aRect, AFloat cornerx = 5.0, AFloat cornery = -1.0) { roundRectO(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height, cornerx, cornery); }

	void roundRect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, AFloat cornerx = 5.0, AFloat cornery = -1.0) {
		if (cornery < 0.0) cornery = cornerx;
		GLfloat dx = x2 - x1; if (dx < 0) dx = -dx;
		GLfloat dy = y2 - y1; if (dy < 0) dy = -dy;
		if (cornerx > dx / 2.0) cornerx = dx / 2.0;
		if (cornery > dy / 2.0) cornery = dy / 2.0;
		if (cornerx < 1.0 && cornery < 1.0)
			rect(x1, y1, x2, y2);
		else {
			int i = 0;
			glBegin(GL_POLYGON);
			for (i = 0; i < 8; i++) glVertex2f(x1 + cornerx - cornerx * sin_table8[i], y1 + cornery - cornery * sin_table8[7 - i]); // bottom left
			for (i = 0; i < 8; i++) glVertex2f(x1 + cornerx - cornerx * sin_table8[7 - i], y2 - cornery + cornery * sin_table8[i]); // top left
			for (i = 0; i < 8; i++) glVertex2f(x2 - cornerx + cornerx * sin_table8[i], y2 - cornery + cornery * sin_table8[7 - i]); // top right
			for (i = 0; i < 8; i++) glVertex2f(x2 - cornerx + cornerx * sin_table8[7 - i], y1 + cornery - cornery * sin_table8[i]); // bottom right
			glEnd();
		}
	}

	void roundRectO(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, AFloat cornerx = 5.0, AFloat cornery = -1.0) {
		if (cornery < 0.0) cornery = cornerx;
		GLfloat dx = x2 - x1; if (dx < 0) dx = -dx;
		GLfloat dy = y2 - y1; if (dy < 0) dy = -dy;
		if (cornerx > dx / 2.0) cornerx = dx / 2.0;
		if (cornery > dy / 2.0) cornery = dy / 2.0;
		if (cornerx < 1.0 && cornery < 1.0)
			rectO(x1, y1, x2, y2);
		else {
			int i = 0;
			glBegin(GL_LINE_LOOP);
			for (i = 0; i < 8; i++) glVertex2f(x1 + cornerx - cornerx * sin_table8[i], y1 + cornery - cornery * sin_table8[7 - i]); // bottom left
			for (i = 0; i < 8; i++) glVertex2f(x1 + cornerx - cornerx * sin_table8[7 - i], y2 - cornery + cornery * sin_table8[i]); // top left
			for (i = 0; i < 8; i++) glVertex2f(x2 - cornerx + cornerx * sin_table8[i], y2 - cornery + cornery * sin_table8[7 - i]); // top right
			for (i = 0; i < 8; i++) glVertex2f(x2 - cornerx + cornerx * sin_table8[7 - i], y1 + cornery - cornery * sin_table8[i]); // bottom right
			glEnd();
		}
	}
	
	void rectO(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
		glBegin(GL_LINE_LOOP);
		rectV(x1, y1, x2, y2);
		glEnd();
	}

	void rectO(ARect aRect) { rectO(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height); }

	void point(AFloat x, AFloat y) {
		glBegin(GL_POINTS);
		glVertex2f(x, y);
		glEnd();
	}
	
	void point(APoint p) { point(p.x, p.y); }
	
	void points(const APoint *p, int n) {
		int i = 0;
		glBegin(GL_POINTS);
		while (i < n) {
			glVertex2f(p[i].x, p[i].y);
			i++;
		}
		glEnd();
	}
	
	void circle(AFloat x, AFloat y, AFloat r) {
		double rho = 0.0;
		vsize_t step = (r < 1.0) ? 4 : ((r < 5.0) ? 8 : (r * 2));
		double step_a =  2.0 * 3.1415926535897 / ((double) step);
		glBegin(GL_POLYGON);
		for (vsize_t i = 0; i < step; i++) {
			glVertex2f(x + sin(rho) * r, y + cos(rho) * r);
			rho += step_a;
		}
		glEnd();
	}

	void circleO(AFloat x, AFloat y, AFloat r) {
		double rho = 0.0;
		vsize_t step = (r < 1.0) ? 4 : ((r < 5.0) ? 8 : (r * 2));
		double step_a =  2.0 * 3.1415926535897 / ((double) step);
		glBegin(GL_LINE_LOOP);
		for (vsize_t i = 0; i < step; i++) {
			glVertex2f(x + sin(rho) * r, y + cos(rho) * r);
			rho += step_a;
		}
		glEnd();
	}
	
	void font(const char *name, AFloat size) {
		if (_window) _window->glfont(name, size);
	}
	
	void text(const APoint pt, const char *txt) {
		if (_window) _window->glstring(pt, AMkPoint(0,0), 0.0, txt);
	}

	void text(const APoint pt, const char *txt, APoint adj, AFloat rot=0.0) {
		if (_window) _window->glstring(pt, adj, rot, txt);
	}
	
	void text(AFloat x, AFloat y, const char *txt, AFloat rot=0.0) {
		if (_window) _window->glstring(AMkPoint(x,y), AMkPoint(0,0), rot, txt);
	}
	
	ASize bbox(const char *txt) {
		return (_window) ? _window->glbbox(txt) : AMkSize(strlen(txt) * 5.6, 10.0); // a crude fall-back
	}
	
	void clip(ARect where) {
		if (_window) where = _window->xy2pixel(where);
		glScissor(where.x, where.y, where.width, where.height);
		ALog("clip to (%g,%g - %g,%g)", where.x, where.y, where.width, where.height);
		glEnable(GL_SCISSOR_TEST);
	}
	
	void clipOff() {
		ALog("disable clipping");
		glDisable(GL_SCISSOR_TEST);
	}
	
	void points(const AFloat *x, const AFloat *y, int n) {
		int i = 0;
		glBegin(GL_POINTS);
		while (i < n) {
			glVertex2f(x[i], y[i]);
			i++;
		}
		glEnd();
	}

	void segments(const APoint *p1, const APoint *p2, int n) {
		int i = 0;
		glBegin(GL_LINES);
		while (i < n) {
			glVertex2f(p1[i].x, p1[i].y);
			glVertex2f(p2[i].x, p2[i].y);
			i++;
		}
		glEnd();
	}
	
	void line(AFloat x1, AFloat y1, AFloat x2, AFloat y2) {
		glBegin(GL_LINE_STRIP);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
		glEnd();
	}
	
	void lineFrom(AFloat x, AFloat y) {
		glBegin(GL_LINE_STRIP);
		glVertex2f(x, y);
	}
	
	void lineBegin() {
		glBegin(GL_LINE_STRIP);
	}
		
	void lineTo(AFloat x, AFloat y) {
		glVertex2f(x, y);
	}
	
	void lineEnd() {
		glEnd();
	}
	
	void polyV(const AFloat *x, const AFloat *y, int n) {
		int i = 0;
		while (i < n) {
			glVertex2f(x[i], y[i]);
			i++;
		}
	}
	
	void polyP(const APoint *pt, int n) {
		int i = 0;
		while (i < n) {
			glVertex2f(pt[i].x, pt[i].y);
			i++;
		}
	}

	void polygon(const APoint *pt, int n, bool convex=false) {
		if (convex || n < 4) { /* up to 3 points it's guaranteed convex */
			polyP(pt, n);
			glEnd();
			return;
		}
		/* otherwise we have to tessellate */
		if (!tess)
			tess = gluNewTess();
		/* not sure how portable this is - it won't work if glXX are macros ... */
#ifdef WIN32
		gluTessCallback(tess, GLU_TESS_BEGIN, glBegin);
		gluTessCallback(tess, GLU_TESS_END, (GLvoid (*) ( )) &glEnd);
		gluTessCallback(tess, GLU_TESS_VERTEX, (GLvoid (*) ()) &glVertex3dv);
#else
		gluTessCallback(tess, GLU_TESS_BEGIN, (GLvoid (*) ( )) &glBegin);
		gluTessCallback(tess, GLU_TESS_END, (GLvoid (*) ( )) &glEnd);
		gluTessCallback(tess, GLU_TESS_VERTEX, (GLvoid (*) ()) &glVertex3dv);
#endif
		/* the only one we really implement ourself */
		gluTessCallback(tess, GLU_TESS_COMBINE, (GLvoid (*) ()) &tcbCombine);
		/* set the winding rule to non-zero (the one I like the best) */
		gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
		/* ok, do it */
		gluTessNormal(tess, 0.0, 0.0, 1.0); /* tell tessellator that we have 2D data */
		gluTessBeginPolygon(tess, NULL);
		gluTessBeginContour(tess);
		GLdouble *v3 = (GLdouble*) AZAlloc(sizeof(GLdouble) * 3, n);
		AMEM(v3);
		for(vsize_t i = 0; i < n; i++) {
			//GLdouble vc[3];
			//vc[0] = pt[i].x; vc[1] = pt[i].y; vc[2] = 0.0;
			v3[i * 3] = pt[i].x;
			v3[i * 3 + 1] = pt[i].y;
			gluTessVertex(tess, v3 + (i * 3), v3 + (i * 3));
		}
		gluTessEndContour(tess);
		gluTessEndPolygon(tess);
		// we don't delete the tessellator since it's likely to be used on next redraw ... gluDeleteTess(tess);
		AFree(v3);
	}
	
	void polygon(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_POLYGON);
		polyV(x, y, n);
		glEnd();
	}

	void polygonO(const APoint *pt, int n) {
		glBegin(GL_LINE_LOOP);
		polyP(pt, n);
		glEnd();
	}
	
	void polygonO(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_LINE_LOOP);
		polyV(x, y, n);
		glEnd();
	}

	void polyline(const APoint *pt, int n) {
		glBegin(GL_LINE_STRIP);
		polyP(pt, n);
		glEnd();		
	}
	
	void polyline(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_LINE_STRIP);
		polyV(x, y, n);
		glEnd();		
	}
	
	void fan(AFloat centerx, AFloat centery, AFloat *x, AFloat *y, int n) {
		glBegin(GL_TRIANGLE_FAN);
		glVertex2f(centerx, centery);
		polyV(x, y, n);
		glEnd();
	}

	void tri(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_TRIANGLES);
		polyV(x, y, n);
		glEnd();
	}
	
	void pointPV(APoint p) {
		glVertex2f(p.x, p.y);
	}
	
	void triP(APoint p1, APoint p2, APoint p3) {
		glBegin(GL_TRIANGLES);
		pointPV(p1);
		pointPV(p2);
		pointPV(p3);
		glEnd();
	}
	
	void trimesh(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_TRIANGLE_STRIP);
		polyV(x, y, n);
		glEnd();
	}

	void quadmesh(const AFloat *x, const AFloat *y, int n) {
		glBegin(GL_QUAD_STRIP);
		polyV(x, y, n);
		glEnd();
	}
};

#endif
