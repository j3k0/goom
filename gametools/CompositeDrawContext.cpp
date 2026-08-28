#include <iostream>
#include <memory>
#include "CompositeDrawContext.h"

using namespace std;
using namespace ios_fc;
using namespace gametools;

/**
 * CompositeSurface implementation
 */

CompositeSurface::CompositeSurface(CompositeImageLibrary &ownerImageLibrary,
                                   SharedPtr<IosSurface> baseSurface)
    : m_ownerImageLibrary(ownerImageLibrary),
      m_baseSurface(baseSurface), m_isCropped(false), m_blendMode(IMAGE_BLEND)
{
    w = baseSurface->w;
    h = baseSurface->h;
}

CompositeSurface::CompositeSurface(CompositeImageLibrary &ownerImageLibrary,
                                   SharedPtr<IosSurface> baseSurface,
                                   const IosRect &cropRect)
    : m_ownerImageLibrary(ownerImageLibrary),
      m_baseSurface(baseSurface), m_isCropped(true), m_cropRect(cropRect), m_blendMode(IMAGE_BLEND)
{
    w = cropRect.w;
    h = cropRect.h;
}

CompositeSurface::~CompositeSurface()
{
}

void CompositeSurface::flip()
{
    m_baseSurface->flip();
}

void CompositeSurface::clear(float r, float g, float b, float a)
{
    m_baseSurface->clear(r,g,b,a);
}

// IosSurface methods
bool CompositeSurface::isOpaque() const
{
    return m_baseSurface->isOpaque();
}

bool CompositeSurface::haveAbility(int ability) const
{
    return m_baseSurface->haveAbility(ability);
}

void CompositeSurface::dropAbility(int ability)
{
    if (!m_isCropped)
        m_baseSurface->dropAbility(ability);
}

RGBA CompositeSurface::readRGBA(int x, int y)
{
    if (!m_isCropped)
        return m_baseSurface->readRGBA(x, y);
    return m_baseSurface->readRGBA(x+m_cropRect.x, y+m_cropRect.y);
}

IosSurface *CompositeSurface::shiftHue(float hue_offset, IosSurface *mask)
{
    IosSurface *srcSurface = m_baseSurface.get();
    auto_ptr<IosSurface> tempSurface;
    if (m_isCropped) {
        DrawContext &baseDC = m_ownerImageLibrary.getBaseDrawContext();
        tempSurface.reset(baseDC.getImageLibrary().createImage(IMAGE_RGBA, m_cropRect.w, m_cropRect.h, IMAGE_READ));
        tempSurface->setBlendMode(IMAGE_COPY);
        tempSurface->draw(m_baseSurface.get(), &m_cropRect, NULL);
        srcSurface = tempSurface.get();
    }
    if (mask == NULL)
        return new CompositeSurface(m_ownerImageLibrary,
                                    srcSurface->shiftHue(hue_offset, NULL));
    CompositeSurface *m = static_cast<CompositeSurface *>(mask);
    return new CompositeSurface(m_ownerImageLibrary,
                                srcSurface->shiftHue(hue_offset, m->m_baseSurface.get()));
}

IosSurface *CompositeSurface::shiftHSV(float h, float s, float v)
{
    return new CompositeSurface(m_ownerImageLibrary,
                                m_baseSurface->shiftHSV(h, s, v));
}

IosSurface *CompositeSurface::setValue(float value)
{
    IosSurface *srcSurface = m_baseSurface.get();
    auto_ptr<IosSurface> tempSurface;
    if (m_isCropped) {
        DrawContext &baseDC = m_ownerImageLibrary.getBaseDrawContext();
        tempSurface.reset(baseDC.getImageLibrary().createImage(IMAGE_RGBA, m_cropRect.w, m_cropRect.h, IMAGE_READ));
        tempSurface->setBlendMode(IMAGE_COPY);
        tempSurface->draw(m_baseSurface.get(), &m_cropRect, NULL);
        srcSurface = tempSurface.get();
    }
    return new CompositeSurface(m_ownerImageLibrary,
                                srcSurface->setValue(value));
}

IosSurface * CompositeSurface::resizeAlpha(int width, int height)
{
    // If the DC doesnt'have the ability to scale graphics when drawing,
    // create a new surface with the rescaled graphics
    if (! m_ownerImageLibrary.getBaseDrawContext().hasScaleAbility()) {
        IosSurface *srcSurface = m_baseSurface.get();
        auto_ptr<IosSurface> tempSurface;
        if (m_isCropped) {
            DrawContext &baseDC = m_ownerImageLibrary.getBaseDrawContext();
            tempSurface.reset(baseDC.getImageLibrary().createImage(IMAGE_RGBA, m_cropRect.w, m_cropRect.h, IMAGE_READ));
            tempSurface->setBlendMode(IMAGE_COPY);
            tempSurface->draw(m_baseSurface.get(), &m_cropRect, NULL);
            srcSurface = tempSurface.get();
        }
        return new CompositeSurface(m_ownerImageLibrary,
                                    srcSurface->resizeAlpha(width, height));
    }
    // Otherwise, just create a new CompositeSurface with the same croprect
    // and base surface, and change its dimensions
    CompositeSurface *result = new CompositeSurface(m_ownerImageLibrary, m_baseSurface, m_cropRect);
    result->w = width;
    result->h = height;
    return result;
}

IosSurface * CompositeSurface::mirrorH()
{
    return new CompositeSurface(m_ownerImageLibrary,
                                m_baseSurface->mirrorH());
}

void CompositeSurface::convertToGray()
{
    m_baseSurface->convertToGray();
}

// DrawTarget implementation

void CompositeSurface::fillRect(const IosRect *rect, const RGBA &color)
{
    m_baseSurface->fillRect(rect, color);
}

void CompositeSurface::putString(IosFont *font, int x, int y, const char *text, float dx, float dy)
{
    m_baseSurface->putString(font, x, y, text, dx, dy);
}

void CompositeSurface::putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy)
{
    m_baseSurface->putStringRight(font, x, y, text, dx, dy);
}

void CompositeSurface::setData(int x, int y, int w, int h, const char *data)
{
    m_baseSurface->setData(x, y, w, h, data);
}

/**
 * CompositeImageLibrary implementation
 */

CompositeImageLibrary::CompositeImageLibrary(CompositeDrawContext &owner)
    : m_owner(owner), m_baseDrawContext(owner.getBaseDrawContext()),
      m_baseImageLibrary(m_baseDrawContext.getImageLibrary())
{
}

DrawContext & CompositeImageLibrary::getBaseDrawContext() const
{
    return m_owner.getBaseDrawContext();
}

IosSurface * CompositeImageLibrary::createImage(ImageType type, int w, int h, ImageSpecialAbility specialAbility)
{
    return new CompositeSurface(*this, m_baseImageLibrary.createImage(type, w, h, specialAbility));
}

IosSurface * CompositeImageLibrary::loadImage(ImageType type, const char *path, ImageSpecialAbility specialAbility)
{
    CompositeSurfaceDefinition *def = m_owner.getCompositeSurfaceDefinition(path);
    if (def == NULL) {
        IosSurface *base = m_baseImageLibrary.loadImage(type, path, specialAbility);
        if (base == NULL) return NULL;
        return new CompositeSurface(*this, base);
    }
    SharedPtr<IosSurface> baseSurface;
    BaseSurfaceMap::iterator existingBaseSurface = m_baseSurfaceMap.find(def->getPath());
    if (existingBaseSurface == m_baseSurfaceMap.end()) {
        baseSurface = SharedPtr<IosSurface>(m_baseImageLibrary.loadImage(type, def->getPath(), specialAbility));
        m_baseSurfaceMap[def->getPath()] = baseSurface;
    }
    else {
        baseSurface = existingBaseSurface->second;
    }
    CompositeSurface *result = new CompositeSurface(*this, baseSurface, def->getCropRect());
    return result;
}

IosFont * CompositeImageLibrary::createFont(const char *path, float size, const IosFontFx &fx)
{
    return m_baseImageLibrary.createFont(path, size, fx);
}

/**
 * CompositeDrawContext implementation
 */

CompositeDrawContext::CompositeDrawContext(DrawContext *baseDrawContext)
    : m_baseDrawContext(baseDrawContext), m_imageLibrary(*this)
{
	name = "CompositeDrawContext";
    w = baseDrawContext->w;
    h = baseDrawContext->h;
}

bool CompositeDrawContext::hasScaleAbility() const
{
    return m_baseDrawContext->hasScaleAbility();
}

ImageSpecialAbility CompositeDrawContext::guessRequiredImageAbility(const ImageOperationList &list)
{
    return m_baseDrawContext->guessRequiredImageAbility(list);
}

// DrawTarget implementation

void CompositeDrawContext::setClipRect(IosRect *rect)
{
    m_baseDrawContext->setClipRect(rect);
}

void CompositeDrawContext::setBlendMode(ImageBlendMode mode)
{
    m_baseDrawContext->setBlendMode(mode);
}

void CompositeDrawContext::setMipmapEnable(bool state)
{
    m_baseDrawContext->setMipmapEnable(state);
}

void CompositeDrawContext::setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t)
{
    m_baseDrawContext->setWrapMode(mode_s, mode_t);
}

void CompositeDrawContext::draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect)
{
    CompositeSurface *s = static_cast<CompositeSurface *>(surf);
    if (! s->m_isCropped) {
        m_baseDrawContext->draw(s->m_baseSurface.get(), srcRect, dstRect);
        return;
    }
    if (srcRect == NULL) {
        s->m_baseSurface->setBlendMode(s->m_blendMode);
        m_baseDrawContext->draw(s->m_baseSurface.get(), &(s->m_cropRect), dstRect);
        return;
    }
    else {
        IosRect rect = *srcRect;
        rect.x += s->m_cropRect.x;
        rect.y += s->m_cropRect.y;
        s->m_baseSurface->setBlendMode(s->m_blendMode);
        m_baseDrawContext->draw(s->m_baseSurface.get(), &rect, dstRect);
    }
}

void CompositeDrawContext::fillRect(const IosRect *rect, const RGBA &color)
{
    m_baseDrawContext->fillRect(rect, color);
}

void CompositeDrawContext::putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy)
{
    m_baseDrawContext->putStringRight(font, x, y, text, dx, dy);
}

void CompositeDrawContext::putString(IosFont *font, int x, int y, const char *text, float dx, float dy)
{
    m_baseDrawContext->putString(font, x, y, text, dx, dy);
}

// Specific methods

void CompositeDrawContext::declareCompositeSurface(const char *key,
                                                   const char *path,
                                                   IosRect &cropRect)
{
    m_compositeSurfaceDefs[key] = CompositeSurfaceDefinition(path, cropRect);
}

void CompositeDrawContext::declareCompositeSurface(const char *key,
                                                   const char *path,
                                                   int x, int y, int w, int h)
{
    IosRect cropRect(x, y, w, h);
    declareCompositeSurface(key, path, cropRect);
}

CompositeSurfaceDefinition *CompositeDrawContext::getCompositeSurfaceDefinition(const char *key)
{
    std::map<std::string, CompositeSurfaceDefinition>::iterator iter;
    iter = m_compositeSurfaceDefs.find(key);
    if (iter == m_compositeSurfaceDefs.end())
        return NULL;
    return &(iter->second);
}

