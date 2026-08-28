/*
 *  GameTools.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "GameTools.h"

namespace gametools
{
	GTApplicationDelegate *gtApp;
	
	void GTSetDelegate(GTApplicationDelegate &appDelegate) {
		gtApp = &appDelegate;
	}
}
