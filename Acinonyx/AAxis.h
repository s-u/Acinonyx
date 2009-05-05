/*
 *  AAxis.h
 *  Acinonyx
 *
 *  Created by Simon Urbanek on 3/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "AVisual.h"

class AAxis : public AVisual {
	AScale *_scale;
public:
	AAxis(AContainer *parent, ARect frame, int flags, AScale *scale) : AVisual(parent, frame, flags), _scale(scale) { scale->retain(); OCLASS(AAxis) }
	virtual ~AAxis() {
		if (_scale) _scale->release();
		DCLASS(AAxis)
	}
};

class AXAxis : public AAxis {
public:
	AXAxis(AContainer *parent, ARect frame, int flags, AScale *scale) : AAxis(parent, frame, flags, scale) { }
	
	virtual void draw() {
		color(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.8);
		rect(_frame);
		color(0.0, 0.0, 1.0, 0.3);
		rectO(_frame);
		color(0.0, 0.0, 0.0, 1.0);
		line(_frame.x, _frame.y + _frame.height, _frame.x + _frame.width, _frame.y + _frame.height);
		text(AMkPoint(_frame.x + _frame.width / 2, _frame.y + _frame.height / 2), AMkPoint(0.5,0.5), "X-AXIS");
	}
};

class AYAxis : public AAxis {
public:
	AYAxis(AContainer *parent, ARect frame, int flags, AScale *scale) : AAxis(parent, frame, flags, scale) { }
	
	virtual void draw() {
		color(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.8);
		rect(_frame);
		color(1.0, 0.0, 0.0, 0.3);
		rectO(_frame);
		color(0.0, 0.0, 0.0, 1.0);
		line(_frame.x + _frame.width, _frame.y, _frame.x + _frame.width, _frame.y + _frame.height);
	}
};