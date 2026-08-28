#ifndef GT_OPENGL_H
#define GT_OPENGL_H

#include "config.h"
#define GL_GLEXT_PROTOTYPES

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#elif defined (HAVE_OPENGL_GL_H)
# include <OpenGL/gl.h>
#elif defined (IOS)
# include <OpenGLES/ES1/gl.h>
#elif defined (ANDROID)
# include <GLES/gl.h>
#elif defined (MACOSX)
# include <OpenGL/gl.h>
# include <Carbon/Carbon.h>
#elif defined (WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <wingdi.h>
# include <GL/gl.h>
#else
#error Unsupported platform...
#endif

#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#elif defined (HAVE_OPENGL_GLEXT_H)
# include <OpenGL/glext.h>
#elif defined (IOS)
# include <OpenGLES/ES1/glext.h>
#elif defined (ANDROID)
# include <GLES/glext.h>
#endif

#if defined(__APPLE__) || defined(__APPLE_CC__) || defined(ANDROID)
# define APIENTRY
#endif

#if !defined(IOS) && !defined(ANDROID)
#define GL_COLOR_ATTACHMENT0_OES GL_COLOR_ATTACHMENT0
#define GL_FRAMEBUFFER_COMPLETE_OES GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_OES GL_FRAMEBUFFER_INCOMPLETE_FORMATS
#define GL_FRAMEBUFFER_UNSUPPORTED_OES GL_FRAMEBUFFER_UNSUPPORTED
#define GL_FRAMEBUFFER_OES GL_FRAMEBUFFER
#define GL_RENDERBUFFER_OES GL_RENDERBUFFER
#define glBindFramebufferOES glBindFramebufferEXT
#define glBindRenderbufferOES glBindRenderbufferEXT
#define glBlendFuncSeparateOES glBlendFuncSeparateEXT
#define glCheckFramebufferStatusOES glCheckFramebufferStatusEXT
#define glDeleteFramebuffersOES glDeleteFramebuffersEXT
#define glDeleteRenderbuffersOES glDeleteRenderbuffersEXT
#define glFramebufferRenderbufferOES glFramebufferRenderbufferEXT
#define glFramebufferTexture2DOES glFramebufferTexture2DEXT
#define glGenFramebuffersOES glGenFramebuffersEXT
#define glGenRenderbuffersOES glGenRenderbuffersEXT
#define glOrthof glOrtho
#define glGenerateMipmapOES glGenerateMipmap
#endif

#endif
