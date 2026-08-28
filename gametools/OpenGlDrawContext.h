//
//  OpenGlDrawContext.h
//
//  Copyright 2009 Fovea. All rights reserved.
//

#ifndef _OPENGLDRAWCONTEXT_H
#define _OPENGLDRAWCONTEXT_H

#include "GTOpenGl.h"
#include "drawcontext.h"
#include "DataPathManager.h"
#include "ios_mutex.h"
#include <memory>

namespace gametools {

class OpenGlDrawContext;

class OpenGlContext {
    public:
        virtual ~OpenGlContext() {}
        virtual void setCurrent() = 0;
        virtual void presentRenderbuffer(GLenum target) = 0; // target in GL_RENDERBUFFER_OES
        virtual void renderbufferStorage(GLenum target, void *data) = 0;
};

class OpenGlImageLibrary : public ImageLibrary
{
public:
    virtual IosSurface * createImage(ImageType type, int w, int h, ImageSpecialAbility abilities);
    virtual IosSurface * loadImage(ImageType type, const char *path, ImageSpecialAbility abilities);
    virtual IosFont    * createFont(const char *path, float size, const IosFontFx &fx = Font_STD);
    OpenGlImageLibrary(DataPathManager &dataPathManager, OpenGlDrawContext &dc)
        : m_dataPathManager(dataPathManager), m_oglDrawContext(dc) {}
	virtual ~OpenGlImageLibrary() {}
private:
    OpenGlDrawContext &m_oglDrawContext;
    DataPathManager &m_dataPathManager;
};

class OpenGlDrawContext : public DrawContext
{
public:
    OpenGlDrawContext(DataPathManager &dataPathManager, int width, int height, OpenGlContext *context);
    virtual void init();
public:
    virtual void clear(float r, float g, float b, float a);
    virtual void flip();
    virtual int getHeight() const;
    virtual int getWidth() const;
    bool        isRotated() const { return m_isRotated; }
    virtual ImageLibrary & getImageLibrary();
    bool resize(int w, int h, void *layer);

    void freeGlObjects();
    void unrefGlObjects();

public:
    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect);
    virtual void setClipRect(IosRect *rect);
    virtual void setBlendMode(ImageBlendMode mode) {}
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t) {}
    virtual void setMipmapEnable(bool state) { }
    virtual void fillRect(const IosRect *rect, const RGBA &color);
    virtual void putString(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void putStringWithShadow(IosFont *font, int x, int y, int shadow_x, int shadow_y, const char *text, float dx, float dy);
	virtual ImageSpecialAbility guessRequiredImageAbility(const ImageOperationList &list) { return IMAGE_NO_ABILITY; } // TODO
    void putStringCenteredXY(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void nextDrawAlpha(float alpha) {  m_alpha = alpha; }
	virtual void nextDrawColor(float r, float g, float b) {
        m_red   = r > 0.0f ? (r < 1.0f ? r : 1.0f) : 0.0f;
        m_green = g > 0.0f ? (g < 1.0f ? g : 1.0f) : 0.0f;
        m_blue  = b > 0.0f ? (b < 1.0f ? b : 1.0f) : 0.0f;
    }
    /*virtual void nextDrawColorAdd(float r, float g, float b) {
        m_redAdd   = r > 0 ? r : 0;
        m_greenAdd = g > 0 ? g : 0;
        m_blueAdd  = b > 0 ? b : 0;
    }*/
    
    // Flip screen vertically.
    void setVFlip(bool vflip);
protected:
    GLuint getColorRenderBuffer()  { return colorRenderbuffer; }
    GLuint getDefaultFrameBuffer() { return defaultFramebuffer; }
    std::auto_ptr<ios_fc::Lock> scopedGlLock();
    
public:
    // Performance measurement
    void startFrame();
    void endFrame();
	void bindFBO();
private:
    OpenGlImageLibrary iimLib;
    // EAGLContext *context;
    OpenGlContext *context;
    // The OpenGL names for the framebuffer and renderbuffer used to render to this view
	GLuint defaultFramebuffer, colorRenderbuffer;
	GLfloat matrix[16];
	float m_alpha;
	float m_red, m_green, m_blue;
    bool  m_isRotated;
	// float m_redAdd, m_greenAdd, m_blueAdd;
    // BlendingMode m_blending;

	void applyAlpha();
	void resetAlpha();
};

}

#endif
