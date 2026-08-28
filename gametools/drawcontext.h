#ifndef _DRAWCONTEXT_H_
#define _DRAWCONTEXT_H_

#include "ios_fc.h"
#include "rgba.h"
#include "vec3.h"
#include <stdlib.h>
#include <string>

namespace gametools {

enum ImageType {
	IMAGE_RGBA = 1,
	IMAGE_RGB = 2
};

enum ImageBlendMode {
    IMAGE_COPY,
    IMAGE_BLEND,
    IMAGE_ADD,
    IMAGE_MULTIPLY
};

enum ImageWrapMode {
    IMAGE_CLAMP,
    IMAGE_REPEAT
};

// Image Special Powers (Abilities)
typedef int ImageSpecialAbility;
const ImageSpecialAbility IMAGE_NO_ABILITY = 0;
const ImageSpecialAbility IMAGE_READ = 1;                           // Reading pixels will be required, keep texture in RAM.
const ImageSpecialAbility IMAGE_COMPRESSION_DISABLE_ALL = 2;        // Prevent any compression to be applied.
const ImageSpecialAbility IMAGE_COMPRESSION_DISABLE_ALPHA_1BIT = 4; // Prevent alpha layer to be made 1 bit.

struct IosRect
{
    IosRect() : x(0),y(0),w(0),h(0),frameX(1),frameY(0) {}
    IosRect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h), frameX(1),frameY(0) {}
    IosRect(const Vec2f &xy, const Vec2f &wh) : x(xy.x), y(xy.y), w(wh.x), h(wh.y), frameX(1),frameY(0) {}
    IosRect(const IosRect &r) : x(r.x), y(r.y), w(r.w), h(r.h), frameX(r.frameX),frameY(r.frameY) {}
    IosRect(const AABBf &aabb)
    : x(aabb.lowerBound.x)
    , y(aabb.lowerBound.y)
    , w(aabb.upperBound.x - aabb.lowerBound.x)
    , h(aabb.upperBound.y - aabb.lowerBound.y)
    , frameX(1), frameY(0) {}
    
    float x, y;
    float w, h;
	float frameX, frameY;
    inline bool hasIntersection(IosRect &rect) const;
    inline bool equals(const IosRect &rect) const;
    inline bool isInside(float wx, float wy) const {
		float lw = (w >= 0.0f ? w : -w);
		float lh = (h >= 0.0f ? h : -h);
        return (wx >= x) && (wx <= x + lw) && (wy >= y) && (wy <= y + lh);
    }
    
    Vec2f center() const { return Vec2f(x+w*0.5f, y+h*0.5f); }
    Vec2f size() const { return Vec2f(w, h); }

    IosRect scaled(float coef) const {
        IosRect ret(x + w * 0.5f * (1.0f - coef),
                    y + h * 0.5f * (1.0f - coef),
                    w * coef, h * coef);
        ret.frameX = frameX;
        ret.frameY = frameY;
        return ret;
    }

    IosRect top(float coef) const;
    IosRect bottom(float coef) const;
    IosRect left(float coef) const;
    IosRect right(float coef) const;
    IosRect subCell(float nCols, float nRows, float col, float row) const;
};

bool IosRect::hasIntersection(IosRect &rect) const
{
    int Amin, Amax, Bmin, Bmax;
    /* Horizontal intersection */
    Amin = x;
    Amax = Amin + w;
    Bmin = rect.x;
    Bmax = Bmin + rect.w;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return false;
    /* Vertical intersection */
    Amin = y;
    Amax = Amin + h;
    Bmin = rect.y;
    Bmax = Bmin + rect.h;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return false;
    return true;
}

bool IosRect::equals(const IosRect &rect) const
{
    return ((h == rect.h) && (w == rect.w)
            && (x == rect.x) && (y == rect.y));
}

struct IosRectC
{
    IosRectC(int x, int y, int w, int h)
    {
        r.x = x; r.y = y; r.w = w; r.h = h;
    }
    IosRect r;
};

class IosSurface;
class IosFont;

enum BlendingMode {
    BLEND_NORMAL,
    BLEND_ADD
};

class DrawTarget
{
public:
    virtual ~DrawTarget() {}
    
    virtual void clear(float r, float g, float b, float a) = 0;
    virtual void flip()  = 0;

    virtual void setClipRect(IosRect *rect) = 0;
    virtual void setBlendMode(ImageBlendMode mode) = 0;
    virtual void setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t) = 0;
    virtual void setMipmapEnable(bool state) = 0;
    
    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect) = 0;

	virtual void fillRect(const IosRect *rect, const RGBA &color) = 0;

    virtual void putString(IosFont *font, int x, int y, const char *text, float dx = 1, float dy = 0) = 0;
    virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx = 1, float dy = 0) = 0;
	virtual void putStringWithShadow(IosFont *font, int x, int y, int shadow_x, int shadow_y, const char *text, float dx = 1, float dy = 0) { putString(font, x,y,text,dx,dy); } // Default DrawTarget have shadow disabled.
    void putStringCenteredXY(IosFont *font, int x, int y, const char *text, float dx = 1, float dy = 0, char align = 'l');
	void putString(IosFont *font,  int left, int right, int y, const char *text, const char *direction, float dx = 1.0f, float dy = 0.0f);
	void putStringCenteredLeft(IosFont *font,  int left, int right, int y, const char *text, const char *direction, float dx = 1.0f, float dy = 0.0f);
	void putStringCenteredRight(IosFont *font, int left, int right, int y, const char *text, const char *direction, float dx = 1.0f, float dy = 0.0f);
	virtual int getHeight() const { return h; }
    virtual int getWidth() const { return w; }

	// Change alpha value for next draw
	virtual void nextDrawAlpha(float alpha) = 0;
	virtual void nextDrawColor(float r, float g, float b) = 0;
    // virtual void nextDrawColorAdd(float r, float g, float b) {}
    
public:
	std::string name;
    int h, w;
};

class BaseSurface {
public:
    virtual ~BaseSurface() {}
    virtual void draw(DrawTarget *dt, IosRect *rect, float alpha) = 0;
    virtual Vec2f size() const { return Vec2f(1,1); }
};

#define IOS_ALPHA_TRANSPARENT 0
#define IOS_ALPHA_OPAQUE      255
class IosSurface : public DrawTarget, public BaseSurface
{
public:
    IosSurface() : m_enableExceptionOnDeletion(false) {}
    ~IosSurface() {
        if (m_enableExceptionOnDeletion)
            throw ios_fc::Exception("IosSurface forbidden deletion");
    }
public:
    // To be implemented by child class.
    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect) = 0;

	virtual bool isOpaque() const = 0;

	virtual bool haveAbility(int ability) const = 0;
	virtual void dropAbility(int ability) = 0;
    virtual RGBA readRGBA(int x, int y) = 0;

    virtual IosSurface *shiftHue(float hue_offset, IosSurface *mask = NULL) = 0;
    virtual IosSurface *shiftHSV(float h, float s, float v) = 0;
    virtual IosSurface *setValue(float value) = 0;

    virtual IosSurface * resizeAlpha(int width, int height) = 0;
    virtual IosSurface * mirrorH() = 0;
    virtual void         convertToGray() = 0;
    
    // Replace a portion of the surface with data
    virtual void setData(int x, int y, int w, int h, const char *data) {}
    
    // Implementation of the BaseSurface interface
    virtual void draw(DrawTarget *dt, IosRect *rect, float alpha) {
        dt->nextDrawAlpha(alpha);
        dt->draw(this, NULL, rect);
    }
    virtual Vec2f size() const { return Vec2f(w,h); }
    
    // Deletion protection
    void enableExceptionOnDeletion(bool enable) {m_enableExceptionOnDeletion = enable;}
private:
    bool m_enableExceptionOnDeletion;
};

struct IosFontFx
{
    uint8_t red, green, blue;
    bool    shadow;
    
    int ID() const { return (int)red + (int)green * 256 + (int)blue * 65536 + (shadow ? 16777216 : 0); }
    
    IosFontFx(const IosFontFx &fx)
    : red(fx.red), green(fx.green), blue(fx.blue), shadow(fx.shadow) {}
    IosFontFx(uint8_t red, uint8_t green, uint8_t blue, bool shadow = true)
    : red(red), green(green), blue(blue), shadow(shadow) {}
};

extern IosFontFx Font_STD;
extern IosFontFx Font_STORY;
extern IosFontFx Font_GREY;
extern IosFontFx Font_DARK;
extern IosFontFx Font_WHITE;

class IosFont
{
public:
    virtual ~IosFont() {}
    virtual float getTextWidth(const char *text) = 0;
    virtual float getTextHeight(const char *text) = 0;
    virtual float getHeight() = 0;
    virtual float getLineSkip() = 0;
};

// Allows to manipulate a rendered text as if it's a surface.
class TextSurface : public BaseSurface {
    IosFont    *m_font;
    std::string m_text;
    Vec2f      m_size;
public:
    TextSurface(IosFont *font, const std::string &text) : m_font(font), m_text(text) {
        m_size.x = m_font->getTextWidth(m_text.c_str());
        m_size.y = m_font->getTextHeight(m_text.c_str());
    }
    virtual void draw(DrawTarget *dt, IosRect *rect, float alpha) {
        // TODO: No rotation supported... yet
        Vec2f c = rect->center();
        dt->nextDrawAlpha(alpha);
        dt->putStringCenteredXY(m_font, c.x, c.y, m_text.c_str(),rect->frameX,rect->frameY);
    }
    virtual Vec2f size() const { return m_size; }
};

// Image and fonts loaders
class ImageLibrary
{
public:
    virtual IosSurface * createImage(ImageType type, int w, int h, ImageSpecialAbility specialAbility = 0) = 0;
    virtual IosSurface * loadImage(ImageType type, const char *path, ImageSpecialAbility specialAbility = 0) = 0;
    virtual IosFont    * createFont(const char *path, float size, const IosFontFx &fx = Font_STD) = 0;
protected:
    virtual ~ImageLibrary() {}
};

/**
 * List of operations that can be performed on a surface.
 * Used for guessRequiredImageAbility()
 */
struct ImageOperationList
{
    ImageOperationList()
        : shiftHue(false), shiftHSV(false), setValue(false),
          resizeAlpha(false), mirrorH(false), convertToGray(false)
    {}
    bool shiftHue;
    bool shiftHSV;
    bool setValue;
    bool resizeAlpha;
    bool mirrorH;
    bool convertToGray;
};

class DrawContext : public DrawTarget
{
public:
    DrawContext() { name = "DrawContext"; }
    virtual ~DrawContext() {}
    virtual ImageLibrary &getImageLibrary() = 0;
    // Query for drawcontext abilities
    virtual bool hasScaleAbility() const { return false; }
    virtual ImageSpecialAbility guessRequiredImageAbility(const ImageOperationList &list) = 0;
};


// Implementation
inline void DrawTarget::putStringCenteredXY(IosFont *font, int x, int y, const char *text, float dx, float dy, char align) {
    if (font == NULL) return;
    float htw = font->getTextWidth(text)  * 0.5f;
    float hth = font->getTextHeight(text) * 0.5f;
    if (align == 'l')
        putString(font,
                  x - htw * dx - hth * dy,
                  y - hth * dx + htw * dy, text, dx, dy);
    else
        putStringRight(font,
                       x + htw * dx + hth * dy,
                       y - hth * dx + htw * dy, text, dx, dy);
}

/*inline void DrawTarget::putStringRight(IosFont *font, int x, int y, const char *text) {
	putString(font, x - font->getTextWidth(text), y, text);
}*/

inline void DrawTarget::putString(IosFont *font, int left, int right, int y,
                                              const char *text, const char *direction, float dx, float dy) {
    if (direction[0] == 'r')
        putStringRight(font, right, y, text, dx, dy);
    else
        putString(font, left, y, text, dx, dy);
}

inline void DrawTarget::putStringCenteredLeft(IosFont *font, int left, int right, int y,
								  const char *text, const char *direction, float dx, float dy) {
	if (direction[0] == 'r')
		putStringRight(font, right, y - font->getTextHeight(text) * 0.5f, text, dx, dy);
	else
		putString(font, left, y - font->getTextHeight(text) * 0.5f, text, dx, dy);
}

inline void DrawTarget::putStringCenteredRight(IosFont *font, int left, int right, int y,
                                              const char *text, const char *direction, float dx, float dy) {
    if (direction[0] != 'r')
        putStringRight(font, right, y - font->getTextHeight(text) * 0.5f, text, dx, dy);
    else
        putString(font, left, y - font->getTextHeight(text) * 0.5f, text, dx, dy);
}

}

#endif // _DRAWCONTEXT_H_

