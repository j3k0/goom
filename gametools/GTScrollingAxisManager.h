//
//  GTScrollingAxisManager.h
//  Project
//
//  Created by Jean-Christophe Hoelt on 11/14/11.
//  Copyright 2011 Fovea.cc. All rights reserved.
//

#ifndef GTScrollingAxisManager_H
#define GTScrollingAxisManager_H

#include <string>
#include "GTPreferences.h"

namespace gametools {

class ScrollingAxisManager {
#define kScrollActivationLimit 10.0f
    bool m_isScrolling;
    float m_storedOffset;
    float m_offset;
    float m_mouseDown;
    std::string m_name;
    
    float m_minOffset;
    float m_maxOffset;
    bool  m_mouseIsDown;
    
    float m_speedOffset;
    float m_speedScrollingTime;
    float m_speed;

public:
    ScrollingAxisManager(const char *name) : m_isScrolling(false), m_storedOffset(0.0f), m_offset(0.0f), m_name(name), m_minOffset(-999999999.9f), m_maxOffset(999999999.9f), m_mouseIsDown(false) {
        m_storedOffset = GetIntPreference((m_name + "ScrollingAxis").c_str(), 0);
        m_speed = 0.0f;
        m_speedScrollingTime = 0.0f;
        m_speedOffset = 0.0f;
    }
    
    void setLimits(float minOffset, float maxOffset) {
        m_minOffset = minOffset;
        m_maxOffset = maxOffset;
    }
    
    bool  isScrolling() const { return m_isScrolling; }
    float getOffset() const { return m_storedOffset + m_offset; }
    void  setOffset(float offset) { m_storedOffset = offset; m_offset = 0; }
    
    void idle(float dt) {
        m_speedScrollingTime += dt;

        if (m_storedOffset < m_minOffset) {
            float coef = dt * 2.f;
            m_storedOffset = m_storedOffset * (1.0f - coef) + m_minOffset * coef;
        }
        if (m_storedOffset > m_maxOffset) {
            float coef = dt * 2.f;
            m_storedOffset = m_storedOffset * (1.0f - coef) + m_maxOffset * coef;
        }

        if (m_isScrolling) {
            if (m_speedScrollingTime > 0.05f) {
                float speed = m_speedOffset / m_speedScrollingTime;
                m_speed = speed * 0.75f + m_speed * 0.25f;
                m_speedScrollingTime = 0.0f;
                m_speedOffset = 0.0f;
            }
        }
        else if (fabsf(m_speed) > 0.5f) {
            m_storedOffset += m_speed * dt;
            if (m_storedOffset < m_minOffset) {
                float coef = dt * 2.f;
                m_speed *= (1.0f - coef);
            }
            if (m_storedOffset > m_maxOffset) {
                float coef = dt * 2.f;
                m_speed *= (1.0f - coef);
            }
            if (m_speedScrollingTime > 0.1f) {
                m_speed *= 0.8f;
                m_speedScrollingTime -= 0.1f;
                if (fabsf(m_speed) < 2.0f) {
                    SetIntPreference((m_name + "ScrollingAxis").c_str(), m_storedOffset);
                    m_speed = 0.0f;
                }
            }
        }
    }
    
    void onMouseDown(float x) {
        m_mouseIsDown = true;
        m_mouseDown = x;
        m_offset = 0.0f;
        m_speed = 0.0f;
    }
    
    void onMouseMoved(float x) {
        if (!m_mouseIsDown) return;
        float d = x - m_mouseDown;
        if (!m_isScrolling) {
            if ((d > kScrollActivationLimit) || (d < -kScrollActivationLimit)) {
                m_isScrolling = true;
                m_speedScrollingTime = 0.0f;
                m_speedOffset = 0.0f;
                m_speed = 0.0f;
            }
        }
        if (m_isScrolling) {
            m_speedOffset += d - m_offset;
            m_offset = d;
            /*if (m_storedOffset + m_offset < m_minOffset)
                m_offset = m_minOffset - m_storedOffset;
            if (m_storedOffset + m_offset > m_maxOffset)
                m_offset = m_maxOffset - m_storedOffset;*/
        }
    }
    
    void onMouseUp(float x) {
        m_mouseIsDown = false;
        m_isScrolling = false;
        m_storedOffset += m_offset;
        m_offset = 0.0f;
        SetIntPreference((m_name + "ScrollingAxis").c_str(), m_storedOffset);
    }
};

}

#endif
