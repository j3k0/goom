/*
 *  GTScreenOrientation.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/11/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef GTScreenOrientation_H
#define GTScreenOrientation_H

namespace gametools {

typedef enum {
	GT_PORTRAIT, GT_LANDSCAPE, GT_ANY_ORIENTATION
} GTScreenOrientation;
	
void GTSetScreenOrientation(GTScreenOrientation orientation);
GTScreenOrientation GTGetScreenOrientation();
void GTSetIgnoreScreenOrientation();
	
// Those two are made public for optimization purposes.
extern float priv_gtScaleFactor;
extern float priv_gtInvScaleFactor;
extern float priv_gtPhysicalSize;
    
void GTSetScaleFactor(float factor);
inline float GTGetScaleFactor() { return priv_gtScaleFactor; }
inline float GTGetInvScaleFactor() { return priv_gtInvScaleFactor; }

// Physical size (diagonal) of the screen (in meters).
inline void  GTSetPhysicalSize(float psize)    { priv_gtPhysicalSize = psize; }
inline float GTGetPhysicalSize() { return priv_gtPhysicalSize; }
inline void  GTSetPhysicalSizeInch(float inch) { priv_gtPhysicalSize = inch * 0.0254f; }
inline float GTGetPhysicalSizeInch() { return priv_gtPhysicalSize / 0.0254f; }

inline float GTiPhonePhysicalSize() { return 0.089f; } // Size for an iPhone
inline float GTiPadPhysicalSize()   { return 0.254f; } // Size for an iPad

    
}

#endif
