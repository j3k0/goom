/*
 *  config.h — SDL desktop platform binding (sdl/).
 *
 *  Equivalent of iphone/xcodeproject/config.h for the SDL2 build.
 *  Must come first in the include path so "config.h" resolves here.
 */

#ifndef FOVEA_SDL_CONFIG_H
#define FOVEA_SDL_CONFIG_H

/* the location of <hash_map> (deprecated SGI extension, still shipped
   by both libc++ and libstdc++) */
#define HASH_FUN_H <ext/hash_fun.h>
#define HASH_MAP_H <ext/hash_map>
#define HASH_NAMESPACE __gnu_cxx
#define HASH_SET_H <ext/hash_set>

#define HAVE_STRTOUL 1

#ifdef __APPLE__
#define HAVE_STRUCT_SOCKADDR_SA_LEN 1
#endif
#define USE_AUDIO 1

/* Render the frame into the engine's own screen FBO at the fixed logical
   size (see OpenGlDrawContext::init / gScreenFBO), then blit it
   letterboxed to the real window drawable in SdlGraphics. This lets the
   window be resized or toggled to fullscreen at runtime without an engine
   relayout (the GUI lays out once at init and glViewport is re-asserted
   per frame at the logical size). */
#define REQUIRE_DEFAULT_RENDERBUFFER 1

#endif
