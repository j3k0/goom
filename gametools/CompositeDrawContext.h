#ifndef _COMPOSITEDRAWCONTEXT_H_
#define _COMPOSITEDRAWCONTEXT_H_

#include <string>
#include <map>
#include "drawcontext.h"
#include "ios_ptr.h"
#include "GTLog.h"

namespace gametools {

class CompositeDrawContext;
class CompositeImageLibrary;

#ifdef DISABLED
template <typename T>
class SharedReference
{
public:
    SharedReference(T *p)
        : m_p(p), m_refCount(1) {}
    T *m_p;
    volatile int m_refCount;
};

template <typename T>
class SharedPtr
{
public:
    SharedPtr()
        : m_ref(NULL) {}
    SharedPtr(T *p)
        : m_ref(p == NULL?NULL:new SharedReference<T>(p)) {}
    SharedPtr(const SharedPtr &s)
        : m_ref(s.m_ref) {
        if (m_ref != NULL)
            ++(m_ref->m_refCount);
    }
    ~SharedPtr() {
        if ((m_ref != NULL)
            && (--(m_ref->m_refCount) == 0)) {
            delete m_ref->m_p;
            delete m_ref;
        }
    }
    void reset(T *p) {
        if ((m_ref != NULL)
            && (--(m_ref->m_refCount) == 0)) {
            delete m_ref->m_p;
            delete m_ref;
        }
        m_ref = (p == NULL?NULL:new SharedReference<T>(p));
    }
    /*operator T *() const {
        if (m_ref == NULL)
            return NULL;
        return m_ref->m_p;
    }*/
    T * operator ->()  const {
        //if (m_ref == NULL)
        //    return NULL;
        return m_ref->m_p;
    }
    T *get() const {
        if (m_ref == NULL)
            return NULL;
        return m_ref->m_p;
    }
    bool empty() const { return m_ref == NULL; }
private:
    SharedReference<T> *m_ref;
};
#endif


class CompositeSurfaceDefinition
{
public:
    CompositeSurfaceDefinition() {}
    CompositeSurfaceDefinition(const char *path, const IosRect &cropRect)
        : m_path(path), m_cropRect(cropRect) {}
    const char *getPath() const { return m_path.data(); }
    const IosRect &getCropRect() const { return m_cropRect; }
private:
    std::string m_path;
    IosRect m_cropRect;
};

class CompositeSurface : public IosSurface
{
public:
    CompositeSurface(CompositeImageLibrary &ownerImageLibrary,
                     ios_fc::SharedPtr<IosSurface> baseSurface);
    CompositeSurface(CompositeImageLibrary &ownerImageLibrary,
                     ios_fc::SharedPtr<IosSurface> baseSurface, const IosRect &cropRect);
    virtual ~CompositeSurface();
    // IosSurface methods
    virtual void clear(float r, float g, float b, float a);
    virtual void flip();
    virtual bool isOpaque() const;
	virtual bool haveAbility(int ability) const;
	virtual void dropAbility(int ability);
    virtual RGBA readRGBA(int x, int y);
    virtual IosSurface *shiftHue(float hue_offset, IosSurface *mask = NULL);
    virtual IosSurface *shiftHSV(float h, float s, float v);
    virtual IosSurface *setValue(float value);
    virtual IosSurface *resizeAlpha(int width, int height);
    virtual IosSurface *mirrorH();
    virtual void        convertToGray();
    // DrawTarget methods
    virtual void setClipRect(IosRect *rect)
    {
        m_baseSurface->setClipRect(rect);
    }
    virtual void setBlendMode(ImageBlendMode mode)
    {
        if (!m_isCropped) {
            m_baseSurface->setBlendMode(mode);
            return;
        }
        m_blendMode = mode;
    }
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t)
    {
        if (!m_isCropped) {
            m_baseSurface->setWrapMode(mode_s, mode_t);
            return;
        }
        else if (mode_s == IMAGE_REPEAT || mode_t == IMAGE_REPEAT) {
            // WARNING, make sure composite surfaces share a commun wrap mode.
            gametools::GTLogf("WARNING: WrapMode enabled on an composite image (%s)\n", name.c_str());
            m_baseSurface->setWrapMode(mode_s, mode_t);
            return;
            // throw "Unsupported wrap mode for a composite surface.";
        }
    }
    
    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect)
    {
        CompositeSurface *s = static_cast<CompositeSurface *>(surf);
        if (!s->m_isCropped) {
            m_baseSurface->draw(s->m_baseSurface.get(), srcRect, dstRect);
            return;
        }
        s->m_baseSurface->setBlendMode(m_blendMode);
        if (srcRect == NULL) {
            m_baseSurface->draw(s->m_baseSurface.get(), &(s->m_cropRect), dstRect);
        }
        else {
            IosRect rect = *srcRect;
            rect.x += s->m_cropRect.x;
            rect.y += s->m_cropRect.y;
            m_baseSurface->draw(s->m_baseSurface.get(), &rect, dstRect);
        }
    }
	virtual void fillRect(const IosRect *rect, const RGBA &color);
    // Replace a portion of the surface with data
    virtual void setData(int x, int y, int w, int h, const char *data);
    virtual void nextDrawAlpha(float alpha)  { m_baseSurface->nextDrawAlpha(alpha); }
	virtual void nextDrawColor(float r, float g, float b) { m_baseSurface->nextDrawColor(r,g,b); }

    virtual void setMipmapEnable(bool state) {
        if (!m_isCropped)
            m_baseSurface->setMipmapEnable(state);
        else
            return;
    }

    virtual void putString(IosFont *font, int x, int y, const char *text, float dx = 1, float dy = 0);
    virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx = 1, float dy = 0);

private:
    CompositeImageLibrary &m_ownerImageLibrary;
    ios_fc::SharedPtr<IosSurface> m_baseSurface;
    bool m_isCropped;
    IosRect m_cropRect;
    ImageBlendMode m_blendMode;
    friend class CompositeDrawContext;
};

class CompositeImageLibrary : public ImageLibrary
{
public:
    CompositeImageLibrary(CompositeDrawContext &owner);
    virtual IosSurface * createImage(ImageType type, int w, int h, ImageSpecialAbility specialAbility = 0);
    virtual IosSurface * loadImage(ImageType type, const char *path, ImageSpecialAbility specialAbility = 0);
    virtual IosFont    * createFont(const char *path, float size, const IosFontFx &fx = Font_STD);
public:
    DrawContext &getBaseDrawContext() const;
private:
    CompositeDrawContext &m_owner;
    DrawContext  &m_baseDrawContext;
    ImageLibrary &m_baseImageLibrary;
    typedef std::map<std::string, ios_fc::SharedPtr<IosSurface> > BaseSurfaceMap;
    BaseSurfaceMap m_baseSurfaceMap;
};

class CompositeDrawContext : public DrawContext
{
public:
    CompositeDrawContext(DrawContext *baseDrawContext);
    virtual void clear(float r, float g, float b, float a) {
        m_baseDrawContext->clear(r,g,b,a);
    }
    virtual void flip() {
        m_baseDrawContext->flip();
    }
    virtual int getHeight() const {
        return m_baseDrawContext->getHeight();
    }
    virtual int getWidth() const {
        return m_baseDrawContext->getWidth();
    }
    virtual ImageLibrary &getImageLibrary() {
        return m_imageLibrary;
    }
    // Query for drawcontext abilities
    virtual bool hasScaleAbility() const;
    virtual ImageSpecialAbility guessRequiredImageAbility(const ImageOperationList &list);
    // DrawTarget implementation
    virtual void setClipRect(IosRect *rect);
    virtual void setBlendMode(ImageBlendMode mode);
    virtual void setMipmapEnable(bool state);
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t);
    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect);
    
	virtual void fillRect(const IosRect *rect, const RGBA &color);
    virtual void putString(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void nextDrawAlpha(float alpha) { m_baseDrawContext->nextDrawAlpha(alpha); }
	virtual void nextDrawColor(float r, float g, float b) {  m_baseDrawContext->nextDrawColor(r,g,b); }
    // virtual void nextDrawColorAdd(float r, float g, float b) { m_baseDrawContext->nextDrawColorAdd(r,g,b); }

    // Specific methods
    DrawContext &getBaseDrawContext() const { return *m_baseDrawContext; }
    void declareCompositeSurface(const char *key,
                                 const char *path,
                                 IosRect &cropRect);
    void declareCompositeSurface(const char *key,
                                 const char *path,
                                 int x, int y, int w, int h);
    CompositeSurfaceDefinition *getCompositeSurfaceDefinition(const char *key);
private:
    DrawContext *m_baseDrawContext;
    CompositeImageLibrary m_imageLibrary;
    std::map<std::string, CompositeSurfaceDefinition> m_compositeSurfaceDefs;
};
    
}

#endif // _COMPOSITEDRAWCONTEXT_H_

