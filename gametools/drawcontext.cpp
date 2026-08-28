//
//  drawcontext.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 9/7/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#include "drawcontext.h"

namespace gametools {

    IosFontFx Font_STD(0xEF, 0xA2, 0x43);
    IosFontFx Font_STORY(0xF8, 0xF8, 0xF8);
    IosFontFx Font_GREY(0xCC, 0xCC, 0xCC);
    IosFontFx Font_DARK(0x43, 0x89, 0xEF);
    IosFontFx Font_WHITE(0xF8, 0xF8, 0xF8);

    IosRect IosRect::top(float coef) const {
        // Compute new size.
        IosRect ret;
        ret.w = this->w;
        ret.h = this->h * coef;

        // Compute the new center.
        float cOffset = (1.0f - coef) * 0.5f * this->h;
        Vec2f c = this->center();
        c.x -= -frameY * cOffset;
        c.y -=  frameX * cOffset;

        // Compute X,Y relative to the center.
        ret.x = c.x - ret.w * 0.5f;
        ret.y = c.y - ret.h * 0.5f;
        
        ret.frameX = frameX;
        ret.frameY = frameY;
        return ret;
    }

    IosRect IosRect::bottom(float coef) const {
        // Compute new size.
        IosRect ret;
        ret.w = this->w;
        ret.h = this->h * coef;

        // Compute the new center.
        float cOffset = (1.0f - coef) * 0.5f * this->h;
        Vec2f c = this->center();
        c.x += -frameY * cOffset;
        c.y +=  frameX * cOffset;

        // Compute X,Y relative to the center.
        ret.x = c.x - ret.w * 0.5f;
        ret.y = c.y - ret.h * 0.5f;
        
        ret.frameX = frameX;
        ret.frameY = frameY;
        return ret;
    }

    IosRect IosRect::left(float coef) const {
        // Compute new size.
        IosRect ret;
        ret.w = this->w * coef;
        ret.h = this->h;

        // Compute the new center.
        float cOffset = (1.0f - coef) * 0.5f * this->w;
        Vec2f c = this->center();
        c.x -= frameX * cOffset;
        c.y += frameY * cOffset;

        // Compute X,Y relative to the center.
        ret.x = c.x - ret.w * 0.5f;
        ret.y = c.y - ret.h * 0.5f;

        ret.frameX = frameX;
        ret.frameY = frameY;
        return ret;
    }

    IosRect IosRect::right(float coef) const {
        // Compute new size.
        IosRect ret;
        ret.w = this->w * coef;
        ret.h = this->h;

        // Compute the new center.
        float cOffset = (1.0f - coef) * 0.5f * this->w;
        Vec2f c = this->center();
        c.x += frameX * cOffset;
        c.y -= frameY * cOffset;

        // Compute X,Y relative to the center.
        ret.x = c.x - ret.w * 0.5f;
        ret.y = c.y - ret.h * 0.5f;

        ret.frameX = frameX;
        ret.frameY = frameY;
        return ret;
    }

    IosRect IosRect::subCell(float nCols, float nRows, float col, float row) const {
        // Top-Left Cell
        IosRect ret = this->left(1.0f / nCols).top(1.0f / nRows);
        // Translate it to the column.
        ret.x += col * frameX * ret.w;
        ret.y += col * frameY * ret.w;
        // Translate it to the row
        ret.x -= row * frameY * ret.h;
        ret.y += row * frameX * ret.h;
        
        return ret;
    }

}
