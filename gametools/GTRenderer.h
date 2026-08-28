/*
 *  GTRenderer.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 2/28/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

/*
 *  DfSceneRenderer.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/11/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef GTSceneRenderer_H
#define GTSceneRenderer_H

#include "GameTools.h"

namespace gametools {

class GTRenderer {
	float m_outputX, m_outputY, m_outputW, m_outputH;
	float m_viewportX, m_viewportY, m_viewportW, m_viewportH;
	bool m_resourcesLoaded;
	
public:
	GTRenderer() : m_resourcesLoaded(false) {}
	virtual ~GTRenderer() { if (m_resourcesLoaded) unloadResources(); }
	
	void setOutputRect(float x, float y, float w, float h);
	void setViewport(float x, float y, float w, float h);
	
	float getOutputWidth() const { return m_outputW; }
	float getOutputHeight() const { return m_outputH; }
	float getViewportWidth() const { return m_viewportW; }
	float getViewportHeight() const { return m_viewportH; }
	
	virtual void loadResources()   { m_resourcesLoaded = true; }
	virtual void unloadResources() { m_resourcesLoaded = false; }
	
	void draw(gametools::DrawTarget *dt);
	
	inline float toWorldX(float x) const {
		return m_viewportX + m_viewportW * (x - m_outputX) / m_outputW;
	}
	inline float toWorldY(float y) const {
		return m_viewportY - m_viewportH * (y - m_outputY - m_outputH) / m_outputH;
	}
	
protected:
	inline float toOutputX(float x) const {
		return m_outputX + m_outputW * (x - m_viewportX) / m_viewportW;
	}
	inline float toOutputY(float y) const {
		return m_outputY + m_outputH * (1.0f - (y - m_viewportY) / m_viewportH);
	}
	inline float toOutputW(float w) const {
		return m_outputW * w / m_viewportW;
	}
	inline float toOutputH(float h) const {
		return m_outputH * h / m_viewportH;
	}
	inline gametools::IosRect toOutput(const gametools::IosRect &rIn) const {
		gametools::IosRect rOut;
		rOut.x = toOutputX(rIn.x);
		rOut.y = toOutputY(rIn.h+rIn.y);
		rOut.w = toOutputW(rIn.w);
		rOut.h = toOutputH(rIn.h);
		rOut.frameX = rIn.frameX;
		rOut.frameY = rIn.frameY;
		return rOut;
	}
    
    inline bool isVisible(Vec2f pos) const {
        return ((pos.x >= m_viewportX)
                && (pos.x <= m_viewportX + m_viewportW)
                && (pos.y >= m_viewportY)
                && (pos.y <= m_viewportY + m_viewportH));
    }
    
    float getOutputX() const { return m_outputX; }
    float getOutputY() const { return m_outputY; }
    float getOutputW() const { return m_outputW; }
    float getOutputH() const { return m_outputH; }
	float getViewportX() const { return m_viewportX; }
    float getViewportY() const { return m_viewportY; }
    float getViewportW() const { return m_viewportW; }
    float getViewportH() const { return m_viewportH; }
};

}

#endif