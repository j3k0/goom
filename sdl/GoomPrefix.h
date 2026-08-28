/* Force-included prefix for the Goom SDL build (replaces the AliHood
   prefix header). Goom-Config.pch is plain text and only pulls ObjC
   headers under __OBJC__, so a textual include is safe. */
#ifndef GOOM_SDL_PREFIX_H
#define GOOM_SDL_PREFIX_H
#include "../Goom/Goom-Config.pch"
#endif
