/*
 *  GameTools.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef GameTools_H
#define GameTools_H

#include <sstream>

#include "AppDelegate.h"
#include "vec3.h"
#include "drawcontext.h"
#include "GTScreenOrientation.h"
#include "GTPlatform.h"
#include "audio.h"
#include "GTRenderer.h"
#include "NotifyCenter.h"
#include "OverlayManager.h"
#include "FPDataPathManager.h"
#include "GTPreferences.h"
#include "GTScrollingAxisManager.h"
#include "GTCloud.h"

namespace gametools {
#define GT_PACKAGE_NUMBER_VLD  9
#define GT_PACKAGE_NUMBER_LD  99
#define GT_PACKAGE_NUMBER_HD 999

	void GTInit(int argc, char ** argv);
	int  GTMain(int argc, char ** argv);
	void GTExit(int exitCode);
	bool GTPaused(); // Returns true if the application is paused.

	inline float frand() { return ((double)rand() / (double)RAND_MAX); }
	inline float fsign(float f) { return (f < 0.0f ? -1.0f : 1.0f); }
	inline float fabs(float f) { return (f < 0.0f ? -f : f); }
    inline std::string nospace(const std::string str) {
        std::stringstream ss;
        int i = 0;
        while (i < str.length()) {
            if (str[i] != ' ') ss << str[i];
            ++i;
        }
        return ss.str();
    }

    // Just usefull.
    class ActionContainer : public AbsoluteContainer, public Action, public IdleComponent {
    public:
        virtual gametools::IdleComponent *getIdleComponent() { return this; }
    };

#ifdef __GNUC__
# define   GTLikely(x)       __builtin_expect((x),1)
# define   GTUnlikely(x)     __builtin_expect((x),0)
# define   GTPrefetch(addr)  __builtin_prefetch(addr)
#else
# define   GTLikely(x)      (x)
# define   GTUnlikely(x)    (x)
# define   GTPrefetch(addr) (addr)
#endif

}

#include "GTLog.h"

#endif
