#include "SdlGraphics.h"

#include "SOIL.h"

#include <SDL.h>

namespace gametools {
namespace graphics_manager {

    // Fit the logical rect inside the drawable, centered (letterbox/pillarbox).
    static void computeBlitRect(int logicalW, int logicalH,
                                int drawW, int drawH,
                                int *x0, int *y0, int *x1, int *y1);

    class SdlOpenGlContext : public OpenGlContext {
        SDL_Window *m_window;
    public:
        SdlOpenGlContext() : m_window(NULL), m_srcFbo(0), m_logicalW(0), m_logicalH(0) {}

        virtual void setCurrent() {
            if (m_window && SDL_GL_GetCurrentContext())
                SDL_GL_MakeCurrent(m_window, SDL_GL_GetCurrentContext());
        }

        // The engine renders the whole frame into its own screen FBO at the
        // fixed logical size (see OpenGlDrawContext::init, gScreenFBO). At
        // present time we blit that FBO, letterboxed, onto the window's real
        // drawable (which may be larger — fullscreen — or a different aspect
        // ratio) and swap. No engine relayout is needed when the window size
        // changes: the blit rect is recomputed from the current drawable.
        virtual void presentRenderbuffer(GLenum target) {
            if (m_srcFbo != 0 && m_logicalW > 0 && m_logicalH > 0) {
                int drawW = 0, drawH = 0;
                SDL_GL_GetDrawableSize(m_window, &drawW, &drawH);
                if (drawW > 0 && drawH > 0) {
                    int bx0, by0, bx1, by1;
                    computeBlitRect(m_logicalW, m_logicalH, drawW, drawH,
                                    &bx0, &by0, &bx1, &by1);
                    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, m_srcFbo);
                    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
                    // Clear the window framebuffer so letterbox bars are black
                    // (not stale) when the aspect ratio differs from logical.
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glBlitFramebufferEXT(0, 0, m_logicalW, m_logicalH,
                                         bx0, by0, bx1, by1,
                                         GL_COLOR_BUFFER_BIT, GL_LINEAR);
                }
            }
            if (m_window)
                SDL_GL_SwapWindow(m_window);
            // Restore the engine FBO as the draw target AFTER the swap.
            // The blit above binds FBO 0 as the draw framebuffer directly,
            // bypassing the engine's iglBindFramebufferOES gCurrentFBO
            // tracking; without this, the next frame's bindFBO()
            // short-circuits and widgets draw into the window's back
            // buffer, which the blit then overwrites with the stale FBO.
            // It must happen after SDL_GL_SwapWindow: on macOS the flush
            // only presents the default framebuffer's back buffer when the
            // default framebuffer is the draw target at flush time.
            if (m_srcFbo != 0 && m_logicalW > 0 && m_logicalH > 0)
                glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, m_srcFbo);
        }

        virtual void renderbufferStorage(GLenum target, void *layer) {
        }

        static SdlOpenGlContext instance;
        void setWindow(SDL_Window *window) { m_window = window; }
        void setLogicalFramebuffer(GLuint fbo, int w, int h) {
            m_srcFbo = fbo; m_logicalW = w; m_logicalH = h;
        }

        // Inverse-map a window-space (SDL event) coordinate into the engine's
        // logical coordinate space, undoing the letterbox blit above. Identity
        // when the window is at the logical size.
        void windowToLogical(float winX, float winY, float *outX, float *outY) const;

        GLuint m_srcFbo;
        int m_logicalW, m_logicalH;
    };
    SdlOpenGlContext SdlOpenGlContext::instance;

    // Fit the logical rect inside the drawable, centered (letterbox/pillarbox).
    static void computeBlitRect(int logicalW, int logicalH,
                                int drawW, int drawH,
                                int *x0, int *y0, int *x1, int *y1)
    {
        float sx = (float)drawW / (float)logicalW;
        float sy = (float)drawH / (float)logicalH;
        float s = (sx < sy) ? sx : sy;
        int outW = (int)((float)logicalW * s + 0.5f);
        int outH = (int)((float)logicalH * s + 0.5f);
        *x0 = (drawW - outW) / 2;
        *y0 = (drawH - outH) / 2;
        *x1 = *x0 + outW;
        *y1 = *y0 + outH;
    }

    void SdlOpenGlContext::windowToLogical(float winX, float winY,
                                           float *outX, float *outY) const
    {
        int drawW = 0, drawH = 0, winW = 0, winH = 0;
        if (m_window) {
            SDL_GL_GetDrawableSize(m_window, &drawW, &drawH);
            SDL_GetWindowSize(m_window, &winW, &winH);
        }
        if (m_srcFbo == 0 || m_logicalW <= 0 || m_logicalH <= 0 ||
            drawW <= 0 || drawH <= 0 || winW <= 0 || winH <= 0) {
            *outX = winX;
            *outY = winY;
            return;
        }
        int bx0, by0, bx1, by1;
        computeBlitRect(m_logicalW, m_logicalH, drawW, drawH, &bx0, &by0, &bx1, &by1);
        // SDL mouse events are in window (points) space; the blit targets the
        // drawable in pixels. Scale into drawable space, then undo the letterbox.
        float rx = (float)drawW / (float)winW;
        float ry = (float)drawH / (float)winH;
        float dx = winX * rx;
        float dy = winY * ry;
        *outX = (dx - (float)bx0) * (float)m_logicalW / (float)(bx1 - bx0);
        *outY = (dy - (float)by0) * (float)m_logicalH / (float)(by1 - by0);
    }

    SdlOpenGlDrawContext::SdlOpenGlDrawContext(DataPathManager *dataPathManager, int w, int h)
        : OpenGlDrawContext(*dataPathManager, w, h, &SdlOpenGlContext::instance)
    {
        this->w = w;
        this->h = h;
        init();
        resize(w, h, NULL);
    }

    SdlDrawContext::SdlDrawContext(SdlOpenGlDrawContext *context) : CompositeDrawContext(context) {
    }

    void SdlOpenGlDrawContext::setupScreenBlit() {
        SdlOpenGlContext::instance.setLogicalFramebuffer(
            getDefaultFrameBuffer(), getWidth(), getHeight());
    }

}}

void SdlOpenGlContext_setWindow(SDL_Window *window) {
    gametools::graphics_manager::SdlOpenGlContext::instance.setWindow(window);
}

void SdlOpenGlContext_setLogicalFramebuffer(unsigned int fbo, int logicalW, int logicalH) {
    gametools::graphics_manager::SdlOpenGlContext::instance.setLogicalFramebuffer(fbo, logicalW, logicalH);
}

void SdlGraphics_WindowToLogical(float winX, float winY, float *outX, float *outY) {
    gametools::graphics_manager::SdlOpenGlContext::instance.windowToLogical(winX, winY, outX, outY);
}

bool SdlGraphics_SaveScreenshot(const char *path) {
    using namespace gametools::graphics_manager;
    SdlOpenGlContext &ctx = SdlOpenGlContext::instance;
    if (ctx.m_srcFbo == 0 || ctx.m_logicalW <= 0 || ctx.m_logicalH <= 0)
        return false;
    // SOIL_save_screenshot reads the bound GL_READ_FRAMEBUFFER: point it at
    // the engine FBO. The next presentRenderbuffer rebinds it every frame,
    // and engine draws go through combined binds, so no restore is needed.
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER, ctx.m_srcFbo);
    // SOIL flips assuming tightly packed rows; PACK_ALIGNMENT defaults to 4.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    int ok = SOIL_save_screenshot(path, SOIL_SAVE_TYPE_BMP, 0, 0,
                                  ctx.m_logicalW, ctx.m_logicalH);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    return ok != 0;
}
