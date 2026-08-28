/*
 *  OverlayManager.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 3/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef OVERLAY_MANAGER_H
#define OVERLAY_MANAGER_H

#include <string>
#include "GameTools.h"

namespace gametools {
	
class OverlayManager;

class OverlayElement {
public:
	OverlayElement() : m_startTime(0), m_endTime(0), m_type('e') {}
	virtual ~OverlayElement() {}
	virtual void draw(gametools::DrawTarget *dt, float alpha) = 0;
protected:
	friend class OverlayManager;
	double m_startTime;
	double m_endTime;
	char   m_type;
	void setTiming(double start, double end) { m_startTime = start; m_endTime = end; }
};
	
class OverlayImage : public OverlayElement {
	std::string m_image;
	IosRect m_rect;
public:
	OverlayImage(const std::string &imagePath, const IosRect &rect)
	: m_image(imagePath), m_rect(rect) { m_type = 'i'; }
	virtual void draw(gametools::DrawTarget *dt, float alpha);
};

class OverlayText : public OverlayElement {
	IosFontRef m_font;
	int m_x, m_y;
	std::string m_text;
public:
	OverlayText(IosFontRef font, int x, int y, const std::string &text)
	: m_font(font), m_x(x), m_y(y), m_text(text) { m_type = 't'; }
	virtual void draw(gametools::DrawTarget *dt, float alpha);
};

class OverlayManager : IdleComponent {
public:
	OverlayManager() : m_currentTime(0) {
		GameUIDefaults::GAME_LOOP->addIdle(this);
	}
	
	// The manager takes charge deallocating the element when done with it.
	void add(OverlayElement *e, float duration) {
        if (e->m_type == 'i') {
            for (int i=0; i<m_array.size(); ++i) {
                // Accelerate already showing elements.
                float remaining = m_array[i].m_endTime - m_currentTime;
                if (remaining > 0.0f) {
                    remaining *= 0.5f;
                    m_array[i].m_endTime = m_currentTime + remaining;
                }
            }
        }
		e->setTiming(m_currentTime, m_currentTime+duration);
		m_array.add(e);
	}
	void addImage(const std::string &imagePath, const IosRect &rect, float duration) {
		add(new OverlayImage(imagePath, rect), duration);
	}
	void addText(IosFontRef font, int x, int y, const std::string &text, float duration) {
		add(new OverlayText(font, x, y, text), duration);
	}
	
	void draw(gametools::DrawTarget *dt);
	void setOutputRect(float x, float y, float w, float h) {
		m_x = x;
		m_y = y;
		m_w = w;
		m_h = h;
	}
	float getX() const { return m_x; }
	float getY() const { return m_y; }
	float getWidth() const { return m_w; }
	float getHeight() const { return m_h; }
	float getCurrentAlpha();

	// Idle Componenent
	virtual void idle(double currentTime) {	m_currentTime = currentTime; }
	
private:
	//std::string m_overlayImage;
	//double m_overlayEndTime;
	//double m_overlayStartTime;
	ios_fc::SelfVector<OverlayElement> m_array;
	double m_currentTime;
	float m_x,m_y,m_w,m_h;
};

OverlayManager &GTGetOverlayManager();
	
}
#endif
