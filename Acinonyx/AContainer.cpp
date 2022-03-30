/*
 *  AContainer.cpp
 *  Acinonyx
 *
 *  Created by Simon Urbanek
 *  Copyright 2008 Simon Urbanek. All rights reserved.
 *
 */

#include "AContainer.h"

#include "AVisual.h"
#include "AWindow.h"
#include "ATools.h"

void AWindow::draw() {
	AVisual *rv = (AVisual*) _rootVisual;
	if (rv) {
		// default font ("" stands for user/default font)
		// FIXME: 10.0 font size works well on OS X - check other platforms...
		rv->font("", 10.0);

		if (dirtyFlag && dirtyFlagLayer < _redraw_layer)
			_redraw_layer = dirtyFlagLayer;
		vsize_t layer = _redraw_layer;
		while (layer < _max_layers) {
			rv->clipOff();
			setTextColor(::textColor); // default text color - black
			ALog("  - draw layer %d\n", layer);
			rv->draw(layer);
			freezeLayer(layer++);
		}
		ALog("  - draw layer %d (transient)\n", LAYER_TRANS);
		rv->draw(LAYER_TRANS); // the transient layer is not stored
		_redraw_layer = LAYER_TRANS;
	}
}

bool AWindow::event(AEvent event)
{
	AVisual *rv = (AVisual*) _rootVisual;
	return rv ? rv->event(event) : false;
}

void AWindow::setFrame(ARect frame) {
	if (frame.width != _frame.width || frame.height != _frame.height) {
		/* NOTE: _gl_frame is adjusted in begin() */
		_gl_frame = _frame = frame;
		setRedrawLayer(LAYER_ROOT);
		AVisual *rv = (AVisual*) _rootVisual;
		if (rv)
			rv->moveAndResize(AMkRect(0.0, 0.0, frame.width, frame.height));
	}
}

