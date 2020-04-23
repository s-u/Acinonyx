/*
 *  ATypes.h - basic Acinonyx types
 *  Acinonyx
 *
 *  Created by Simon Urbanek
 *  Copyright 2008 Simon Urbanek. All rights reserved.
 *
 *  lang: C
 */

#ifndef A_TYPES_H
#define A_TYPES_H

#define PFA 1

#define LAYER_ROOT    0
#define LAYER_HILITE  1
#define LAYER_OBJECTS 2
#define LAYER_WIDGETS 3 /* widgets layer (currently we fold trans and widgets since we use currently cue-widgets but it may change...) */
#define LAYER_TRANS   3 /* transient layer - topmost */

#ifdef __cplusplus
extern "C" {
#endif
	
	/* basic types */

	typedef double AFloat; /**< basic floating point type used throughout Acinonyx for graphics purposes */
	
	typedef unsigned int vsize_t; /**< basic unsigned type for iterations and sized in Acinonyx */
	
	typedef unsigned int notifid_t; /**< notification ID type */ 

	typedef unsigned char byte_t; /**< unsigend 8-bit type for general storage */
	
	typedef struct ARect_s { AFloat x,y,width,height; } ARect; /**< structure representing a rectangular region by progin and size */
	typedef struct APoint_s { AFloat x,y; } APoint; /**< structure describing a point */
	typedef struct ASize_s { AFloat width, height; } ASize; /**< structure describing size */

	typedef struct ADataRange_s { double begin, length; } ADataRange; /**< structure describing a range of data points */
	typedef struct ARange_s { AFloat begin, length; } ARange; /**< structure describing a range of graphics points */

	typedef struct AColor_s { AFloat r,g,b,a; } AColor; /**< structure describing RGBA color */
	
	/** strcture describing an event in Acinonyx */
	typedef struct AEvent_s {
		int event;
		int flags;
		int key;
		APoint location;
	} AEvent;

#define AMkRect(x,y,w,h) ((ARect) { (AFloat)(x),(AFloat)(y),(AFloat)(w),(AFloat)(h) })
#define AMkPoint(x,y) ((APoint) { (AFloat)(x),(AFloat)(y) })
#define AMkSize(w,h) ((ASize) { (AFloat)(w),(AFloat)(h) })
#define AMkRange(b,l) ((ARange) { (AFloat)(b),(AFloat)(l) })
#define AMkDataRange(b,l) ((ADataRange) { (double)(b),(double)(l) })
#define AMkEvent(e,f,k,l) ((AEvent) { (int)(e),(int)(f),(int)(k),(APoint)(l) })
#define AMkColor(r,g,b,a) ((AColor) { (AFloat)(r),(AFloat)(g),(AFloat)(b),(AFloat)(a) })

#define AMAX(X,Y) (((X) > (Y))?(X):(Y))
#define AMIN(X,Y) (((X) < (Y))?(X):(Y))
	
#define AUndefSize AMkSize(-1.0f, -1.0f)
	// FIXME: AUndefRange needs a more reasonable definition
#define AUndefRange AMkRange(0.0f, -1.0f)
#define AUndefDataRange AMkDataRange(0.0, -1.0)

#define ANotFound ((vsize_t) -1)
	
#define APointsAreEqual(A,B) (((A).x == (B).x) && ((A).y == (B).y))
#define ASizesAreEqual(A,B) (((A).width == (B).width) && ((A).height == (B).height))
#define ARectsAreEqual(A,B) (APointsAreEqual(A,B) && ASizesAreEqual(A,B))
#define ADataRangesAreEqual(A,B) (((A).begin == (B).begin) && ((A).length == (B).length))
#define ARectContains(A,B) (((B).x >= (A).x) && ((B).x <= (A).x + (A).width) && ((B).y >= (A).y) && ((B).y <= (A).y + (A).height)) 
#define ARectsIntersect(A,B) (!(((A).x + (A).width < (B).x) || ((A).x > (B).x + (B).width) || ((A).y + (A).height < (B).y) || ((A).y > (B).y + (B).height)))
#define ARect4(X) (X).x, (X).y, (X).width, (X).height
#define AColorsAreEqual(A,B) (((A).r == (B).r) && ((A).g == (B).g) && ((A).b == (B).b) && ((A).a == (B).a))

#define AEF_BUTTON1 0x001
#define AEF_BUTTON2 0x002
#define AEF_BUTTON3 0x004
#define AEF_BUTTONS 0x007
#define AEF_SHIFT   0x010
#define AEF_CTRL    0x020
#define AEF_ALT     0x040
#define AEF_META    0x080
#define AEF_MKEYS   0x0f0

#define AE_MOUSE_DOWN   0x0101
#define AE_MOUSE_UP     0x0102
#define AE_MOUSE_CLICK  0x0103
#define AE_MOUSE_2CLICK 0x0104

#define AE_MOUSE_IN     0x0106
#define AE_MOUSE_OUT    0x0107

#define AE_MOUSE_MOVE   0x0108

#define AE_KEY_DOWN     0x0201
#define AE_KEY_UP       0x0202
#define AE_KEY_TYPED    0x0203

#define KEY_LEFT  123
#define KEY_RIGHT 124
#define KEY_DOWN  125
#define KEY_UP    126

#define KEY_A	    0
#define KEY_S       1
#define KEY_H	    4
#define KEY_C       8
#define KEY_X       7
#define KEY_0      29
#define KEY_L      37
#define KEY_U	   32
#define KEY_I	   34
#define KEY_1      18
#define KEY_2      19
#define KEY_3      20
#define KEY_4      21
#define KEY_5      23
	
/** This macro is used after memory allocation to check for out of memory issues */
#define AMEM(x) // FIXME: replace with some error handling if x is NULL
	
#include <limits.h>
	
extern double NA_double;
extern float NA_float;
#define NA_int INT_MIN

#ifdef EDEBUG
#ifdef DEBUG
#define ELog ALog
#else
#define ELog printf
#endif
#endif
	
	int R_IsNA(double x);

/* AinNA should be used to check for the presence of NA in the data */
#define AisNA(X) R_IsNA(X)
	
#ifdef __cplusplus
}
#endif	

#endif
