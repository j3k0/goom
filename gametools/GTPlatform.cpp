/*
 *  GTPlatform.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 2/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "GTLog.h"
#include "GTPlatform.h"
#include <sstream>
#include <stdio.h>

namespace gametools {

static GTPlatform gtPlatform = GT_OTHER;

void GTSetPlatform(GTPlatform platform) {
	gtPlatform = platform;
    GTLogf("GTPlatform: %s", GTGetPlatformString());
    switch (platform) {
        case GT_IPAD1:       // GLBench: 12.4 fps
            GTSetPlatformPerfo(12.4 / 60.0);
            break;
        case GT_IPAD3:
            GTSetPlatformPerfo(52.5 / 60.0);
            break;
        case GT_IPHONE4S:
        case GT_IPHONE5:
        case GT_IPAD_MINI:
        case GT_IPAD2:
        case GT_IPAD4:
            GTSetPlatformPerfo(1.0f);
            break;
        default:
            return;
    }
}

GTPlatform GTGetPlatform() {
	return gtPlatform;
}

static std::string gPlatformString = "";

const char *GTGetPlatformString() {
    if (gPlatformString == "") {
        switch (gtPlatform) {
            case GT_NOKIA_N8:        return "nokia-n8";
            case GT_NOKIA_E7:        return "nokia-e7";
            case GT_NOKIA_X7:        return "nokia-x7";
            case GT_SYMBIAN_GENERIC: return "symbian";
            case GT_ANDROID_GENERIC: return "android";
            case GT_IPHONE1G:        return "iphone-1g";
            case GT_IPHONE3G:        return "iphone-3g";
            case GT_IPHONE3GS:       return "iphone-3gs";
            case GT_IPHONE4:         return "iphone-4";
            case GT_IPHONE4S:        return "iphone-4s";
            case GT_IPHONE5:         return "iphone-5";
            case GT_IPAD1:           return "ipad-1";
            case GT_IPAD2:           return "ipad-2";
            case GT_IPAD3:           return "ipad-3";
            case GT_IPAD4:           return "ipad-4";
            case GT_IPAD_MINI:       return "ipad-mini";
            case GT_DESKTOP:         return "desktop";
            default:                 return "other";
        }
    }
    else
        return gPlatformString.c_str();
}

void GTSetPlatformString(const char *str) {
    gPlatformString = str;
}

static float gPlatformPerfo = 0.0f;
void  GTSetPlatformPerfo(float index) { gPlatformPerfo = index; }
float GTGetPlatformPerfo() { return gPlatformPerfo; }

#if 0
    DROPPED FEATURE
const char *GTGetPlatformScreenString() {
	switch (gtPlatform) {
		case GT_NOKIA_N8:        return "small";
		case GT_NOKIA_E7:        return "small";
		case GT_NOKIA_X7:        return "small";
		case GT_SYMBIAN_GENERIC: return "tiny";
		case GT_IPHONE1G:        return "small";
		case GT_ANDROID_GENERIC: return "small";
		case GT_GALAXY_Y:        return "tiny";
		case GT_GALAXY_S1:       return "small";
		case GT_GALAXY_TAB1:     return "small";
		case GT_IPHONE3G:        return "small";
		case GT_IPHONE3GS:       return "small";
		case GT_IPHONE4:         return "large";
		case GT_IPHONE4S:        return "large";
		case GT_IPHONE5:         return "large";
		case GT_IPAD1:           return "large";
		case GT_IPAD2:           return "large";
		case GT_IPAD_MINI:       return "large";
		case GT_DESKTOP:         return "huge";
		default:                 return "large";
	}
}

std::vector<const char *> GTGetPlatformStrings() {
    std::vector<const char *>ret;
	// ret.push_back(GTGetPlatformString());
    ret.push_back(GTGetPlatformScreenString());
    /*
    switch (gtPlatform) {
		case GT_NOKIA_N8:
		case GT_SYMBIAN_GENERIC:
		case GT_NOKIA_E7:
		case GT_NOKIA_X7:
        case GT_GALAXY_Y:
			ret.push_back("tiny");
			break;
		case GT_IPHONE1G:
            ret.push_back("small");
            break;
		case GT_IPHONE3G:
		case GT_IPHONE3GS:
            ret.push_back("small");
            break;
		case GT_IPHONE4:
		case GT_IPHONE4S:
            ret.push_back("small");
            break;
		case GT_IPAD1:
            ret.push_back("small");
            break;
		case GT_IPAD2:
            ret.push_back("small");
            break;
		case GT_DESKTOP:
            ret.push_back("large");
            ret.push_back("small");
            break;
		default:
            ret.push_back("small");
            ret.push_back("large");
	}
    */
    return ret;
}
#endif

static float g_pVersion = 0;
float GTGetPlatformVersion() {
	return g_pVersion;
}
void GTSetPlatformVersion(float version) {
	char tmp[1024];
	sprintf(tmp, "GTPlatform Version: %1.1f", version);
	GTLog(tmp);
	g_pVersion = version;
}
	
bool GTPlatformIsIPhone() {
	return (gtPlatform == GT_IPHONE1G)
      || (gtPlatform == GT_IPHONE3G)
      || (gtPlatform == GT_IPHONE3GS)
      || (gtPlatform == GT_IPHONE4)
      || (gtPlatform == GT_IPHONE4S)
      || (gtPlatform == GT_IPHONE5);
}

bool GTPlatformIsIPad() {
	return (gtPlatform == GT_IPAD1)
        || (gtPlatform == GT_IPAD2)
        || (gtPlatform == GT_IPAD_MINI)
        || (gtPlatform == GT_IPAD3)
        || (gtPlatform == GT_IPAD4);
}

bool GTPlatformIsSymbian() {
#ifdef SYMBIAN
    return true;
	//return (gtPlatform == GT_NOKIA_N8)
    //  ||   (gtPlatform == GT_NOKIA_E7)
    //  ||   (gtPlatform == GT_NOKIA_X7)
    //  ||   (gtPlatform == GT_SYMBIAN_GENERIC);
#else
    return false;
#endif
}

bool GTPlatformIsAndroid() {
#ifdef ANDROID
    return true;
#else
    return false;
#endif
}

bool GTPlatformIsSlowerThan(GTPlatform p) {
    return (int)gtPlatform < (int)p;
}

bool GTPlatformIsFasterThan(GTPlatform p) {
    return (int)gtPlatform > (int)p;
}

}
