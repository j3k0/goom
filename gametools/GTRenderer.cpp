/*
 *  GTRenderer.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 2/28/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "GTRenderer.h"

namespace gametools {

void GTRenderer::draw(DrawTarget *dt) {
	if (!m_resourcesLoaded)
		loadResources();
}

void GTRenderer::setOutputRect(float x, float y, float w, float h) {
	m_outputX = x;
	m_outputY = y;
	m_outputW = w;
	m_outputH = h;
}

void GTRenderer::setViewport(float x, float y, float w, float h) {
	m_viewportX = x;
	m_viewportY = y;
	m_viewportW = w;
	m_viewportH = h;
}

}