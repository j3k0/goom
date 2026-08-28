#include "graphicsmanager.h"

namespace gametools {
void GTLog(const char *txt);

void LegacyDrawContext::setClipRect(IosRect *rect) {
	(void)rect;
	// m_graphicsManager.setClipRect(rect);
}

void LegacyDrawContext::setBlendMode(ImageBlendMode mode) {
	(void)mode;
	// deprecated.
	// m_graphicsManager.setBlendMode(mode);
}

void LegacyDrawContext::setWrapMode(ImageWrapMode mode_s, ImageWrapMode mode_t) {
    (void)mode_s;
    (void)mode_t;
}

void LegacyDrawContext::setMipmapEnable(bool state) {
    (void)state;
}

static void initVertices(float *vertices, const IosRect *pDstRect) {
	if (((pDstRect->frameY == 0) && (pDstRect->frameX == 1))
		|| (fabs(pDstRect->frameY) < 0.001f && pDstRect->frameX > 0.9f)) {
		float w = pDstRect->w;
		float h = pDstRect->h;
		// P1
		vertices[0] = pDstRect->x;
		vertices[1] = pDstRect->y;
		// P2
		vertices[2] = pDstRect->x + w;
		vertices[3] = pDstRect->y;
		// P3
		vertices[4] = pDstRect->x;
		vertices[5] = pDstRect->y + h;
		// P4
		vertices[6] = vertices[2];
		vertices[7] = vertices[5];
	}
	else {
		float w = pDstRect->w;
		float h = pDstRect->h;
		// u (w,0)
		float ux = w *  pDstRect->frameX /*+ 0 * pDstRect->frameY*/;
		float uy = w * -pDstRect->frameY /*+ 0 * pDstRect->frameX*/;
		// v (0,h)
		float vx = 0 *  pDstRect->frameX + h * pDstRect->frameY;
		float vy = 0 * -pDstRect->frameY + h * pDstRect->frameX;
		// Offset to put keep the center in place.
		float ox = 0.5f * (w - w*pDstRect->frameX - h*pDstRect->frameY);
		float oy = 0.5f * (h + w*pDstRect->frameY - h*pDstRect->frameX);
		float x = pDstRect->x + ox;
		float y = pDstRect->y + oy;
		// P1 (0,0)
		vertices[0] = x;
		vertices[1] = y;
		// P2 (w,0)
		vertices[2] = x + ux;
		vertices[3] = y + uy;
		// P3 (0,h)
		vertices[4] = x + vx;
		vertices[5] = y + vy;
		// P4 (w,h)
		vertices[6] = x + ux + vx;
		vertices[7] = y + uy + vy;
	}
}

static const IosRect *fixRects(const IosRect *rect, float w, float h)
{
	if (rect != NULL)
		return rect;
	else {
		static IosRect cRect;
		cRect.x = 0;
		cRect.y = 0;
		cRect.h = h;
		cRect.w = w;
		return &cRect;
	}
}


void LegacyDrawContext::draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect) {
	if (surf == NULL) return;
	graphics_manager::Quad2D quad;
	const IosRect *pSrcRect = fixRects(srcRect, surf->w, surf->h);
	const IosRect *pDstRect = fixRects(dstRect, w, h);
	initVertices(&quad.vertex[0], pDstRect);
	float p2w = surf->getWidth();
	float p2h = surf->getHeight();
	quad.texcoord[0] = (float)(pSrcRect->x) / p2w;             quad.texcoord[1] = (float)(pSrcRect->y+pSrcRect->h) / p2h;
	quad.texcoord[2] = (float)(pSrcRect->x+pSrcRect->w) / p2w; quad.texcoord[3] = (float)(pSrcRect->y+pSrcRect->h) / p2h;
	quad.texcoord[4] = (float)(pSrcRect->x) / p2w;             quad.texcoord[5] = (float)(pSrcRect->y) / p2h;
	quad.texcoord[6] = (float)(pSrcRect->x+pSrcRect->w) / p2w; quad.texcoord[7] = (float)(pSrcRect->y) / p2h;
	LegacySurface *lsurf = static_cast<LegacySurface*>(surf);
	m_graphicsManager.draw(lsurf->getImage(), quad, RGBAf(r,g,b,a), lsurf->getBlendMode(), lsurf->getWrapModeS(), lsurf->getWrapModeT());
	resetColor();
}

void LegacyDrawContext::fillRect(const IosRect *dstRect, const RGBA &color) {
	graphics_manager::Quad2D quad;
	const IosRect *pDstRect = fixRects(dstRect, w, h);
	initVertices(&quad.vertex[0], pDstRect);
	m_graphicsManager.fillQuad(quad, RGBAf(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, color.alpha / 255.0f));
	resetColor();
}

void LegacyDrawContext::putString(IosFont *font, int x, int y, const char *text, float dx, float dy) {
	LegacyFont* lfont = static_cast<LegacyFont*>(font);
    const IosFontFx &fx = lfont->getFx();
    r = fx.red / 255.f;
    g = fx.green / 255.f;
    b = fx.blue / 255.f;
	m_graphicsManager.putString(lfont->getFont(), lfont->getHeight(), RGBAf(r,g,b,a), x,y,
								graphics_manager::ALIGN_LEFT, graphics_manager::ALIGN_TOP, text, dx, dy);
	resetColor();
}

void LegacyDrawContext::putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy) {
	LegacyFont* lfont = static_cast<LegacyFont*>(font);
    const IosFontFx &fx = lfont->getFx();
    r = fx.red / 255.f;
    g = fx.green / 255.f;
    b = fx.blue / 255.f;
	m_graphicsManager.putString(lfont->getFont(), lfont->getHeight(), RGBAf(r,g,b,a), x,y,
								graphics_manager::ALIGN_RIGHT, graphics_manager::ALIGN_TOP, text, dx, dy);
	resetColor();
}

}
