/*
 *  OverlayManager.cpp
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 3/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "OverlayManager.h"
#include "ios_filepath.h"

namespace gametools {

static OverlayManager *gtOverlayManager = (OverlayManager*)0;

OverlayManager &GTGetOverlayManager() {
    if (gtOverlayManager == 0)
        gtOverlayManager = new OverlayManager();
    return *gtOverlayManager;
}

void OverlayManager::draw(DrawTarget *dt) {
	for (int i=0; i<m_array.size(); ++i) {
		OverlayElement &e = m_array[i];
		if (m_currentTime > e.m_startTime && m_currentTime < e.m_endTime) {
			float alpha = (m_currentTime - e.m_startTime) * 2.0f;
			if (alpha > 1.0f) {
				alpha = (e.m_endTime - m_currentTime) * 2.0f;
				if (alpha > 1.0f) alpha = 1.0f;
			}
			e.draw(dt, alpha);
		}
		for (int i=m_array.size(); i-->0;) {
			if (m_currentTime > m_array[i].m_endTime)
				m_array.removeAt(i);
		}
	}
}

float OverlayManager::getCurrentAlpha() {
	float currentAlpha = 0.0f;
	for (int i=0; i<m_array.size(); ++i) {
		OverlayElement &e = m_array[i];
		if (m_currentTime > e.m_startTime && m_currentTime < e.m_endTime) {
			float alpha = (m_currentTime - e.m_startTime) * 2.0f;
			if (alpha > 1.0f) {
				alpha = (e.m_endTime - m_currentTime) * 2.0f;
				if (alpha > 1.0f) alpha = 1.0f;
			}
			if (alpha > currentAlpha) currentAlpha = alpha;
		}
	}
	currentAlpha *= 4.0f;
	if (currentAlpha > 1.0f) currentAlpha = 1.0f;
	return currentAlpha;
}
	
void OverlayImage::draw(gametools::DrawTarget *dt, float alpha) {
	IosSurfaceRef image = gtCommander->getSurface(IMAGE_RGBA, m_image.c_str());
	dt->nextDrawAlpha(alpha);
	dt->draw(image.get(), NULL, &m_rect);
}

void OverlayText::draw(gametools::DrawTarget *dt, float alpha) {
	dt->nextDrawAlpha(alpha);
	dt->putStringCenteredXY(m_font.get(), m_x, m_y, m_text.c_str());
}

}
