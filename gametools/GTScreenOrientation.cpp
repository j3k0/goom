/*
 *  GTScreenOrientation.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/11/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "GTScreenOrientation.h"

namespace gametools {

static GTScreenOrientation gtScreenOrientation = GT_ANY_ORIENTATION;
static bool gtIgnoreScreenOrientation = false;
float priv_gtScaleFactor    = 1.0f;
float priv_gtInvScaleFactor = 1.0f;
float priv_gtPhysicalSize   = GTiPhonePhysicalSize();
    
void GTSetScreenOrientation(GTScreenOrientation orientation) {
	gtScreenOrientation = orientation;
}

GTScreenOrientation GTGetScreenOrientation() {
	return gtIgnoreScreenOrientation ? GT_ANY_ORIENTATION : gtScreenOrientation;
}

void GTSetIgnoreScreenOrientation() { gtIgnoreScreenOrientation = true; }

void GTSetScaleFactor(float factor) {
    priv_gtScaleFactor = factor;
    priv_gtInvScaleFactor = 1.0f / factor;
}

}
