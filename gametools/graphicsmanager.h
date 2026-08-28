#ifndef VECTORCONTEXT_H
#define VECTORCONTEXT_H

#include "drawcontext.h"
#include "null-managers/NullDrawContext.h"
#include "ios_ptr.h"
#include "vec3.h"
#include "rgba.h"
#include "GTScreenOrientation.h"

namespace gametools {
namespace graphics_manager {

enum TextHorizontalAlign {
	ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER
};

enum TextVerticalAlign {
	ALIGN_TOP, ALIGN_MIDDLE, ALIGN_BOTTOM
};

enum BlendMode {
    BLEND_NORMAL,
    BLEND_ADD,
    BLEND_SHADOW,
    BLEND_HIGHLIGHT
};

enum WrapMode {
    WRAP_CLAMP,
    WRAP_REPEAT
};
    
struct Quad2D  {
	// A - B
	// | / |
	// C - D
	//
	// vertex order.
	// A,B,C,D (TRIANGLE_STRIP style)
	float vertex[8];  // (x,y,z) x 4
	float texcoord[8]; // (u,v) x 4
};

struct Quad3D  {
	// A - B
	// | / |
	// C - D
	//
	// vertex order.
	// A,B,C,D (TRIANGLE_STRIP style)
	float vertex[12];  // (x,y,z) x 4
	float texcoord[8]; // (u,v) x 4
};

/*struct RGBA {
	float r,g,b,a;
};*/

class Image {
public:
	virtual ~Image() {}
	virtual float getTypicalWidth() const = 0;
	virtual float getTypicalHeight() const = 0;
};

class Font {
public:
	virtual ~Font() {}
	virtual float getTextWidth(const char *text, float fontSize) const = 0;
	virtual float getTextHeight(const char *text, float fontSize) const = 0;
	virtual float getLineHeight(int fontSize) const = 0;
};

class ImageLibrary
{
public:
	virtual ~ImageLibrary() {}
	virtual Image *loadImage(const char *path, ImageType type) = 0;
	virtual Font  *loadFont (const char *path) = 0;
};

class GraphicsManager
{
protected:
	int w,h;

public:
	GraphicsManager(int w, int h) {
		this->w = w;
		this->h = h;
	}
	virtual ~GraphicsManager() {}

	inline int getHeight() const { return h; }
	inline int getWidth() const  { return w; }

	virtual void draw(const Image &surf, const Quad2D &quad, const gametools::RGBAf &color,
                      BlendMode blend, WrapMode wrapS = WRAP_CLAMP, WrapMode wrapT = WRAP_CLAMP) = 0;
	// virtual void draw(const Image &surf, const Quad3D &quad) = 0;
	virtual void fillQuad(const Quad2D &quad, const RGBAf &color) = 0;
	virtual void putString(const Font &font, int pixelSize, const RGBAf &color, int x, int y, TextHorizontalAlign halign, TextVerticalAlign valign, const char *text, float dx = 1.0f, float dy = 0.0f) = 0;

	virtual ImageLibrary &getImageLibrary() = 0;
};

}

//
// Legacy DrawContext Implementation
//
// Stub to a VectorContext...
//

class LegacySurface : public IosSurface
{
	ios_fc::SharedPtr<graphics_manager::Image> m_surface;
	graphics_manager::BlendMode m_blendMode;
    graphics_manager::WrapMode  m_wrapModeS, m_wrapModeT;
public:
	LegacySurface(ios_fc::SharedPtr<graphics_manager::Image> surface, int w, int h) : m_surface(surface)  {
		this->w = w;
		this->h = h;
		this->m_blendMode = graphics_manager::BLEND_NORMAL;
        this->m_wrapModeS = graphics_manager::WRAP_CLAMP;
        this->m_wrapModeT = graphics_manager::WRAP_CLAMP;
	}

		  graphics_manager::Image &getImage()       { return *m_surface; }
	const graphics_manager::Image &getImage() const { return *m_surface; }

	virtual bool isOpaque() const { return false; }
	virtual bool haveAbility(int ability) const { return true; }
	virtual void dropAbility(int ability) {}
	virtual RGBA readRGBA(int x, int y) {
		RGBA rgba = {0, 0, 0, 0};
		return rgba;
	}
	virtual IosSurface *shiftHue(float hue_offset, IosSurface *mask = NULL) {
		(void)hue_offset;
		(void)mask;
		return new LegacySurface(m_surface, this->w, this->h);
	}
	virtual IosSurface *shiftHSV(float h, float s, float v) {
		(void)h;
		(void)s;
		(void)v;
		return new LegacySurface(m_surface, this->w, this->h);
	}
	virtual IosSurface *setValue(float value) {
		(void)value;
		return new LegacySurface(m_surface, this->w, this->h);
	}
	virtual IosSurface *setAlpha(float value) {
		(void)value;
		return new LegacySurface(m_surface, this->w, this->h);
	}
	virtual IosSurface * resizeAlpha(int width, int height) {
		return new LegacySurface(m_surface, width, height);
	}
	virtual IosSurface * mirrorH() {
		return new LegacySurface(m_surface, this->w, this->h);
	}
	virtual void convertToGray() {}

	// DrawTarget implementation / render to texture disabled.
	virtual void setClipRect(IosRect *rect) {}
	virtual void setBlendMode(ImageBlendMode mode) {
	    switch (mode) {
	    case IMAGE_COPY:  m_blendMode = graphics_manager::BLEND_NORMAL; break;
	    case IMAGE_BLEND: m_blendMode = graphics_manager::BLEND_NORMAL; break;
	    case IMAGE_ADD:   m_blendMode = graphics_manager::BLEND_ADD;    break;
	    }
	}
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t) {
	    switch (mode_s) {
            case IMAGE_CLAMP:  m_wrapModeS = graphics_manager::WRAP_CLAMP;  break;
            case IMAGE_REPEAT: m_wrapModeS = graphics_manager::WRAP_REPEAT; break;
	    }
	    switch (mode_t) {
            case IMAGE_CLAMP:  m_wrapModeT = graphics_manager::WRAP_CLAMP;  break;
            case IMAGE_REPEAT: m_wrapModeT = graphics_manager::WRAP_REPEAT; break;
	    }
    }
    virtual void setMipmapEnable(bool state) {}
	virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect) {}
	virtual void fillRect(const IosRect *rect, const RGBA &color) {}
	virtual void putString(IosFont *font, int x, int y, const char *text, float dx, float dy) {}
	virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy) {}

    virtual void clear(float r, float g, float b, float a) { /* TODO */ }
    virtual void flip()  { /* TODO */ }
	virtual void nextDrawAlpha(float alpha) { /* TODO */ }
	virtual void nextDrawColor(float r, float g, float b) { /* TODO */ }

	graphics_manager::BlendMode getBlendMode() const { return m_blendMode; }
	graphics_manager::WrapMode getWrapModeS() const { return m_wrapModeS; }
	graphics_manager::WrapMode getWrapModeT() const { return m_wrapModeT; }
};

class LegacyFont : public IosFont
{
	ios_fc::SharedPtr<graphics_manager::Font> m_font;
	float m_size;
	IosFontFx m_fx;
public:
	LegacyFont(ios_fc::SharedPtr<graphics_manager::Font> font, float size, const IosFontFx &fx) : m_font(font), m_size(size), m_fx(fx) {}

		  graphics_manager::Font &getFont()       { return *m_font; }
	const graphics_manager::Font &getFont() const { return *m_font; }

	virtual float getTextWidth(const char *text)  { return m_font->getTextWidth(text, m_size); }
	virtual float getTextHeight(const char *text) { return m_font->getTextHeight(text, m_size); }

	virtual float getHeight()   { return m_size; }
	virtual float getLineSkip() { return m_size; }

	const IosFontFx &getFx() const { return m_fx; }
};

class LegacyImageLibrary : public ImageLibrary
{
	graphics_manager::ImageLibrary &m_library;
public:
	LegacyImageLibrary(graphics_manager::ImageLibrary &library) : m_library(library) {}

	virtual IosSurface *createImage(ImageType type, int w, int h, ImageSpecialAbility specialAbility) {
		(void)type;
		(void)w;
		(void)h;
		(void)specialAbility;
		throw ios_fc::Exception("Unsupported feature: VectorLegacyImageLibrary::createImage()");
	}
	virtual IosSurface * loadImage(ImageType type, const char *path, ImageSpecialAbility specialAbility) {
		(void)specialAbility;
		graphics_manager::Image *vimage = m_library.loadImage(path, type);
		return new LegacySurface(vimage, vimage->getTypicalWidth(), vimage->getTypicalHeight());
	}
	virtual IosFont    * createFont(const char *path, float size, const IosFontFx &fx) {
		return new LegacyFont(m_library.loadFont(path), size, fx);
	}
};

class LegacyDrawContext : public DrawContext
{
	graphics_manager::GraphicsManager &m_graphicsManager;
	LegacyImageLibrary m_imageLibrary;
	float r,g,b,a;

public:
	LegacyDrawContext(graphics_manager::GraphicsManager &graphicsManager) :
		m_graphicsManager(graphicsManager), m_imageLibrary(graphicsManager.getImageLibrary())
	{
		this->w = graphicsManager.getWidth();
		this->h = graphicsManager.getHeight();
		resetColor();
	}
    virtual void clear(float red, float green, float blue, float alpha) {}
	virtual void flip() {}

	virtual ImageLibrary &getImageLibrary() {
		return m_imageLibrary;
	}

	// Query for drawcontext abilities
	virtual bool hasScaleAbility() const { return false; }
	virtual ImageSpecialAbility guessRequiredImageAbility(const ImageOperationList &list) {
		return IMAGE_NO_ABILITY;
	}

	// DrawTarget implementation
	virtual void setClipRect(IosRect *rect);
	virtual void setBlendMode(ImageBlendMode mode);
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t);
    virtual void setMipmapEnable(bool state);
	virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect);
	virtual void fillRect(const IosRect *rect, const RGBA &color);
	virtual void putString(IosFont *font, int x, int y, const char *text, float dx, float dy);
	virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy);

	// Change alpha value for next draw
	virtual void nextDrawAlpha(float alpha) {
		this->a = alpha;
	}
	virtual void nextDrawColor(float red, float green, float blue) {
		this->r = red;
		this->g = green;
		this->b = blue;
	}
	void resetColor() {
		r = g = b = a = 1.0f;
	}
};

}

#endif // VECTORCONTEXT_H
