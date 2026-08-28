#ifndef SDLGRAPHICS_H
#define SDLGRAPHICS_H

#include "graphicsmanager.h"
#include "FPDataPathManager.h"
#include "OpenGlDrawContext.h"
#include "CompositeDrawContext.h"

struct SDL_Window;

namespace gametools {
namespace graphics_manager {

    class SdlOpenGlDrawContext : public OpenGlDrawContext {
    public:
        SdlOpenGlDrawContext(DataPathManager *dataPathManager, int w, int h);
        // Publish the engine screen FBO + logical render size to the GL
        // context wrapper (base-class accessors are protected). Call once the
        // context is fully initialised.
        void setupScreenBlit();
    };

    class SdlDrawContext : public CompositeDrawContext {
    public:
        SdlDrawContext(SdlOpenGlDrawContext *context);
    };

}}

// Hand the SDL window to the GL context wrapper (swap target).
void SdlOpenGlContext_setWindow(SDL_Window *window);

// Tell the GL context wrapper which engine framebuffer holds the rendered
// frame and at what (fixed) logical size, so it can blit it letterboxed to
// the real drawable at present time. Call once after the DrawContext is up.
void SdlOpenGlContext_setLogicalFramebuffer(unsigned int fbo, int logicalW, int logicalH);

// Inverse-map a window-space (SDL mouse event) coordinate into the engine's
// logical coordinate space, undoing the letterbox blit. Identity when the
// window is at the logical size.
void SdlGraphics_WindowToLogical(float winX, float winY, float *outX, float *outY);

// Read back the engine's logical framebuffer and save it as a BMP at `path`.
// Reads the engine FBO (not the window back buffer, which is undefined after
// the swap), so letterbox bars are excluded. False when no logical
// framebuffer is set or the write failed.
bool SdlGraphics_SaveScreenshot(const char *path);

#endif // SDLGRAPHICS_H
