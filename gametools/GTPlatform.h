/*
 *  GTPlatform.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 2/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef GTPLATFORM_H
#define GTPLATFORM_H

#include <vector>

namespace gametools {

// This enum lists the known devices for GameTools SDK.
// The order IS important.
//
// Make sure devices are ordered from slowest to fastest.
typedef enum {
	GT_IPHONE1G,
	GT_SYMBIAN_GENERIC,
    GT_ANDROID_GENERIC,
	GT_NOKIA_N8,
	GT_IPHONE3G,
	GT_NOKIA_E7,
	GT_NOKIA_X7,
	GT_IPHONE3GS,
	GT_IPAD1,       // GLBench: 12.4 fps
	GT_IPHONE4,
    GT_IPAD3,       // GLBench: 52.5 fps (iPad3 has many pixels, but not that better of a GPU).
	GT_IPHONE4S,    // GLBench: 58.6 fps
	GT_IPAD_MINI,   // GLBench: 59.5 fps
	GT_IPAD2,       // GLBench: 59.0 fps
    GT_IPHONE5,     // GLBench: 59.6 fps
    GT_IPAD4,       // GLBench: 59.6 fps
	GT_OTHER,
	GT_DESKTOP
} GTPlatform;

void GTSetPlatform(GTPlatform platform);
GTPlatform GTGetPlatform();
float GTGetPlatformVersion();
void GTSetPlatformVersion(float version);

const char *GTGetPlatformString();
void GTSetPlatformString(const char *str);

void  GTSetPlatformPerfo(float index); // 0.5 being "Galaxy S1" (30fps on GLBench 2.1 Egypt Onscreen)
float GTGetPlatformPerfo();

#if 0
const char *GTGetPlatformScreenString();
#ifndef SWIG
std::vector<const char *> GTGetPlatformStrings();
#endif
#endif

bool GTPlatformIsIPhone();
bool GTPlatformIsIPad();
bool GTPlatformIsSymbian();

bool GTPlatformIsSlowerThan(GTPlatform p);
bool GTPlatformIsFasterThan(GTPlatform p);

}

#endif
