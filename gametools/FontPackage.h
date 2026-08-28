/*
 *  FontPackage.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 11/19/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


#ifndef FontPackage_H
#define FontPackage_H

#include <string>
#include "drawcontext.h"

namespace gametools {
	
	class FontPackage
	{
	public:
		FontPackage() :
			m_fontDefault("gfx/font.ttf"),m_fontSmall("gfx/font.ttf"),m_fontFunny("gfx/font.ttf"),
			m_fontDefaultSize(24), m_fontSmallSize(17), m_fontActiveFX(Font_STD), m_fontInactiveFX(Font_GREY), m_fontTextFX(Font_STORY)
		{}
		
		void setDefaultFont(const std::string &fontDefault) { m_fontDefault = fontDefault; }
		void setSmallFont(const std::string &fontSmall) { m_fontSmall = fontSmall; }
		void setFunnyFont(const std::string &fontFunny) { m_fontFunny = fontFunny; }
		void setDefaultSize(int fontDefaultSize) { m_fontDefaultSize = fontDefaultSize; }
		void setSmallSize(int fontSmallSize) { m_fontSmallSize = fontSmallSize; }
		void setFontActiveFX(IosFontFx fontFX) { m_fontActiveFX = fontFX; }
		void setFontInactiveFX(IosFontFx fontFX) { m_fontInactiveFX = fontFX; }
		void setFontTextFX(IosFontFx fontFX) { m_fontTextFX = fontFX; }
		
		const std::string &getDefaultFont() const { return m_fontDefault; }
		const std::string &getSmallFont() const { return m_fontSmall; }
		const std::string &getFunnyFont() const { return m_fontFunny; }
		int getDefaultSize() const { return m_fontDefaultSize; }
		int getSmallSize() const { return m_fontSmallSize; }
		IosFontFx getFontActiveFX() const { return m_fontActiveFX; }
		IosFontFx getFontInactiveFX() const { return m_fontInactiveFX; }
		IosFontFx getFontTextFX() const { return m_fontTextFX; }
		
	private:
		std::string m_fontDefault;
		std::string m_fontSmall;
		std::string m_fontFunny;
		int m_fontDefaultSize;
		int m_fontSmallSize;

		IosFontFx m_fontActiveFX;
		IosFontFx m_fontInactiveFX;
		IosFontFx m_fontTextFX;
	};
	
}
#endif