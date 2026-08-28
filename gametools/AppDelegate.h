/*
 *  AppDelegate.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef APP_DELEGATE_H
#define APP_DELEGATE_H

#include "Commander.h"

namespace gametools {
	
	class GTApplicationDelegate {
	public:
		virtual ~GTApplicationDelegate() {}

		virtual const CommanderDataPackage& initialDataPackage(const char *locale) = 0;
		virtual void mainScreenIsReady(Screen &screen) {}
		virtual void mainScreenDidResize(int w, int h) {}
		virtual void applicationDidFinishLaunching()   {}
        virtual void applicationWillGoInactive()       {}
		
		virtual void buttonL1(bool state) {}
		virtual void buttonL2(bool state) {}
		virtual void buttonR1(bool state) {}
		virtual void buttonR2(bool state) {}
	};
	
	void GTSetDelegate(GTApplicationDelegate &appDelegate);

	extern GTApplicationDelegate *gtApp;
}

#endif
