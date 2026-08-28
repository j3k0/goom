//
//  OpenGlDrawContext.mm
//  flobopop
//
//  Created by Florent Boudet on 15/11/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

//#define BENCH_BLIT 1

#include "OpenGlDrawContext.h"
// #include "ios_filepath.h"
#include <sys/stat.h>
#include <deque>
#include <list>
#include "SOIL.h"

#ifdef ANDROID
# define REQUIRE_POWER_OF_TWO 1
# define USE_TEXTURES_16BITS 1
# define SHOULD_LOAD_TEXTURES_16BITS() \
    ((GTGetPlatformPerfo() > 0.0f && GTGetPlatformPerfo() < 0.5f) || (GTGetPlatformPerfo() <= 0.0f && g_width+g_height < 700+480))
# define USE_GL_FILTER GL_LINEAR
# define USE_GL_MIPMAP_FILTER GL_LINEAR_MIPMAP_NEAREST
# define ENABLE_MIPMAPS 1
#endif

#ifdef IOS
# include "CoreFoundation/CoreFoundation.h"
# define REQUIRE_POWER_OF_TWO 1
# define REQUIRE_DEFAULT_RENDERBUFFER 1
// # define USE_GL_FILTER GL_NEAREST
# define ENABLE_MIPMAPS 1
# define ENABLE_PVR_TEXTURES 1
# define USE_TEXTURES_16BITS 1
# define SHOULD_LOAD_TEXTURES_16BITS() ( (g_width > 0) && ( g_width+g_height <= 640+480 /* 2G,3G,3GS */ ) )
#endif

#ifdef MACOSX
# define ENABLE_MIPMAPS 1
# define REQUIRE_POWER_OF_TWO 1
#endif

#ifndef USE_GL_FILTER
# define USE_GL_FILTER GL_LINEAR
#endif

#ifndef USE_GL_MIPMAP_FILTER
# define USE_GL_MIPMAP_FILTER GL_LINEAR_MIPMAP_LINEAR
#endif

static std::deque<std::string>  m_consoleLines;

#include "GTScreenOrientation.h"
#include "GTPlatform.h"
#include "GTPreferences.h"
#include "GLFreeType.h"
#ifndef GT_NO_FRIBIDI
#include "fribidi.h"
#endif
// #import "UIDevice+machine.h"
#include "Commander.h"
#include "GTLog.h"

using namespace gametools;
using namespace ios_fc;

static OpenGlContext *gContext;
static int g_width  = 0;
static int g_height = 0;

ios_fc::Mutex *glMutex;
#define SCOPED_GL_LOCK ios_fc::Lock glLock(*glMutex)

std::auto_ptr<ios_fc::Lock> OpenGlDrawContext::scopedGlLock() {
    return std::auto_ptr<ios_fc::Lock>(new ios_fc::Lock(*glMutex));
}

#ifdef DEBUGxxxxx
#define GL_GET_ERROR() \
while(1) {\
GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
GTLogf("OpenGL Error: %s:%d, %d\nSTACK:\n%s", __FILE__, __LINE__, err, get_stack_trace().c_str()); \
break; \
}
#else
#define GL_GET_ERROR() while(0) { break; }
#endif

static void setDefaultStates() {
    // By default "Texture" always enabled.
    glEnable(GL_TEXTURE_2D);
    
    // By default "Alpha blending" always enabled.
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
    glEnable(GL_BLEND); GL_GET_ERROR();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
    glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

class Benchmarking {
	int num;
	unsigned int frame;
	double sub;
	double tot[64]; // <- max methods = 64;
	double frames[64];
	double start;
	int num_ignore;
	double max_fps;

public:
	Benchmarking() : num(0),start(-1.0) {}
	void set_num_methods(int num) {
		if (this->num > 0) return;
		this->num = num;
		for (int i=0;i<num;++i) {
			tot[i] = 0.00001;
			frames[i] = 0;
		}
		sub = 0.0;
		frame = 0;
		num_ignore = 100;
		max_fps = 10.0;
	}
	inline unsigned int current_method() {
		return frame / 10;
	}
	inline void start_frame() {
		if (this->num == 0) return;
		if (num_ignore-->0) return;
		glFlush();
		start = ios_fc::getTimeMs();
	}
	inline void end_frame() {
		if (this->num == 0) return;
		if (num_ignore-->0) return;
		if (this->start < 0) return; // first frame...
		glFlush();
		double end = ios_fc::getTimeMs();
		sub += (end - start);
		int i = frame%10;
		if (i == 9) {
			int j = current_method();
			//glFlush();
			//double end_end = ios_fc::getTimeMs();
			//sub += (end_end - end);
			tot[j]    = tot[j]*0.97 + sub; // Give more weight to latest frames.
			frames[j] = frames[j]*0.97 + 10.0;
			sub = 0;
		}
		++frame;
		if (frame == 10*num) frame = 0;
	}
	inline void draw() {
		if (this->num == 0) return;
		for (int i=0; i<num; ++i) {
			double fps = 1000.0*(frames[i]/tot[i]);
			if (fps > max_fps) max_fps = fps*1.1;
			GLfloat x = 0, y = 30 + i * 30;
			GLfloat w = 320.0*fps/max_fps, h = 20;
			GLfloat vertices[8];
			GLushort faces[4] = {0,1,2,3};
			vertices[0] = x;   vertices[1] = y;
			vertices[2] = x+w; vertices[3] = y;
			vertices[4] = x;   vertices[5] = y+h;
			vertices[6] = x+w; vertices[7] = y+h;
			switch (i) {
				case 0: glColor4f(1, 0, 0, .5); break;
				case 1: glColor4f(0, 0, 1, .5); break;
				case 2: glColor4f(0, 1, 0, .5); break;
				case 3: glColor4f(1, 0, 1, .5); break;
			}
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
            // TODO: glGenBuffers, glBindBuffer, and glBufferData
            // GL_ELEMENT_ARRAY_BUFFER
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
			glEnableClientState(GL_VERTEX_ARRAY);
			glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faces[0]);		
			glDisable(GL_BLEND);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
		glColor4f(1,1,1,1);
	}
};
Benchmarking BENCH;

#if 0
#include <libgen.h>
void writePPM(const char *path, GLenum type, int width, int height, int p2width, int p2height, GLubyte *data) {
    FILE *fout;
    char fname[2048];
    char *ppath = strdup(path);
    sprintf(fname, "/tmp/%s.ppm", basename(ppath));
    fout = fopen(fname, "w");
    if (type == GL_RGB) {
        fprintf(fout, "P3\n");
        fprintf(fout, "%d %d\n", p2width, p2height);
        fprintf(fout, "255\n");
        for (int y=0; y<p2height; ++y) {
            for (int x=0; x<p2width; ++x) {
                GLubyte *c = data + (p2width*y + x) * 3;
                fprintf(fout, "%d %d %d ", c[0], c[1], c[2]);
            }        
            fprintf(fout, "\n");
        }
    }
    else {
        fprintf(fout, "P3\n");
        fprintf(fout, "%d %d\n", p2width, p2height);
        fprintf(fout, "255\n");
        for (int y=0; y<p2height; ++y) {
            for (int x=0; x<p2width; ++x) {
                GLubyte *c = data + (p2width*y + x) * 4;
                fprintf(fout, "%d %d %d ", c[0], c[1], c[2]);
            }        
            fprintf(fout, "\n");
        }
    }
}
#endif

//-- RGB<->HSV conversion

static int max3(int i1, int i2, int i3)
{
    if ((i1>=i2)&&(i1>=i3)) return i1;
    if ((i2>=i3)&&(i2>=i1)) return i2;
    return i3;
}

static int min3(int i1, int i2, int i3)
{
    if ((i1<=i2)&&(i1<=i3)) return i1;
    if ((i2<=i3)&&(i2<=i1)) return i2;
    return i3;
}

//-- RGB, each 0 to 255
//-- H = 0.0 to 360.0 (corresponding to 0..360.0 degrees around hexcone)
//-- S = 0.0 (shade of gray) to 1.0 (pure color)
//-- V = 0.0 (black) to 1.0 (white)

//-- Based on C Code in "Computer Graphics -- Principles and Practice,"
//-- Foley et al, 1996, pp. 592,593.
static inline HSVA image_rgba2hsva(RGBA c)
{
    HSVA res;
    float minVal = (float)min3(c.red, c.blue, c.green);
    res.value    = (float)max3(c.red, c.green, c.blue);
    float delta  = res.value - minVal;
    
    // -- Calculate saturation: saturation is 0 if r, g and b are all 0
    if (res.value == 0.0f)
        res.saturation = 0.0f;
    else
        res.saturation = delta / res.value;
    
    if (res.saturation == 0.0f)
        res.hue = 0.0f; // Achromatic: When s = 0, h is undefined but who cares
    else             // Chromatic
        if (res.value == c.red) // between yellow and magenta [degrees]
            res.hue = 60.0f * (c.green - c.blue) / delta;
        else if (res.value == c.green) // between cyan and yellow
            res.hue = 120.0 + 60.0f * (c.blue - c.red) / delta;
        else // between magenta and cyan
            res.hue = 240.0f + 60.0f * (c.red - c.green) / delta;
    
    if (res.hue < 0.0f) res.hue += 360.0f;
    // return a list of values as an rgb object would not be sensible
    res.value /= 255.0f;
    res.alpha = c.alpha;
    return res;
}

static inline RGBA image_hsva2rgba(HSVA c)
{
    float red=0.0f,green=0.0f,blue=0.0f,hueTemp=0.0f;
    if (c.saturation == 0.0f) // color is on black-and-white center line
    {
        red   = c.value; // achromatic: shades of gray
        green = c.value; // supposedly invalid for h=0 but who cares
        blue  = c.value;
    }
    else { // chromatic color
        if (c.hue == 360.0f)      // 360 degrees same as 0 degrees
            hueTemp = 0.0f;
        else {
            hueTemp = c.hue / 60.0f;  // h is now in [0,6)
            // hueTemp = hueTemp / 60.0f;
        }
        float i = floor(hueTemp); // largest integer <= h
        float f = hueTemp - i;    // fractional part of h
        
        float p = c.value * (1.0f - c.saturation);
        float q = c.value * (1.0f - (c.saturation * f));
        float t = c.value * (1.0f - (c.saturation * (1.0-f)));
        
        switch((int)i) {
            case 0:
                red   = c.value;
                green = t;
                blue  = p;
                break;
            case 1:
                red   = q;
                green = c.value;
                blue  = p;
                break;
            case 2:
                red   = p;
                green = c.value;
                blue  = t;
                break;
            case 3:
                red   = p;
                green = q;
                blue  = c.value;
                break;
            case 4:
                red   = t;
                green = p;
                blue  = c.value;
                break;
            case 5:
                red   = c.value;
                green = p;
                blue  = q;
                break;
        }
    }
    RGBA ret;
    ret.red   = (GLubyte)(red   * 255.0f);
    ret.green = (GLubyte)(green * 255.0f);
    ret.blue  = (GLubyte)(blue  * 255.0f);
    ret.alpha = c.alpha;
    return ret;
}

/**
 * Shift the saturation of a surface
 */
static GLubyte *image_rgba_shift_hsv(GLubyte *src, int src_w, int src_h, float h, float s, float v)
{
    GLubyte *ret = (GLubyte*)malloc(src_h*src_w*4);
    for (int y=src_h; y--;) {
        for (int x=src_w; x--;) {
            GLubyte *c = src + 4*(src_w*y+x);
            GLubyte *r = ret + 4*(src_w*y+x);
            RGBA rgba;
            rgba.red = c[0];
            rgba.green = c[1];
            rgba.blue = c[2];
            rgba.alpha = c[3];
            HSVA hsva = image_rgba2hsva(rgba);
            hsva.hue += h;
            if (hsva.hue > 360.0f) hsva.hue -= 360.0f;
            if (hsva.hue < 0.0f) hsva.hue += 360.0f;
            hsva.saturation += s;
            if (hsva.saturation > 1.) hsva.saturation = 1.f;
            if (hsva.saturation < 0.0f) hsva.saturation = .0f;
            hsva.value += v;
            if (hsva.value > 1.) hsva.value = 1.f;
            if (hsva.value < 0.0f) hsva.value = .0f;
            rgba = image_hsva2rgba(hsva);
            r[0] = rgba.red;
            r[1] = rgba.green;
            r[2] = rgba.blue;
            r[3] = rgba.alpha;
        }
    }
    return ret;
}

/**
 * Shift the hue of a surface
 */
static GLubyte *image_rgba_shift_hue(GLubyte *src, int src_w, int src_h, float hue_offset)
{
    GLubyte *ret = (GLubyte*)malloc(src_h*src_w*4);
    for (int y=src_h; y--;) {
        for (int x=src_w; x--;) {
            GLubyte *c = src + 4*(src_w*y+x);
            GLubyte *r = ret + 4*(src_w*y+x);
            RGBA rgba;
            rgba.red = c[0];
            rgba.green = c[1];
            rgba.blue = c[2];
            rgba.alpha = c[3];
            HSVA hsva = image_rgba2hsva(rgba);
            hsva.hue += hue_offset;
            if (hsva.hue > 360.0f) hsva.hue -= 360.0f;
            if (hsva.hue < 0.0f) hsva.hue += 360.0f;
            rgba = image_hsva2rgba(hsva);
            r[0] = rgba.red;
            r[1] = rgba.green;
            r[2] = rgba.blue;
            r[3] = rgba.alpha;
        }
    }
    return ret;
}

/**
 * Shift the hue of a surface
 */
static GLubyte *image_rgba_shift_hue_masked(GLubyte *src, int src_w, int src_h,
                                            GLubyte *mask, int mask_w, int mask_h, float hue_offset)
{
    GLubyte *ret = (GLubyte*)malloc(src_h*src_w*4);
    for (int y=src_h; y--;) {
        for (int x=src_w; x--;) {
            GLubyte *c = src + 4*(src_w*y+x);
            GLubyte *r = ret + 4*(src_w*y+x);
            GLubyte *m = mask + 4*(mask_w*y+x);
            if ((x > mask_w) || (y > mask_h) || (m[3] != 0)) {
                r[0] = c[0];
                r[1] = c[1];
                r[2] = c[2];
                r[3] = c[3];
            }
            else {
                RGBA rgba;
                rgba.red = c[0];
                rgba.green = c[1];
                rgba.blue = c[2];
                rgba.alpha = c[3];
                HSVA hsva = image_rgba2hsva(rgba);
                hsva.hue += hue_offset;
                if (hsva.hue > 360.0f) hsva.hue -= 360.0f;
                if (hsva.hue < 0.0f) hsva.hue += 360.0f;
                rgba = image_hsva2rgba(hsva);
                r[0] = rgba.red;
                r[1] = rgba.green;
                r[2] = rgba.blue;
                r[3] = rgba.alpha;
            }
        }
    }
    return ret;
}


GLenum toGL(ImageType type) {
	switch (type) {
		case IMAGE_RGB:
			return GL_RGB;
		case IMAGE_RGBA:
			return GL_RGBA;
	}
	return GL_RGBA;
}

int toBpp(ImageType type) {
	switch (type) {
		case IMAGE_RGB:
			return 3;
		case IMAGE_RGBA:
			return 4;
	}
	return 4;
}

static const char *getFBOStatusString(int status) {
    if (status == 0x8CD5) return "Framebuffer complete";
    if (status == 0x8CD6) return "Framebuffer incomplete attachment";
    if (status == 0x8CD7) return "Framebuffer incomplete missing attachment";
    if (status == 0x8CD9) return "Framebuffer incomplete dimensions";
    if (status == 0x8CDA) return "Framebuffer incomplete formats";
    if (status == 0x8CDD) return "Framebuffer unsupported";
    return "[Invalid status code]";
}

/*
static float stubColor[][4] = {
    {1., 0.5, 0.5, 1.},
    {0.5, 0.5, 1., 1.},
    {0.5, 1., 0.5, 1.},
    {1., 1., 0.5, 1.},
    {1., 0.5, 1., 1.},
    {1., 1., 0.5, 1.},
    {1., 0, 0, 1.},
    {0, 0, 1., 1.},
    {0, 1., 0, 1.}
};
*/
static __inline__ int power_of_2(int input)
{
    int value = 1;
    while (value < input) {
        value <<= 1;
    }
    return value;
}

/*
GLFont(const char *fname, unsigned int h, float letter_spacing) { init(fname, h, letter_spacing); }
GLFont() {}
~GLFont() {}
float letter_spacing;
//The init function will create a font of
//of the height h from the file fname.
void init(const char * fname, unsigned int h, float letter_spacing);
//Free all the resources assosiated with the font.
void clean();
//The flagship function of the library - this thing will print
//out text at window coordinates x,y, using the font ft_font.
//The current modelview matrix will also be applied to the text. 
void print(float x, float y, const wchar_t *fmt, ...) ;
void printCentered(float x, float y, const wchar_t *fmt, ...);
*/
static std::wstring toWString(const std::string &s0)
{
    wchar_t *s1 = new wchar_t[s0.size() + 1];
    for (unsigned int j=0; j<s0.size(); ++j) s1[j] = s0[j];
    s1[s0.size()] = 0;
    std::wstring s2 = s1;
    delete[] s1;
    return s2;
}
static std::string toString(const std::wstring &s0)
{
    char *s1 = new char[s0.size() + 1];
    for (unsigned int j=0; j<s0.size(); ++j) s1[j] = s0[j];
    s1[s0.size()] = 0;
    std::string s2 = s1;
    delete[] s1;
    return s2;
}

static int gCurrentFBO = 0;
static int gScreenFBO = 0;
static GLfloat *gCurrentMatrix = NULL;
void iglBindFramebufferOES(GLuint i, GLfloat *matrix) {
	gCurrentMatrix = matrix;
	if (i == gCurrentFBO) return;
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, i); GL_GET_ERROR();
	/* if (matrix) glLoadMatrixf(matrix); */
	gCurrentFBO = i;
}

/*
static GLfloat *prevPrint = NULL;
void printMatrix(GLfloat *matrix, const char *txt) {
	if (gCurrentFBO == 7) printf("txt=%s\n", txt);
	if (matrix == prevPrint) return;
	prevPrint=matrix;
	printf("matrix(0x%08x) %s =\n",matrix, txt);
	for (int i=0; i<2; ++i) {
		for (int j=0; j<2; ++j) printf("%1.10f ", matrix[i*4+j]);
		printf("\n");
	}
}
*/

static ios_fc::HashMap g_fontFilesData;

class OpenGlIosFont : public IosFont
{
private:
	GLFont *mCustomFont;
	float mSize;
	IosFontFx mFX;

#define kMaxStringLength 2048
    unsigned short mBackupOutputBuffer[kMaxStringLength];
    char           mBackupText[kMaxStringLength];
    
    void utf8ToUnicode(const char *utf8Text, unsigned short outputBuffer[kMaxStringLength]) {
#ifdef GT_NO_FRIBIDI
        // Plain UTF-8 -> UCS-2 conversion of the first code point, no bidi
        // reordering. goom never draws text; this only keeps the API usable.
        unsigned int c = (unsigned char)utf8Text[0];
        int extra = (c == 0) ? 0 : ((c & 0xE0) == 0xE0) ? 2 : ((c & 0xC0) == 0xC0) ? 1 : 0;
        if (extra == 2)      c = ((c & 0x0F) << 12) | (((unsigned char)utf8Text[1] & 0x3F) << 6) | ((unsigned char)utf8Text[2] & 0x3F);
        else if (extra == 1) c = ((c & 0x1F) << 6) | ((unsigned char)utf8Text[1] & 0x3F);
        outputBuffer[0] = (unsigned short)c;
        outputBuffer[1] = 0;
#else
        if (utf8Text[0] == 0) {
            // Empty text
            outputBuffer[0] = 0;
            return;
        }
        if (strcmp(utf8Text, mBackupText) == 0) {
            // Same text than last time
            for (int i=0; i<kMaxStringLength; ++i) {
                unsigned short c = mBackupOutputBuffer[i];
                outputBuffer[i] = c;
                if (c == 0) break;
            }
            return;
        }
        strcpy(mBackupText, utf8Text);
        FriBidiChar  mVisualBuffer[kMaxStringLength];
        FriBidiChar  mWorkingBuffer[kMaxStringLength];
        FriBidiLevel mEmbeddingLevels[kMaxStringLength];
		int size = strlen(utf8Text);
        assert(size < kMaxStringLength);
		bzero(mVisualBuffer,  kMaxStringLength * sizeof(FriBidiChar));
		bzero(mWorkingBuffer, kMaxStringLength * sizeof(FriBidiChar));
		// bzero(outputBuffer,   kMaxStringLength * sizeof(unsigned short));
        // mWorkingBuffer[0] = 0;
        // mVisualBuffer[0]  = 0;
        // outputBuffer[0]   = 0;
        
		FriBidiParType input_base_direction = FRIBIDI_PAR_LTR;
        // unsigned char c = ((const unsigned char *)utf8Text)[0];
        // if ((c >= 0xd8) && (c <= 0xdb)) // Arabic UTF-8 Blocks
        // input_base_direction = FRIBIDI_PAR_WRTL;
        
		fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, utf8Text, size, mWorkingBuffer);
		if (fribidi_log2vis(mWorkingBuffer, size, &input_base_direction,
							mVisualBuffer, NULL, NULL, mEmbeddingLevels)) {
			// Convert to 16 bits (FriBiDi char is 32 bits)
			for (int i=0; i<kMaxStringLength; ++i) {
                mBackupOutputBuffer[i] = outputBuffer[i] = mVisualBuffer[i];
                if (outputBuffer[i] == 0) return;
            }
			return;
		}
		else {
			for (int i=0; i<kMaxStringLength; ++i) {
                mBackupOutputBuffer[i] = outputBuffer[i] = mWorkingBuffer[i];
                if (outputBuffer[i] == 0) return;
            }
			return;
        }
#endif
    }
public:
	OpenGlIosFont(DataPathManager &dataPathManager, const char *path, float size, const IosFontFx &fx = Font_STD) : mCustomFont(NULL), mSize(size), mFX(fx) {
        ios_fc::FilePath ipath(path);
		GTLogf("+ Loading Font [%.1f]: %s", size, ipath.getBaseName().c_str());
        DataInputStream *f = dataPathManager.hasDataInputStream(path) ? dataPathManager.openDataInputStream(path) : NULL;
        if (f) {
            ios_fc::HashValue *dataHolder = g_fontFilesData.get(path);
            ios_fc::VoidBuffer *data = NULL;
            if (dataHolder != NULL) {
                GTLogf("  Using cached data.");
                data = static_cast<ios_fc::VoidBuffer *>(dataHolder->value.ptr);
            }
            else {
                GTLogf("  Loading from file.");
                BufferedStream stream(*f);
                data = new ios_fc::VoidBuffer(stream.data());
                g_fontFilesData.put(path, static_cast<void*>(data));
            }
            mCustomFont = new GLFont(data->ptr(), data->size(), size, 0.0f, size * 1.5f);
            mSize = size;
            delete f;
        }
        else {
            GTLogf("  Font file not found, text rendering disabled for this font.");
        }
	}
	
	virtual ~OpenGlIosFont() {
        if (mCustomFont == NULL) return;
		GTLogf("- Freeing Font [%.1f]: %s", mSize, mCustomFont->getFileName());
		mCustomFont->clean();
		delete mCustomFont;
	}
    virtual float getTextWidth(const char *text);
    virtual float getTextHeight(const char *text);
    virtual float getHeight();
    virtual float getLineSkip();
	
	// void applyBlendMode() {}
	
	void print(float x, float y, const char *txt, float alpha, char align, float dx, float dy) {
        printWithShadow(x,y,1.0f,2.0f,txt,alpha,align,dx,dy);
	}
	void printWithShadow(float x, float y, float sx, float sy, const char *txt, float alpha, char align, float dx, float dy) {
		if (mCustomFont == NULL) return;
		if (gCurrentMatrix) {
			glPushMatrix();
			glLoadMatrixf(gCurrentMatrix);
		}
		// applyBlendMode();
        unsigned short utxt[kMaxStringLength];
		utf8ToUnicode(txt, utxt);
        
        if (mFX.shadow) {
            glColor4ub(0,0,0, alpha*0xc0);
            mCustomFont->printUnicode(mSize, x+sx, y+sy, utxt, align, dx, -dy);
        }
        glColor4ub(mFX.red,mFX.green,mFX.blue, alpha*255);
        mCustomFont->printUnicode(mSize, x,y, utxt, align, dx, -dy);
		
		glColor4ub(0xFF,0xFF,0xFF,0xFF);
		if (gCurrentMatrix)
			glPopMatrix();
	}
};

float OpenGlIosFont::getTextWidth(const char *text)
{
    if (mCustomFont == NULL) return 0.0f;
    unsigned short utxt[kMaxStringLength];
    utf8ToUnicode(text, utxt);
    return mCustomFont->getWidthUnicode(mSize, utxt);
}

float OpenGlIosFont::getTextHeight(const char *text)
{
    if (mCustomFont == NULL) return 0.0f;
    return mCustomFont->getHeightUnicode(mSize, NULL); // utf8ToUnicode(text)); // mSize * GTGetScaleFactor();
}

float OpenGlIosFont::getHeight()
{
    return mSize;
}

float OpenGlIosFont::getLineSkip()
{
    if (mCustomFont == NULL) return mSize;
    return mCustomFont->line_spacing * mSize / mCustomFont->getHeight();
}

class OpenGlSurface;

class OpenGlSurfaceLibrary {
public:
    void registerSurface(OpenGlSurface *surf)   { m_surfaceLib.push_back(surf); }
    void unregisterSurface(OpenGlSurface *surf) { m_surfaceLib.remove(surf);   }

    // Light flush free OpenGl related data from surface.
    // Used when app was paused.
    void unrefGlObjects();
    // Heavy flush free everything that can be restored.
    // Used when app was backgrounded.
    void freeGlObjects();
private:
    std::list<OpenGlSurface*> m_surfaceLib;
};
static OpenGlSurfaceLibrary g_surfaceLibrary;

#if ENABLE_PVR_TEXTURES

#define kMaxTextureLevel 16
typedef struct _PVRTexData {
    VoidBuffer    buffer;
    GLenum internalFormat;
    uint32_t      pvrDataOffset[kMaxTextureLevel];
    uint32_t      dataSize [kMaxTextureLevel];
    uint32_t      width    [kMaxTextureLevel];
    uint32_t      height   [kMaxTextureLevel];
} PVRTexData;

typedef struct _PVRTexHeader
{
    uint32_t headerLength;
    uint32_t height;
    uint32_t width;
    uint32_t numMipmaps;
    uint32_t flags;
    uint32_t dataLength;
    uint32_t bpp;
    uint32_t bitmaskRed;
    uint32_t bitmaskGreen;
    uint32_t bitmaskBlue;
    uint32_t bitmaskAlpha;
    uint32_t pvrTag;
    uint32_t numSurfs;
} PVRTexHeader;

#endif

// OpenGL Surface
class OpenGlSurface
{
public:
    OpenGlSurface(ImageType type, const char *name)
      : m_compressed(false), m_type(type), m_opaque(type==IMAGE_RGB?true:false), m_name(name)
	{
		m_texture   = 0;
        m_textureOK = false;
		m_fbo       = 0;
        m_fboDepth  = 0;
		m_nRef      = 0;
		m_nDataRef  = 0;
		m_data      = NULL;
        m_dataOK    = false;
		m_h         = 1;
		m_w         = 1;
		m_p2w       = 1;
		m_p2h       = 1;
#if IMAGE_QUALITY_BENCHMARK
        m_highestFactor = 0.0f;
        m_lowestFactor = 16.0f;
#endif
        m_useCompression = COMPRESSION_NONE;
        g_surfaceLibrary.registerSurface(this);
	}

	virtual ~OpenGlSurface() {
        g_surfaceLibrary.unregisterSurface(this);
        SCOPED_GL_LOCK;
        gContext->setCurrent();
		if (m_fbo) {
			glDeleteFramebuffersOES(1, &m_fbo); GL_GET_ERROR();
            if (m_fboDepth)
                glDeleteRenderbuffersOES(1, &m_fboDepth);
        }
		if (m_textureOK) {
			glDeleteTextures(1, &m_texture); GL_GET_ERROR();
        }
        // decDataRef();
        // I assume data will never be shared between openGlSurfaces.
        if (m_data != NULL) {
            GTLogf("  WARNING %s DATA IS STILL REFERENCED SOMEWHERE...", ios_fc::FilePath(m_name.c_str()).getBaseName().c_str());
            free(m_data);
        }
		GTLogf("- Freeing GLSurface: %s", ios_fc::FilePath(m_name.c_str()).getBaseName().c_str());
        
#if IMAGE_QUALITY_BENCHMARK
        if (m_lowestFactor < m_highestFactor && ((m_lowestFactor * m_highestFactor > 1.0f) || (m_highestFactor > 1.5f))) {
            float ll = log2f(m_lowestFactor);
            float lh = log2f(m_highestFactor);
            if (ll < -4.0f) ll = -4.0f;
            if (lh < -4.0f) lh = -4.0f;
            if (ll > 4.0f) ll = 4.0f;
            if (lh > 4.0f) lh = 4.0f;
            printf("           !!! Factor > %2.2f: ", m_lowestFactor);
            if (ll < 0) {
                while (ll < 0) {
                    printf("-");
                    ll += 0.1f;
                }
            }
            else if (ll > 0) {
                while (ll > 0) {
                    printf("+");
                    ll -= 0.1f;
                }
            }
            printf("\n           !!! Factor < %2.2f: ", m_highestFactor);
            if (lh < 0) {
                while (lh < 0) {
                    printf("-");
                    lh += 0.1f;
                }
            }
            else if (lh > 0) {
                while (lh > 0) {
                    printf("+");
                    lh -= 0.1f;
                }
            }
            printf("\n");
        }
        else if ((m_lowestFactor == m_highestFactor) && ((m_lowestFactor < 0.75f) || (m_lowestFactor > 1.5f))) {
            float ll = log2f(m_lowestFactor);
            if (ll < -4.0f) ll = -4.0f;
            if (ll > 4.0f) ll = 4.0f;
            printf("           !!! Factor = %2.2f: ", m_lowestFactor);
            if (ll < 0) {
                while (ll < 0) {
                    printf("-");
                    ll += 0.1f;
                }
            }
            else if (ll > 0) {
                while (ll > 0) {
                    printf("+");
                    ll -= 0.1f;
                }
            }
            printf("\n");
        }
#endif
	}

    void setOpaque(bool opaque) { m_opaque = opaque; }
    
    int p2w() const { return m_p2w; }
    int p2h() const { return m_p2h; }

    void freeGlObjects() {
        if (m_textureOK) {
            glDeleteTextures(1, &m_texture); GL_GET_ERROR();
            m_texture = 0;
            m_textureOK = false;
        }
        if (m_fbo) {
            glDeleteFramebuffersOES(1, &m_fbo); GL_GET_ERROR();
            m_fbo = 0;
            if (m_fboDepth) {
                glDeleteRenderbuffersOES(1, &m_fboDepth);
                m_fboDepth = 0;
            }
        }
    }
    void unrefGlObjects() {
        // Called when the OpenGl context has been destroyed,
        // Erase all references to textures and fbo.
        m_texture = 0;
        m_textureOK = false;
        m_fbo = 0;
        m_fboDepth = 0;
    }

protected:
	friend class OpenGlSurfaceRef;
    GLuint m_fbo, m_fboDepth;
	int m_nRef;
    bool m_compressed;
#if ENABLE_PVR_TEXTURES
    PVRTexData m_pvrData;
#endif
	ImageType m_type;
    GLuint m_texture;
    bool m_textureOK;
	int m_w,m_h;
	int m_p2w,m_p2h;
	GLubyte *m_data;
    bool     m_dataOK; // Because data=NULL is also possible for Pure OpenGL surfaces.
    GLsizei m_dataSize;
	int m_nDataRef;
	GLfloat m_matrix[16];
    bool m_opaque;
	std::string m_name;
#if IMAGE_QUALITY_BENCHMARK
    float m_highestFactor; // Track textures needing higher resolution.
    float m_lowestFactor; // Track textures needing lower resolution.
#endif
	enum {
        COMPRESSION_NONE = 0,
        COMPRESSION_PVR = 1,
        COMPRESSION_RGBA4 = 2,
        COMPRESSION_RGB5A1 = 3
    } m_useCompression;
public:
    // Reference counter (for the data buffer)
    inline void incDataRef() {
        m_nDataRef++;
    }
	inline void decDataRef() {
		--m_nDataRef;
        if (m_nDataRef <= 0) {
            if (m_data != NULL) {
                if (m_useCompression == COMPRESSION_PVR) {
#if ENABLE_PVR_TEXTURES
                    m_pvrData.buffer.realloc(0);
                    for (int i=0; i<kMaxTextureLevel; ++i)
                        m_pvrData.dataSize[i] = 0;
                    // Don't free data on PVR textures: data == m_pvrData.buffer
#endif
                }
                else {
                    free(m_data);
                }
                m_data = NULL;
            }
            m_dataOK = false;
        }
	}
    inline bool noDataRef() const { return m_nDataRef <= 0; }

    // Reference counter (for the surface this surface)
	inline void incRef() {
		++m_nRef;
	}
	inline void decRef() {
		--m_nRef;
	}
	inline bool noRef() const { return m_nRef <= 0; }

    void loadData() {
        if (noDataRef()) {
            GTLogf("+ Load Surface Data: %s", /*ios_fc::FilePath(m_name.c_str()).getBaseName().c_str()*/m_name.c_str());
            _loadData();
        }
        incDataRef();
    }

	inline void genTexture() {
		if (!m_textureOK) {
            bool needDataLoad = (m_dataOK == false);
            if (needDataLoad)
                loadData();
            // GTLogf("+ Create GL Texture: %s", ios_fc::FilePath(m_name.c_str()).getBaseName().c_str());

            int force_channels = SOIL_LOAD_AUTO;
            if (m_type == IMAGE_RGB)
                force_channels = SOIL_LOAD_RGB;
            else
                force_channels = SOIL_LOAD_RGBA;
            if (m_data == NULL) { 
                glEnable(GL_TEXTURE_2D); GL_GET_ERROR();
                glGenTextures(1, &m_texture); GL_GET_ERROR();
                glBindTexture(GL_TEXTURE_2D, m_texture); GL_GET_ERROR();
                glTexImage2D(GL_TEXTURE_2D, 0, toGL(m_type), m_p2w, m_p2h, 0, toGL(m_type), GL_UNSIGNED_BYTE, NULL); GL_GET_ERROR();
                // GTLog("                  [Null Texture]");
            }
#if ENABLE_PVR_TEXTURES
            else if (m_useCompression == COMPRESSION_PVR) {
                GTLog("glCompressed PVR");
                glEnable(GL_TEXTURE_2D); GL_GET_ERROR();
                glGenTextures(1, &m_texture); GL_GET_ERROR();
                glBindTexture(GL_TEXTURE_2D, m_texture); GL_GET_ERROR();
                const uint8_t *bytes = ((const uint8_t *)m_pvrData.buffer);
                for (int level = 0; level < kMaxTextureLevel; ++level) {
                    if (m_pvrData.dataSize[level] != 0) {
                        glCompressedTexImage2D(GL_TEXTURE_2D, level, m_pvrData.internalFormat, m_pvrData.width[level], m_pvrData.height[level], 0, m_pvrData.dataSize[level], bytes + m_pvrData.pvrDataOffset[level]);
                    }
                    else break;
                }
                if (m_pvrData.dataSize[1] == 0) { // No mipmaps?
                    GTLog("glGenerateMipmap PVR");
                    glGenerateMipmapOES(GL_TEXTURE_2D);
                }
            }
#endif
            else {
                m_texture = SOIL_create_OGL_texture(m_data, m_w, m_h, force_channels /* channels */,
                                                    SOIL_CREATE_NEW_ID,
                                                    0 
#if REQUIRE_POWER_OF_TWO
                                                    | SOIL_FLAG_POWER_OF_TWO
#endif
#if USE_TEXTURES_16BITS
                                                    | (m_useCompression == COMPRESSION_RGBA4  ? SOIL_FLAG_TEXTURE_16BITS : 0)
                                                    | (m_useCompression == COMPRESSION_RGBA4  ? SOIL_FLAG_TEXTURE_16BITS_4444 : 0)
                                                    | (m_useCompression == COMPRESSION_RGB5A1 ? SOIL_FLAG_TEXTURE_16BITS : 0)
                                                    | (m_useCompression == COMPRESSION_RGB5A1 ? SOIL_FLAG_TEXTURE_16BITS_5551: 0)
#endif
                                                   );
                /* check for an error during the load process */
                if(m_texture == 0) {
                    GTLogf("SOIL loading error: '%s'", SOIL_last_result());
                    return;
                }
#if ENABLE_MIPMAPS
                glGenerateMipmapOES(GL_TEXTURE_2D);
#endif
            }
//#if REQUIRE_POWER_OF_TWO /* Texture has already been created, we have to trust than w and h are Po2 */
//            m_p2w = power_of_2(m_w);
//            m_p2h = power_of_2(m_h);
//#else
//            m_p2w = m_w;
//            m_p2h = m_h;
//#endif
#if 0
			glEnable(GL_TEXTURE_2D); GL_GET_ERROR();
			glGenTextures(1, &m_texture); GL_GET_ERROR();
			glBindTexture(GL_TEXTURE_2D, m_texture); GL_GET_ERROR();
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   GL_GET_ERROR();
            if (!m_compressed) {
                /*GLenum internalFormat = (m_format == GL_RGB) ? GL_RGB565_OES : GL_RGBA4_OES;
                if (GTPlatformIsFasterThan(GT_IPHONE3G))
                    internalFormat = m_format;*/
                glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_p2w, m_p2h, 0, m_format, GL_UNSIGNED_BYTE, m_data); GL_GET_ERROR();
            }
            else {
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_format, m_p2w, m_p2h, 0, m_dataSize, m_data); GL_GET_ERROR();
            }
#endif
            /*
#if ENABLE_MIPMAPS
            if (m_data == NULL)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_FILTER);
            else
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_LINEAR_MIPMAP_NEAREST
#else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_FILTER); GL_GET_ERROR();
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, USE_GL_FILTER); GL_GET_ERROR();
            */
            // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrap_s);
            // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrap_t);
            m_textureOK = true;
            if (needDataLoad)
                decDataRef();
		}
	}

protected:
    virtual void _loadData() = 0;

	inline void genFBO() {
		if (!m_fbo) {
			//Setup Texture Framebuffer
			GL_GET_ERROR();
            GTLogf("genFBO(%p",this);
			bindTexture(); // Makes sure texture is ready
			glGenFramebuffersOES(1, &m_fbo); GL_GET_ERROR();
			iglBindFramebufferOES(m_fbo,NULL); GL_GET_ERROR();
			glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, m_texture, 0); GL_GET_ERROR();
            int status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
			if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
                GTLogf("genFBO[1]: %s", getFBOStatusString(status));
#if !defined(ANDROID) && !defined(IOS)
                if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES) {
                    // Incomplete attachment, trying to add a depth buffer... (some video cards require it)
                    glGenRenderbuffersOES(1, &m_fboDepth); GL_GET_ERROR();
                    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_fboDepth); GL_GET_ERROR();
                    glRenderbufferStorageEXT(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT, m_p2w, m_p2h); GL_GET_ERROR();
                    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER_OES, m_fboDepth); GL_GET_ERROR();
                    status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
                    if (status == GL_FRAMEBUFFER_COMPLETE_OES) {
                        glDepthFunc(GL_LEQUAL);
                        glDisable(GL_DEPTH_TEST);
                    }
                    GTLogf("genFBO[2]: %s", getFBOStatusString(status));
                }
#endif
			}
			// Setup OpenGL projection
			glPushMatrix();
			/* glLoadIdentity();
			   glViewport(0, 0, this->m_w, this->m_h);
			   glOrthof(0.0, (GLfloat) this->m_w, (GLfloat) this->m_h, 0.0, 0.0, 1.0);*/
			glTranslatef(0.0f, m_h, 0.0f);
			glScalef(1,-1,1);
			glGetFloatv(GL_MODELVIEW_MATRIX,&m_matrix[0]);
			glPopMatrix(); GL_GET_ERROR();
		}
	}
	
	inline void bindFBO() {
		genFBO();
		iglBindFramebufferOES(m_fbo,&m_matrix[0]);
	}

public:
	inline void bindTexture() {
		genTexture();
        if (m_texture) {
            glBindTexture(GL_TEXTURE_2D, m_texture); GL_GET_ERROR();
            // glEnable(GL_TEXTURE_2D); GL_GET_ERROR();
        }
        else {
            glDisable(GL_TEXTURE_2D); GL_GET_ERROR();
        }
	}
	inline void unbindTexture() {
		// glBindTexture(GL_TEXTURE_2D, 0);
        if (! m_texture) {
            glEnable(GL_TEXTURE_2D); GL_GET_ERROR();
        }
	}
};

class DataOpenGlSurface : public OpenGlSurface {
    private:
    public:
        DataOpenGlSurface(ImageType type, const char *name, int w, int h, int p2w, int p2h, GLubyte *data)
          : OpenGlSurface(type, name)
        {
            m_data   = data;
            m_dataOK = true;
            m_w = w;
            m_h = h;
            m_p2w = p2w;
            m_p2h = p2h;
            incDataRef(); // 
        }
        ~DataOpenGlSurface() {
            decDataRef();
        }
        virtual void _loadData() {
            // Data wont get unloaded (because we made sure so)
            // No need to do anything.
        }
};

class PureOpenGlSurface : public OpenGlSurface {
    public:
        PureOpenGlSurface(ImageType type, const char *name, int w, int h)
          : OpenGlSurface(type, name)
        {
            m_w = w;
            m_h = h;
            m_p2w = w;
            m_p2h = h;
//#if REQUIRE_POWER_OF_TWO
//            m_p2w = power_of_2(w);
//            m_p2h = power_of_2(h);
//#else
//            m_p2w = w;
//            m_p2h = h;
//#endif
        }
        virtual void _loadData() {
            // A Pure OpenGl Surface has no data.
            m_dataOK = true;
            // m_data   = (GLubyte*)malloc(m_p2h*m_p2w*4);
            m_data = NULL;
        }
};

// PVR Loader
#if ENABLE_PVR_TEXTURES
static char gPVRTexIdentifier[5] = "PVR!";
#define PVR_TEXTURE_FLAG_TYPE_MASK  0xff
enum
{
    kPVRTextureFlagTypePVRTC_2 = 24,
    kPVRTextureFlagTypePVRTC_4
};

static PVRTexData loadPvrData(std::auto_ptr<DataInputStream> file)
{
    BufferedStream   fileBuffer(*file);
    PVRTexData texData;
    texData.buffer = fileBuffer.data();
    // Unpack PVR data
    bool success = false;
    bool hasAlpha;
    PVRTexHeader *header = NULL;
    uint32_t flags, pvrTag;
    uint32_t dataLength = 0, dataOffset = 0;
    uint32_t blockSize = 0, widthBlocks = 0, heightBlocks = 0;
    uint32_t width = 0, height = 0, bpp = 4;
    uint8_t *bytes = NULL;
    uint32_t formatFlags;
        
    header = (PVRTexHeader*)texData.buffer;
    pvrTag = CFSwapInt32LittleToHost(header->pvrTag);
    
    if (gPVRTexIdentifier[0] != ((pvrTag >>  0) & 0xff) ||
        gPVRTexIdentifier[1] != ((pvrTag >>  8) & 0xff) ||
        gPVRTexIdentifier[2] != ((pvrTag >> 16) & 0xff) ||
        gPVRTexIdentifier[3] != ((pvrTag >> 24) & 0xff)) {
        return texData;
    }
    
    flags = CFSwapInt32LittleToHost(header->flags);
    formatFlags = flags & PVR_TEXTURE_FLAG_TYPE_MASK;
    
    if (formatFlags == kPVRTextureFlagTypePVRTC_4 || formatFlags == kPVRTextureFlagTypePVRTC_2)
    {
        if (formatFlags == kPVRTextureFlagTypePVRTC_4)
            texData.internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        else if (formatFlags == kPVRTextureFlagTypePVRTC_2)
            texData.internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        
        width = CFSwapInt32LittleToHost(header->width);
        height = CFSwapInt32LittleToHost(header->height);
        
        if (CFSwapInt32LittleToHost(header->bitmaskAlpha))
            hasAlpha = TRUE;
        else
            hasAlpha = FALSE;
        
        dataLength = CFSwapInt32LittleToHost(header->dataLength);
        
        bytes = ((uint8_t *)texData.buffer) + sizeof(PVRTexHeader);
        
        // Calculate the data size for each texture level and respect the minimum number of blocks
        int level = 0;
        while (dataOffset < dataLength)
        {
            if (formatFlags == kPVRTextureFlagTypePVRTC_4)
            {
                blockSize    = 4 * 4; // Pixel by pixel block size for 4bpp
                widthBlocks  = width / 4;
                heightBlocks = height / 4;
                bpp = 4;
            }
            else
            {
                blockSize    = 8 * 4; // Pixel by pixel block size for 2bpp
                widthBlocks  = width / 8;
                heightBlocks = height / 4;
                bpp = 2;
            }
            
            // Clamp to minimum number of blocks
            if (widthBlocks < 2)
                widthBlocks = 2;
            if (heightBlocks < 2)
                heightBlocks = 2;
            
            texData.dataSize[level]      = widthBlocks * heightBlocks * ((blockSize  * bpp) / 8);
            texData.pvrDataOffset[level] = dataOffset + sizeof(PVRTexHeader);
            texData.width   [level]      = width;
            texData.height  [level]      = height;
            dataOffset += texData.dataSize[level];
            width  = ((width  >> 1) > 1 ? (width  >> 1) : 1);
            height = ((height >> 1) > 1 ? (height >> 1) : 1);
            ++level;
        }
        while (level < kMaxTextureLevel) {
            texData.dataSize[level] = 0;
            texData.width[level] = 0;
            texData.height[level] = 0;
            texData.pvrDataOffset[level] = 0;
            ++level;
        }
        success = TRUE;
    }
    else {
        GTLog("Sorry, unknown PVR format\n");
    }
    
    return texData;
}
#endif

class FileOpenGlSurface : public OpenGlSurface {
    private:
        DataPathManager    &m_dataPathManager;
        std::string         m_path;
        ImageSpecialAbility m_specialAbility;
        bool                m_lowDef;

    public:
        FileOpenGlSurface(DataPathManager &dataPathManager,
                          ImageType type, const char *path, ImageSpecialAbility specialAbility, bool lowDef)
          : OpenGlSurface(type, path), m_dataPathManager(dataPathManager), m_path(path), m_specialAbility(specialAbility), m_lowDef(lowDef)
        {
#if USE_TEXTURES_16BITS
            if ((specialAbility & IMAGE_COMPRESSION_DISABLE_ALL) || !SHOULD_LOAD_TEXTURES_16BITS())
                m_useCompression = COMPRESSION_NONE;
            else if (specialAbility & IMAGE_COMPRESSION_DISABLE_ALPHA_1BIT)
                m_useCompression = COMPRESSION_RGBA4;
            else
                m_useCompression = COMPRESSION_RGB5A1;
#endif
        }

#if ENABLE_PVR_TEXTURES
    unsigned char *MY_load_pvr_image(std::auto_ptr<DataInputStream> file,
                                     int *width, int *height, int *channels,
                                     int force_channels) {
        m_useCompression = COMPRESSION_PVR;
        m_pvrData = loadPvrData(file);
        *width = m_pvrData.width[0];
        *height = m_pvrData.height[0];
        *channels = 4;
        return (unsigned char*)m_pvrData.buffer;
    }
#endif
        unsigned char *MY_load_image(std::auto_ptr<DataInputStream> file,
                                     int *width, int *height, int *channels,
                                     int force_channels) {
            BufferedStream   fileBuffer(*file);
            VoidBuffer buffer = fileBuffer.data();
            unsigned char *data = SOIL_load_image_from_memory(buffer, buffer.size(),
                                                              width, height, channels,
                                                              force_channels);
            return data;
        }

        virtual void _loadData() {

            m_w = m_h = m_p2h = m_p2w = 1;
            m_dataOK = true;
            
            if (!m_dataPathManager.hasDataInputStream(m_path.c_str())) {
                GTLogf("File not found: %s", m_path.c_str());
                return;
            }
            int force_channels = SOIL_LOAD_AUTO;
            if (m_type == IMAGE_RGB)
                force_channels = SOIL_LOAD_RGB;
            else
                force_channels = SOIL_LOAD_RGBA;

            // Uses 24/32 bits textures
            int channels, width, height;
            unsigned char *data = NULL;

            // Check is RGB+A file format is present.
            std::string rgbPath(m_path + "-rgb.jpg");
#if ENABLE_PVR_TEXTURES
            std::string pvrPath(m_path + ".pvr");
            if (m_dataPathManager.hasDataInputStream(pvrPath.c_str())) {
                // Load PVR
                data = MY_load_pvr_image(std::auto_ptr<DataInputStream>(m_dataPathManager.openDataInputStream(pvrPath.c_str())),
                                         &width, &height, &channels,
                                         force_channels);
                if (data == NULL) {
                    GTLogf("Error while loading %s", pvrPath.c_str());
                    return;
                }
                
                if (m_type == IMAGE_RGB) channels = 3;
                else                     channels = 4;
            }
            else
#endif
            if (m_dataPathManager.hasDataInputStream(rgbPath.c_str())) {

                GTLogf("File %s found: loading RGB+A", rgbPath.c_str());

                // Yes: load two images and merge.
                int aWidth, aHeight, aChannels, rgbChannels;
                unsigned char *rgbData = MY_load_image(std::auto_ptr<DataInputStream>(m_dataPathManager.openDataInputStream(rgbPath.c_str())),
                                                       &width, &height, &rgbChannels,
                                                       SOIL_LOAD_RGB);
                if (rgbData == NULL) {
                    GTLogf("Error while loading %s", rgbPath.c_str());
                    return;
                }
                unsigned char *aData = NULL;
                std::string aPath(m_path + "-a.png");
                if (m_dataPathManager.hasDataInputStream(aPath.c_str())) {
                    aData = MY_load_image(std::auto_ptr<DataInputStream>(m_dataPathManager.openDataInputStream(aPath.c_str())),
                                          &aWidth, &aHeight, &aChannels,
                                          SOIL_LOAD_L);
                }
                data = (unsigned char *)malloc(width * height * 4);
                if (aData == NULL) {
                    if (data == NULL || rgbChannels != 3) {
                        SOIL_free_image_data(rgbData);
                        if (data == NULL)
                            GTLogf("Error while loading %s: out of memory", m_path.c_str());
                        else
                            GTLogf("Error while loading %s: invalid RGB+A combo", m_path.c_str());
                        return;
                    }
                    int outPixel = 0;
                    int rgbPixel = 0;
                    for (int y=0; y<height; ++y) {
                        for (int x=0; x<width; ++x) {
                            data[outPixel + 0] = (int)rgbData[rgbPixel + 0];
                            data[outPixel + 1] = (int)rgbData[rgbPixel + 1];
                            data[outPixel + 2] = (int)rgbData[rgbPixel + 2];
                            data[outPixel + 3] = 255;
                            rgbPixel += 3;
                            outPixel += 4;
                        }
                    }
                }
                else {
                    if (data == NULL || aChannels != 1 || rgbChannels != 3) {
                        SOIL_free_image_data(rgbData);
                        SOIL_free_image_data(aData);
                        if (data == NULL)
                            GTLogf("Error while loading %s: out of memory", m_path.c_str());
                        else
                            GTLogf("Error while loading %s: invalid rgb-a combo", m_path.c_str());
                        return;
                    }
                    int outPixel = 0;
                    int rgbPixel = 0;
                    int aPixel   = 0;
                    for (int y=0; y<height; ++y) {
                        for (int x=0; x<width; ++x) {
                            data[outPixel + 0] = (int)rgbData[rgbPixel + 0];
                            data[outPixel + 1] = (int)rgbData[rgbPixel + 1];
                            data[outPixel + 2] = (int)rgbData[rgbPixel + 2];
                            data[outPixel + 3] = aData  [aPixel   + 0];
                            rgbPixel += 3;
                            outPixel += 4;
                            aPixel   += 1;
                        }
                    }
                }
                channels = 4;
                SOIL_free_image_data(rgbData);
                if (aData) SOIL_free_image_data(aData);
            }
            else if (m_dataPathManager.hasDataInputStream(m_path.c_str())) {
                data = MY_load_image(std::auto_ptr<DataInputStream>(m_dataPathManager.openDataInputStream(m_path.c_str())),
                                     &width, &height, &channels,
                                     force_channels);
                if (data == NULL) {
                    GTLogf("Error while loading %s", m_path.c_str());
                    return;
                }

                if (m_type == IMAGE_RGB) channels = 3;
                else                     channels = 4;
            }
            else {
                GTLogf("Error while loading %s", m_path.c_str());
                return;
            }
            
            m_data = data;
            m_w    = width;
            m_h    = height;
            m_p2w  = width;  // Because SOIL does some magic internally,
            m_p2h  = height; // no need to know that it's a Po2 texture.

            // Low-end devices get half-sized images.
            /*
            Disabled as we now generate half-sized images at asset compilation time.
            --------
            if (m_lowDef && width > 1 && height > 1) {
                int halfW = width  / 2;
                int halfH = height / 2;
                m_data = (GLubyte*)malloc(channels*halfW*halfH);
                int indexOut = 0;
                for (int y=0; y<halfH; ++y) {
                    int oldY = y * 2;
                    for (int x=0; x<halfW; ++x) {
                        int oldX = x * 2;
                        for (int c=0; c<channels; ++c)
                            m_data[indexOut++] = data[(oldY*width + oldX) * channels + c];
                    }
                }
                free(data);
                m_w = m_p2w = halfW;
                m_h = m_p2h = halfH;
            }
            */
        }

#if 0
        virtual void genOpenGlTexture(OpenGlSurface *surface) {
            SCOPED_GL_LOCK;
            // [EAGLContext setCurrentContext:gContext];
            gContext->setCurrent();
            // SOIL_free_image_data(data);
            OpenGlSurface *surf = new OpenGlSurface(toGL(type), width, height, texID, data/*, premultipliedAlpha*/);
            surf->setName(fullPath.c_str());
            IosSurface *result = new OpenGlSurfaceRef(surf);
            if (!(specialAbility & IMAGE_READ))
                surf->decDataRef(); // We do not need the buffer.	

            result->name = fullPath;
            return result;
        }
#endif
};

void OpenGlSurfaceLibrary::unrefGlObjects() {
    std::list<OpenGlSurface*>::iterator it = m_surfaceLib.begin();
    while (it != m_surfaceLib.end()) {
        (*it)->unrefGlObjects();
        ++it;
    }
}

void OpenGlSurfaceLibrary::freeGlObjects() {
    std::list<OpenGlSurface*>::iterator it = m_surfaceLib.begin();
    while (it != m_surfaceLib.end()) {
        (*it)->freeGlObjects();
        ++it;
    }
}

static void fixRects(IosRect *srcRect, IosRect *dstRect, const IosSurface *surf, const DrawTarget *targ, IosRect **pSrcRect, IosRect **pDstRect)
{
	static IosRect cSrcRect(0,0,1,1);
	static IosRect cDstRect(0,0,1,1);
    if (srcRect != NULL)
        *pSrcRect = srcRect;
    else {
        cSrcRect.w = surf->w;
        cSrcRect.h = surf->h;
        *pSrcRect = &cSrcRect;
    }
    if (dstRect != NULL)
        *pDstRect = dstRect;
    else {
        cDstRect.w = targ->w;
        cDstRect.h = targ->h;
        *pDstRect = &cDstRect;
    }
}

static bool isOutside(float w, float h, IosRect *r) {
	if (GTGetScreenOrientation() == GT_LANDSCAPE) {
		// Swap w and h.
		float t = w; w = h; h = t;
	}
	if ((r->frameY == 0 && r->frameX == 1) || (r->frameY == 0 && r->frameX == -1)) {
		// Basic case
		float xmin,xmax,ymin,ymax;
		if (r->w > 0) {
			xmin = r->x; xmax = r->x + r->w;
		}
		else {
			xmin = r->x + r->w; xmax = r->x;
		}
		if (r->h > 0) {
			ymin = r->y; ymax = r->y + r->h;
		}
		else {
			ymin = r->y + r->h; ymax = r->y;
		}
		if (xmax < 0.0f) return true;
		if (ymax < 0.0f) return true;
		if (xmin > w) return true;
		if (ymin > h) return true;
	}
	else {
		// General case... make it as it can be the worst.
		float s = (fabs(r->w) > fabs(r->h) ? r->w : r->h);
		// Depends on the angle, let's take the worst case: 45 degrees
		if (s > 0) {
			if (r->x + s * 1.41f < 0.0f) return true;
			if (r->y + s * 1.41f < 0.0f) return true;
			if (r->x - s * 0.41f > w) return true;
			if (r->y - s * 0.41f > h) return true;
		}
		else {
			if (r->x - s * 0.41f < 0.0f) return true;
			if (r->y - s * 0.41f < 0.0f) return true;
			if (r->x + s * 1.41f > w) return true;
			if (r->y + s * 1.41f > h) return true;
		}
	}
	return false;
}

static void initVertices(GLfloat *vertices, const IosRect *pDstRect/*, float sx, float sy*/) {
	if (/*((pDstRect->frameY == 0) && (pDstRect->frameX == 1))
		|| */ (fabs(pDstRect->frameY) < 0.001f && pDstRect->frameX > 0.99f)) {
		float w = pDstRect->w/**sx*/;
		float h = pDstRect->h/**sy*/;
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
		float w = pDstRect->w/**sx*/;
		float h = pDstRect->h/**sy*/;
		// u (w,0)
		float ux =    w *  pDstRect->frameX /* +    0 * pDstRect->frameY*/;
		float uy =    w * -pDstRect->frameY /* +    0 * pDstRect->frameX*/;
		// v (0,h)
		float vx = /* 0 *  pDstRect->frameX    + */ h * pDstRect->frameY;
		float vy = /* 0 * -pDstRect->frameY    + */ h * pDstRect->frameX;
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
		/*printf("    x': %.3f\n", vertices[2]);
		printf("    y': %.3f\n", vertices[5]);*/
	}
}


class OpenGlSurfaceRef : public IosSurface
{
public:
	OpenGlSurface *m_ref;
	ImageBlendMode m_mode;
    int            m_wrap_s, m_wrap_t;
    bool           m_mipmap;
	bool           gray, hflip;
    bool           m_needParametersUpdate;
	
	OpenGlSurfaceRef(OpenGlSurface *s) : m_ref(s) {
		this->h = m_ref->m_h;
		this->w = m_ref->m_w;
		m_ref->incRef();
		m_mode = IMAGE_BLEND;
        m_wrap_s = GL_CLAMP_TO_EDGE;
        m_wrap_t = GL_CLAMP_TO_EDGE;
        m_mipmap = true;
		gray = false;
		hflip = false;
        m_needParametersUpdate = true;
	}

	OpenGlSurfaceRef(OpenGlSurfaceRef *s) : m_ref(s->m_ref) {
		this->h = m_ref->m_h;
		this->w = m_ref->m_w;
		m_ref->incRef();
		copyDrawMode(s);
	}

	~OpenGlSurfaceRef() {
		m_ref->decRef();
		if (m_ref->noRef()) delete m_ref;
	}

	virtual bool isOpaque() const { return m_ref->m_opaque; }

	virtual bool haveAbility(ImageSpecialAbility a) const {
		if (a == IMAGE_READ)
			return m_ref->m_data != NULL;
		return false;
	}
	virtual void dropAbility(ImageSpecialAbility a) {
		if (a == IMAGE_READ)
			m_ref->decDataRef();
	}
    virtual RGBA readRGBA(int x, int y) {
		RGBA c;
		if (m_ref->m_type== IMAGE_RGB) {
			GLubyte *base = m_ref->m_data + (x + y * m_ref->m_p2w) * 3;
			c.red = base[0];
			c.green = base[1];
			c.blue = base[2];
			c.alpha = 255;
		}
		else if (m_ref->m_type == IMAGE_RGBA) {
			GLubyte *base = m_ref->m_data + (x + y * m_ref->m_p2w) * 3;
			c.red = base[1];
			c.green = base[2];
			c.blue = base[3];
			c.alpha = base[0]; // XXX, WHY THE HELL IS THE BUFFER IN ARGB? but it works... on my simulator...
		}
		return c;
	}
	
	void copyDrawMode(OpenGlSurfaceRef *ref) {
		this->hflip = ref->hflip;
		this->m_mode = ref->m_mode;
        this->m_wrap_s = ref->m_wrap_s;
        this->m_wrap_t = ref->m_wrap_t;
        this->m_mipmap = ref->m_mipmap;
		this->gray = ref->gray;
        this->m_needParametersUpdate = true;
	}
	
    virtual IosSurface *shiftHue(float hue_offset, IosSurface *mask = NULL) {
        // TODO
        /*
        SCOPED_GL_LOCK;
        if ((m_ref->m_data != NULL) && (m_ref->m_type == IMAGE_RGBA) && (hue_offset != 0)) {
            GLubyte *newData;
            if (mask == NULL)
                newData = image_rgba_shift_hue(m_ref->m_data, m_ref->m_p2w, m_ref->m_p2h, hue_offset);
            else {
                OpenGlSurfaceRef *maskSurf = static_cast<OpenGlSurfaceRef*>(mask);
                newData = image_rgba_shift_hue_masked(m_ref->m_data, m_ref->m_p2w, m_ref->m_p2h,
                                                      maskSurf->m_ref->m_data, maskSurf->m_ref->m_p2w, maskSurf->m_ref->m_p2h,hue_offset);
            }
            // OpenGlSurface* retSurf = new OpenGlSurface(m_ref->m_format, m_ref->m_w, m_ref->m_h, m_ref->m_p2w, m_ref->m_p2h, newData);
            OpenGlSurface* retSurf = new DataOpenGlSurface(m_ref->m_type, "shiftHue", m_ref->m_w, m_ref->m_h, m_ref->m_p2w, m_ref->m_p2h, newData);
            retSurf->decDataRef();
            OpenGlSurfaceRef *ret = new OpenGlSurfaceRef(retSurf);
			ret->copyDrawMode(this);
			return ret;
        }
        */
		return new OpenGlSurfaceRef(this);
	}
	
    virtual IosSurface *shiftHSV(float h, float s, float v) {
		return new OpenGlSurfaceRef(this); // TODO
	}
	
    virtual IosSurface *setValue(float value) {
        SCOPED_GL_LOCK;
        /*
        if ((m_ref->m_data != NULL) && (m_ref->m_format == GL_RGBA) && (value != 0)) {
            GLubyte *newData = image_rgba_shift_hsv(m_ref->m_data, m_ref->m_p2w, m_ref->m_p2h, 0, 0, value);
            // OpenGlSurface* retSurf = new OpenGlSurface(m_ref->m_format, m_ref->m_w, m_ref->m_h, m_ref->m_p2w, m_ref->m_p2h, newData);
            OpenGlSurface* retSurf = new DataOpenGlSurface(m_ref->m_type, "setValue", m_ref->m_w, m_ref->m_h, m_ref->m_p2w, m_ref->m_p2h, newData);
            // retSurf->decDataRef();
            OpenGlSurfaceRef *ret = new OpenGlSurfaceRef(retSurf);
			ret->copyDrawMode(this);
			return ret;
        }
        */
		return new OpenGlSurfaceRef(this);
	}
	
    virtual IosSurface * resizeAlpha(int width, int height) {
		return new OpenGlSurfaceRef(this); // TODO... easy fix with drawToGl supporting scaling...
	}
	
    virtual IosSurface * mirrorH() {
		OpenGlSurfaceRef *ret = new OpenGlSurfaceRef(this);
		ret->hflip = !ret->hflip;
		return ret;
	}
	
    virtual void convertToGray() {
		gray = true;
	}

	// Draw Target Implementation
	
    virtual void setClipRect(IosRect *rect) {
	}
    
    virtual void setBlendMode(ImageBlendMode mode) {
        // printf("OpenGlSurfaceRef::setBlendMode(%d)\n", (int)mode);
		m_mode = mode;
    }
    virtual void setWrapMode(ImageWrapMode modeS, ImageWrapMode modeT) {
        if (modeS == IMAGE_CLAMP) {
            if (m_wrap_s != GL_CLAMP_TO_EDGE) {
                m_wrap_s = GL_CLAMP_TO_EDGE;
                m_needParametersUpdate = true;
            }
        }
        else {
            if (m_wrap_s != GL_REPEAT) {
                m_wrap_s = GL_REPEAT;
                m_needParametersUpdate = true;
            }
        }
        
        if (modeT == IMAGE_CLAMP) {
            if (m_wrap_t != GL_CLAMP_TO_EDGE) {
                m_wrap_t = GL_CLAMP_TO_EDGE;
                m_needParametersUpdate = true;
            }
        }
        else {
            if (m_wrap_t != GL_REPEAT) {
                m_wrap_t = GL_REPEAT;
                m_needParametersUpdate = true;
            }
        }
    }
    
    virtual void setMipmapEnable(bool state) {
        if (m_mipmap != state) {
            m_mipmap = state;
            m_needParametersUpdate = true;
        }
    }

    GLint m_savedViewport[4];
    virtual void clear(float r, float g, float b, float a) {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
        glMatrixMode(GL_MODELVIEW); GL_GET_ERROR();
        glPushMatrix(); GL_GET_ERROR();
        glLoadIdentity(); GL_GET_ERROR();
        glOrthof(0.0, (GLfloat)this->w, 0.0, (GLfloat) this->h, 0.0, 1.0);
        glClearColor(r, g, b, a); GL_GET_ERROR();
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); GL_GET_ERROR();
        m_alpha = m_red = m_green = m_blue = 1.0f;
        glGetIntegerv(GL_VIEWPORT, m_savedViewport);
        glViewport(0, 0, this->w, this->h);
        
        setDefaultStates();
    }

    virtual void flip() {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glViewport(m_savedViewport[0], m_savedViewport[1], m_savedViewport[2], m_savedViewport[3]);
    }
    
    float m_alpha;
	float m_red, m_green, m_blue;
    
    virtual void nextDrawAlpha(float alpha) {  m_alpha = alpha; }
	virtual void nextDrawColor(float r, float g, float b) {
        m_red   = r > 0 ? r : 0;
        m_green = g > 0 ? g : 0;
        m_blue  = b > 0 ? b : 0;
    }
    
    void applyAlpha() {
        if (m_alpha < 1.0f) {
            glColor4f(m_red, m_green, m_blue, m_alpha); GL_GET_ERROR();
        }
        else if (m_red + m_green + m_blue < 2.95f) {
            glColor4f(m_red, m_green, m_blue, 1.0f); GL_GET_ERROR();
        }
    }
    
    void resetAlpha() {
        if (m_alpha < 1.0f) {
            m_alpha = m_red = m_green = m_blue = 1.0f;
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f); GL_GET_ERROR();
        }
        else if (m_red + m_green + m_blue < 2.95f) {
            m_red = m_green = m_blue = 1.0f;
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f); GL_GET_ERROR();
        }
    }

    virtual void draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect) {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
		IosRect *pSrcRect, *pDstRect;
		fixRects(srcRect, dstRect, surf, this, &pSrcRect, &pDstRect);
		OpenGlSurfaceRef *ipSurf = static_cast<OpenGlSurfaceRef*>(surf);
        if (ipSurf) {
            ipSurf->applyBlendMode();
            applyAlpha();
            ipSurf->drawToGL(pSrcRect, pDstRect, this->h);
            resetAlpha();
            ipSurf->unapplyBlendMode();
        }
    }

    virtual void fillRect(const IosRect *rect, const RGBA &color) {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
		glColor4f(color.red/255.0f, color.green/255.0f, color.blue/255.0f, color.alpha/255.0f);
		GLfloat vertices[8];
		GLushort faces[4] = {0,1,2,3};
		initVertices(vertices, rect/*, 1, 1*/);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY); 
		glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faces[0]);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
    virtual void putString(IosFont *font, int x, int y, const char *text, float dx, float dy) {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
		OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
		ifont->print(x,y,text,1.0f,'l',dx,dy);
	}
    virtual void putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy) {
        SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
		OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
		ifont->print(x,y,text,1.0f,'r',dx,dy);
	}
	virtual void putStringWithShadow(IosFont *font, int x, int y, int shadow_x, int shadow_y, const char *text, float dx, float dy) {
	    SCOPED_GL_LOCK;
        // [EAGLContext setCurrentContext:gContext];
        gContext->setCurrent();
		m_ref->bindFBO();
		OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
		ifont->printWithShadow(x,y,shadow_x,shadow_y,text,1.0f,'l',dx,dy);
	}
	
    virtual void setData(int x, int y, int w, int h, const char *data)
    {
        SCOPED_GL_LOCK;
        m_ref->bindTexture();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                     toGL(m_ref->m_type),
                     GL_UNSIGNED_BYTE,
                     data);
        GL_GET_ERROR();
    }
    
	//
	// OpenGL code
	//
	
	virtual void bindFBO() {
		m_ref->bindFBO();
	}

	void applyBlendMode() {
		m_ref->bindTexture();
		if (m_ref->m_opaque) {
            if (m_mode == IMAGE_ADD) {
                // [default] glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);
                // [default] glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
            }
            else if (m_mode == IMAGE_MULTIPLY) {
                // [default] glEnable(GL_BLEND);
                glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
                // [default] glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
            }
            else {
                glDisable(GL_BLEND);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); GL_GET_ERROR();
            }
            
		}
        else switch (m_mode) {
			case IMAGE_BLEND:
                // This is default blend mode:
				// [default] glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
				// [default] glEnable(GL_BLEND); GL_GET_ERROR();
                // [default] glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
                // [default] glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); */

                /*if (m_ref->m_premultipliedAlpha) {
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
                    glBlendFuncSeparateOES(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                }
                else {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
                    glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    // JEKO COMMENTED OUT 3/8/2012.
                }*/
                return;
			case IMAGE_COPY:
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); GL_GET_ERROR();
				glDisable(GL_BLEND);
				return;
			case IMAGE_ADD:
				// [default] glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
				// [default] glEnable(GL_BLEND); GL_GET_ERROR();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); GL_GET_ERROR();
                /*if (m_ref->m_premultipliedAlpha) {
                    glBlendFunc(GL_ONE, GL_ONE); GL_GET_ERROR();
                }
                else {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); GL_GET_ERROR();
                // JEKO ADDED 3/8/2012. glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                }*/
				return;
            case IMAGE_MULTIPLY:
                // [default] glEnable(GL_BLEND);
                glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
                // [default] glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
                return;
		}
	}
    
    void unapplyBlendMode() {
        if (m_ref->m_opaque || m_mode != IMAGE_BLEND) {
            // Restore default blend mode.
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GL_GET_ERROR();
            glEnable(GL_BLEND); GL_GET_ERROR();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
            glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
	
	void drawToGL(IosRect *pSrcRect, IosRect *pDstRect, int targetHeight) {
        if (m_needParametersUpdate) {
#if ENABLE_MIPMAPS
            if (m_ref->m_data == NULL || !m_mipmap)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_FILTER);
            else
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_MIPMAP_FILTER); // GL_LINEAR_MIPMAP_NEAREST
#else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_FILTER);
#endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, USE_GL_FILTER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrap_s);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrap_t);
            m_needParametersUpdate = false;
        }
        GL_GET_ERROR();
        
        GLfloat texcoord[8];
        GLfloat vertices[8];
        GLushort faces[4] = {0,1,2,3};
        texcoord[0] = (GLfloat)(pSrcRect->x) / m_ref->p2w();             texcoord[1] = (GLfloat)(pSrcRect->y) / m_ref->p2h();
        texcoord[2] = (GLfloat)(pSrcRect->x+pSrcRect->w) / m_ref->p2w(); texcoord[3] = (GLfloat)(pSrcRect->y) / m_ref->p2h();
        texcoord[4] = (GLfloat)(pSrcRect->x) / m_ref->p2w();             texcoord[5] = (GLfloat)(pSrcRect->y+pSrcRect->h) / m_ref->p2h();
        texcoord[6] = (GLfloat)(pSrcRect->x+pSrcRect->w) / m_ref->p2w(); texcoord[7] = (GLfloat)(pSrcRect->y+pSrcRect->h) / m_ref->p2h();
        initVertices(vertices, pDstRect/*, sx, sy*/);
        
#if IMAGE_QUALITY_BENCHMARK
        float scaleFactor = pDstRect->w / pSrcRect->w;
        if (scaleFactor < 0.0f) scaleFactor = -scaleFactor;
        if (scaleFactor > m_ref->m_highestFactor)
            m_ref->m_highestFactor = scaleFactor;
        if (scaleFactor < m_ref->m_lowestFactor)
            m_ref->m_lowestFactor = scaleFactor;
#endif

        /*if (m_ref->m_name == "gfx/ali-ui-map.jpg") {
            GTLogf("Drawing MAP: p2w=%d, p2h=%d, pSrcRect.x=%.0f, pSrcRect.w=%.0f", m_ref->p2w(), m_ref->p2h(), pSrcRect->x, pSrcRect->w);
            GTLogf("Texcoord = (%.2f,%.2f), (%.2f,%.2f), (%.2f,%.2f), (%.2f,%.2f)",
                   texcoord[0], texcoord[1], texcoord[2], texcoord[3], 
                   texcoord[4], texcoord[5], texcoord[6], texcoord[7]);
            GTLogf("Vertices = (%.2f,%.2f), (%.2f,%.2f), (%.2f,%.2f), (%.2f,%.2f)",
                   vertices[0], vertices[1], vertices[2], vertices[3], 
                   vertices[4], vertices[5], vertices[6], vertices[7]);
        }*/
        
        glTexCoordPointer(2, GL_FLOAT, 0, &texcoord[0]);
        GL_GET_ERROR();
        glEnableClientState(GL_TEXTURE_COORD_ARRAY); 
        GL_GET_ERROR();
        glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
        GL_GET_ERROR();
        glEnableClientState(GL_VERTEX_ARRAY);
        GL_GET_ERROR();
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faces[0]);
        GL_GET_ERROR();
		m_ref->unbindTexture();
	}
};

IosSurface * OpenGlImageLibrary::createImage(ImageType type, int w, int h, ImageSpecialAbility specialAbility)
{
    SCOPED_GL_LOCK;
    // [EAGLContext setCurrentContext:gContext];
    gContext->setCurrent();
	OpenGlSurface *s;
	if (specialAbility & IMAGE_READ) {
#if REQUIRE_POWER_OF_TWO
		int p2width  = power_of_2(w);
		int p2height = power_of_2(h);
#else
		int p2width = w;
		int p2height = h;
#endif
		GLubyte *data = (GLubyte*)calloc(1,toBpp(type)*p2width*p2height);
		s = new DataOpenGlSurface(type, "Memory Buffer", w, h, p2width, p2height, data);
	}
	else {
		s = new PureOpenGlSurface(type, "PureGL Buffer", w, h);
	}
    s->loadData();   // Increase dataRef by 1
    s->genTexture();
    s->decDataRef();
	return new OpenGlSurfaceRef(s);
}

IosSurface * OpenGlImageLibrary::loadImage(ImageType type, const char *path, ImageSpecialAbility specialAbility)
{
    if (!m_dataPathManager.hasDataInputStream(path)) {
        GTLogf("ERROR: Image %s does not exist!", path);
        return NULL;
    }
    bool lowDef = (GTGetPlatformPerfo() > 0.0f && GTGetPlatformPerfo() < 0.25f) || (m_oglDrawContext.getWidth() + m_oglDrawContext.getHeight() < 400 + 300);
    OpenGlSurface *surf = new FileOpenGlSurface(m_dataPathManager, type, path, specialAbility, lowDef);
    surf->loadData();
    SCOPED_GL_LOCK;
    gContext->setCurrent();
    surf->genTexture();
    surf->decDataRef();
    IosSurface *result  = new OpenGlSurfaceRef(surf);
    if (specialAbility & IMAGE_READ) surf->incDataRef();
    return result;
}


IosFont * OpenGlImageLibrary::createFont(const char *path, float size, const IosFontFx &fx)
{
    GTLogf("createFont(%s)", path);
    return new OpenGlIosFont(m_dataPathManager, path, size * GTGetScaleFactor(), fx);
}

OpenGlDrawContext::OpenGlDrawContext(DataPathManager &dataPathManager, int width, int height, OpenGlContext *context)
  : iimLib(dataPathManager, *this), context(context)/*, m_blending(BLEND_NORMAL)*/
  , defaultFramebuffer(0), colorRenderbuffer(0)
{
	name = "OpenGlDrawContext";
    glMutex = new ios_fc::Mutex();
    SCOPED_GL_LOCK;
	
	this->w = width;
    this->h = height;
    this->m_isRotated = false;
    gContext = context; // ugly hack
}

void OpenGlDrawContext::init() {
    // Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
#if REQUIRE_DEFAULT_RENDERBUFFER
    if (defaultFramebuffer == 0 && colorRenderbuffer == 0) {
        glGenFramebuffersOES(1,  &defaultFramebuffer); GL_GET_ERROR();
        glGenRenderbuffersOES(1, &colorRenderbuffer); GL_GET_ERROR();
    }
    // glRenderbufferStorage(GL_RENDERBUFFER_OES, GL_EXT_color_buffer_half_float, this->w, this->h);
    iglBindFramebufferOES(defaultFramebuffer, NULL); GL_GET_ERROR();
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer); GL_GET_ERROR();
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer); GL_GET_ERROR();
    gScreenFBO = defaultFramebuffer;
#else
    gScreenFBO = defaultFramebuffer = 0;
#endif
    // resize(this->w, this->h, NULL);
}

/*
void OpenGlDrawContext::init() {
	if (getWidth() + getHeight() >= 1300)
		GTSetScaleFactor(2.0f); // Gameui will double its pixels
	if (getWidth() + getHeight() >= 3500) // iPad 3 - retina display.
		GTSetScaleFactor(4.0f); // Gameui will quadruple its pixels

	if (getWidth() + getHeight() == 640 + 960) {
		// Retina display detected
        NSString *platform = [[[UIDevice currentDevice] machine] retain];
        GTLog("Platform: %@", platform);
        if ([platform isEqualToString:@"iPhone3,1"]) GTSetPlatform(GT_IPHONE4);
        else if ([platform isEqualToString:@"iPhone4,1"]) GTSetPlatform(GT_IPHONE4S);
        else GTSetPlatform(GT_IPHONE4);
        GTSetPhysicalSize(0.089f);
	}
	else if (getWidth() + getHeight() == 768 + 1024) {
		// iPad detected
        NSString *platform = [[[UIDevice currentDevice] machine] retain];
        NSString *subplatform = @"";
        if ([platform length] > 6)
            subplatform = [[platform substringToIndex:6] retain];
        GTLog("Platform: %@", subplatform);
        if ([subplatform isEqualToString:@"iPad1"]) GTSetPlatform(GT_IPAD1);
        else if ([subplatform isEqualToString:@"iPad2"]) GTSetPlatform(GT_IPAD2);
        else GTSetPlatform(GT_IPAD2); // Newer model, assume at least as good as iPad 2
        [platform release];
        if ([subplatform length] > 0)
            [subplatform release];
        GTSetPhysicalSize(0.254f);
	}
	else if (getWidth() + getHeight() == 320 + 480) {
        // iPhone 3 / iPod
        NSString *platform = [[[UIDevice currentDevice] machine] retain];
        GTLog("Platform: %@", platform);
        if ([platform isEqualToString:@"iPhone1,1"]) GTSetPlatform(GT_IPHONE1G);
        else if ([platform isEqualToString:@"iPhone1,2"]) GTSetPlatform(GT_IPHONE3G);
        else if ([platform isEqualToString:@"iPhone2,1"]) GTSetPlatform(GT_IPHONE3GS);
        else if ([platform isEqualToString:@"iPod1,1"]) GTSetPlatform(GT_IPHONE1G);
        else if ([platform isEqualToString:@"iPod2,1"]) GTSetPlatform(GT_IPHONE3G);
        else if ([platform isEqualToString:@"iPod3,1"]) GTSetPlatform(GT_IPHONE3GS);
        else GTSetPlatform(GT_IPHONE3GS); // It's a newer model, assume perfs are good.
        [platform release];
        GTSetPhysicalSize(0.089f);
	}
    else if (getWidth() + getHeight() == 2048 + 1536) {
        GTSetPlatform(GT_IPAD3);
        GTSetPhysicalSize(0.254f);
    }
    else {
        // Newer model, assume perfs at least those of an iPad 2.
        GTSetPlatform(GT_IPAD2);
        // And assume retina display.
        GTSetPhysicalSize(0.089f * (getWidth() + getHeight()) / (640.0f + 960.0f));
    }
	GTSetPlatformVersion(atof([[[UIDevice currentDevice] systemVersion] cStringUsingEncoding:NSUTF8StringEncoding]));

    // Create OpenGLES context
    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    if (!context || ![EAGLContext setCurrentContext:context])
    {
        // TODO: traiter erreur
    }
}
*/

void OpenGlDrawContext::setVFlip(bool vflip) {
    if (vflip) {
        if (GTGetScreenOrientation() == GT_PORTRAIT) {
            glLoadIdentity();
            glViewport(0, 0, this->w, this->h);
            glOrthof((GLfloat)this->w, 0.0f, 0.0f, (GLfloat) this->h, 0.0f, 1.0f);
        }
        else {
            GLfloat matrix[] = {
                0,1,0,0,
                -1,0,0,0,
                0,0,1,0,
                0,0,0,1
            };
            glLoadMatrixf(matrix);
            glViewport(0, 0, this->w, this->h);
            glOrthof((GLfloat)this->h, 0.0f, 0.0f, (GLfloat) this->w, 0.0, 1.0);
        }
    }
    else {
        if (GTGetScreenOrientation() == GT_PORTRAIT) {
            glLoadIdentity();
            glViewport(0, 0, this->w, this->h);
            glOrthof(0.0f, (GLfloat)this->w, (GLfloat) this->h, 0.0f, 0.0, 1.0);
        }
        else {
            GLfloat matrix[] = {
                0,1,0,0,
                -1,0,0,0,
                0,0,1,0,
                0,0,0,1
            };
            glLoadMatrixf(matrix);
            glViewport(0, 0, this->w, this->h);
            glOrthof(0.0f, (GLfloat)this->h, (GLfloat) this->w, 0.0f, 0.0, 1.0);
        }
    }
    GL_GET_ERROR();
    glGetFloatv(GL_MODELVIEW_MATRIX, &this->matrix[0]);
}

void OpenGlDrawContext::clear(float r, float g, float b, float a) {
    SCOPED_GL_LOCK;
#if USE_DEFERRED_DRAW
    performDeferredDraw();
#endif
    // [EAGLContext setCurrentContext:gContext];
    gContext->setCurrent();
	this->bindFBO();
    glClearColor(r, g, b, a); GL_GET_ERROR();
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); GL_GET_ERROR();

    setDefaultStates();
}

void OpenGlDrawContext::flip()
{
    SCOPED_GL_LOCK;
    context->setCurrent();
    iglBindFramebufferOES(defaultFramebuffer, matrix); GL_GET_ERROR();
	BENCH.draw();
#if REQUIRE_DEFAULT_RENDERBUFFER
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer); GL_GET_ERROR();
#endif
    context->presentRenderbuffer(GL_RENDERBUFFER_OES);
}

void OpenGlDrawContext::bindFBO() {
	iglBindFramebufferOES(defaultFramebuffer, matrix);
}

int OpenGlDrawContext::getHeight() const {
    return this->h; // (GTGetScreenOrientation() == GT_PORTRAIT ? this->h : this->w);
}

int OpenGlDrawContext::getWidth() const {
    return this->w; // (GTGetScreenOrientation() == GT_PORTRAIT ? this->w : this->h);
}

ImageLibrary & OpenGlDrawContext::getImageLibrary() {
    return iimLib;
}

void OpenGlDrawContext::applyAlpha() {
	if (m_alpha < 1.0f) {
		glColor4f(m_red, m_green, m_blue, m_alpha); GL_GET_ERROR();
	}
	else if (m_red + m_green + m_blue < 2.95f) {
		glColor4f(m_red, m_green, m_blue, 1.0f); GL_GET_ERROR();
	}
}

void OpenGlDrawContext::resetAlpha() {
	if (m_alpha < 1.0f) {
		m_alpha = 1.0f;
		m_red = m_green = m_blue = 1.0f;
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f); GL_GET_ERROR();
	}
	else if (m_red + m_green + m_blue < 2.95f) {
		m_red = m_green = m_blue = 1.0f;
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f); GL_GET_ERROR();
	}
}

// #define USE_DEFERRED_DRAW 1

#if USE_DEFERRED_DRAW
static OpenGlDrawContext *g_defIDC = NULL;
static EAGLContext *g_defContext = NULL;
static int       g_defNumImages = 0;
static OpenGlSurfaceRef *g_defIpSurf = NULL;
static float g_defAlpha;
static float g_defRed;
static float g_defGreen;
static float g_defBlue;

#define MAX_DO_DRAW_IMAGES 32
static float g_vertices [MAX_DO_DRAW_IMAGES * 4 * 2];
static float g_texcoords[MAX_DO_DRAW_IMAGES * 4 * 2];
static GLushort  g_faces[MAX_DO_DRAW_IMAGES * 6] = {
	0,1,2,1,2,3,
	4,5,6,5,6,7,
	8,9,10,9,10,11,
	12,13,14,13,14,15,
	16,17,18,17,18,19,
	20,21,22,21,22,23,
	24,25,26,25,26,27,
	28,29,30,29,30,31,
	32,33,34,33,34,35,
	36,37,38,37,38,39,
	40,41,42,41,42,43,
	44,45,46,45,46,47,
	48,49,50,49,50,51,
	52,53,54,53,54,55,
	56,57,58,57,58,59,
	60,61,62,61,62,63,
	64,65,66,65,66,67,
	68,69,70,69,70,71,
	72,73,74,73,74,75,
	76,77,78,77,78,79,
	80,81,82,81,82,83,
	84,85,86,85,86,87,
	88,89,90,89,90,91,
	92,93,94,93,94,95,
	96,97,98,97,98,99,
	100,101,102,101,102,103,
	104,105,106,105,106,107,
	108,109,110,109,110,111,
	112,113,114,113,114,115,
	116,117,118,117,118,119,
	120,121,122,121,122,123,
	124,125,126,125,126,127
};
#endif

static void performDeferredDraw() {
#if USE_DEFERRED_DRAW
    if ((g_defNumImages > 0) && (g_defIpSurf != NULL)) {
        
        if (g_currentContext != g_defContext) {
            // [EAGLContext setCurrentContext:g_defContext];
            g_defContext->setCurrent();
            g_defIDC->bindFBO();
            g_currentContext = g_defContext;
        }
        OpenGlSurfaceRef *ipSurf = g_defIpSurf;
        if (g_defIpSurf) {
            g_defIpSurf->applyBlendMode(ipSurf->m_mode);
            //if ((g_defAlpha < 1.0f) || (g_defRed + g_defGreen + g_defBlue < 2.95f))
                glColor4f(g_defRed, g_defGreen, g_defBlue, g_defAlpha);
            
            g_defIpSurf->m_ref->bindTexture();
            // glBindTexture(GL_TEXTURE_2D,  m_texture);
            switch (g_defIpSurf->m_mode) {
                case IMAGE_COPY:
                    glBlendFunc(GL_ONE, GL_ZERO);
                    break;
                case IMAGE_BLEND:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case IMAGE_ADD:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    break;
            }
            
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, USE_GL_FILTER);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, USE_GL_FILTER);
            glTexCoordPointer(2, GL_FLOAT, 0, &g_texcoords[0]);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY); 
            glVertexPointer(2, GL_FLOAT, 0, &g_vertices[0]);
            glEnableClientState(GL_VERTEX_ARRAY);
            glDrawElements(GL_TRIANGLES, g_defNumImages * 6, GL_UNSIGNED_SHORT, &g_faces[0]);

            //if ((g_defAlpha < 1.0f) || (g_defRed + g_defGreen + g_defBlue < 2.95f))
            //    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        g_defIpSurf = NULL;
        g_defNumImages = 0;
	}
#endif
}

void OpenGlDrawContext::draw(IosSurface *surf, IosRect *srcRect, IosRect *dstRect)
{
    SCOPED_GL_LOCK;
	IosRect *pSrcRect, *pDstRect;
	fixRects(srcRect, dstRect, surf, this, &pSrcRect, &pDstRect);
#if USE_DEFERRED_DRAW
	OpenGlSurfaceRef *ipSurf = static_cast<OpenGlSurfaceRef *> (surf);

    if ((g_defIpSurf != ipSurf) || (g_defRed != m_red) || (g_defGreen != m_green) || (g_defBlue != m_blue) || (g_defAlpha != m_alpha)) {
		performDeferredDraw();
	}
    
	// Push the Image to the list.
	if (g_defNumImages == 0) {
        g_defContext = context;
        g_defIpSurf  = ipSurf;
		g_defRed = m_red;
		g_defGreen = m_green;
		g_defBlue = m_blue;
        g_defAlpha = m_alpha;
        g_defIDC = this;
	}

    GLfloat texcoord[8];
    GLfloat vertices[8];
    texcoord[0] = (GLfloat)(pSrcRect->x) / ipSurf->m_ref->p2w();             texcoord[1] = (GLfloat)(pSrcRect->y) / ipSurf->m_ref->p2h();
    texcoord[2] = (GLfloat)(pSrcRect->x+pSrcRect->w) / ipSurf->m_ref->p2w(); texcoord[3] = (GLfloat)(pSrcRect->y) / ipSurf->m_ref->p2h();
    texcoord[4] = (GLfloat)(pSrcRect->x) / ipSurf->m_ref->p2w();             texcoord[5] = (GLfloat)(pSrcRect->y+pSrcRect->h) / ipSurf->m_ref->p2h();
    texcoord[6] = (GLfloat)(pSrcRect->x+pSrcRect->w) / ipSurf->m_ref->p2w(); texcoord[7] = (GLfloat)(pSrcRect->y+pSrcRect->h) / ipSurf->m_ref->p2h();
    initVertices(vertices, pDstRect);
    memcpy(&g_vertices[g_defNumImages * 8],  &vertices[0], 8*sizeof(float));
	memcpy(&g_texcoords[g_defNumImages * 8], &texcoord[0], 8*sizeof(float));
    
    ++g_defNumImages;
	if (g_defNumImages == MAX_DO_DRAW_IMAGES)
		performDeferredDraw();
	m_alpha = m_red = m_green = m_blue = 1.0f;
#else
    // [EAGLContext setCurrentContext:context];
    context->setCurrent();
    this->bindFBO();
	OpenGlSurfaceRef *ipSurf = static_cast<OpenGlSurfaceRef *> (surf);
	if (ipSurf) {
		ipSurf->applyBlendMode();
		applyAlpha();
		ipSurf->drawToGL(pSrcRect, pDstRect, this->h);
		resetAlpha();
        ipSurf->unapplyBlendMode();
	}
#endif
}

void OpenGlDrawContext::setClipRect(IosRect *rect) {
}

void OpenGlDrawContext::fillRect(const IosRect *rect, const RGBA &color) {
	SCOPED_GL_LOCK;
    performDeferredDraw();
    
	// [EAGLContext setCurrentContext:gContext];
    gContext->setCurrent();
	this->bindFBO();
	/*
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_GET_ERROR();
	glEnable(GL_BLEND); GL_GET_ERROR();
	glDisable(GL_COLOR_MATERIAL);
	glDisableClientState(GL_COLOR_ARRAY);
	printf("fillRect(%d,%d,%d,%d)\n", color.red, color.green, color.blue, color.alpha);
	*/
	glColor4f(color.red/255.0f, color.green/255.0f, color.blue/255.0f, color.alpha/255.0f);
	GLfloat vertices[8];
	GLushort faces[4] = {0,1,2,3};
    
    IosRect r;
    if (rect == NULL) {
        r.x = 0;
        r.y = 0;
        r.h = getHeight();
        r.w = getWidth();
    }
    else
        r = *rect;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
    glBlendFuncSeparateOES(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

	initVertices(vertices, &r/*, 1, 1*/);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY); 
	glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, &faces[0]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void OpenGlDrawContext::putStringRight(IosFont *font, int x, int y, const char *text, float dx, float dy) {
    SCOPED_GL_LOCK;
    performDeferredDraw();
    gContext->setCurrent();
    if (text[0] != 0) {
        this->bindFBO();
        OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
        ifont->print(x,y,text,m_alpha,'r',dx,dy);
    }
	resetAlpha();
    setDefaultStates();
}
void OpenGlDrawContext::putString(IosFont *font, int x, int y, const char *text, float dx, float dy) {
	SCOPED_GL_LOCK;
    performDeferredDraw();
    gContext->setCurrent();
    if (text[0] != 0) {
        this->bindFBO();
        OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
        ifont->print(x,y,text,m_alpha,'l',dx,dy);
    }
	resetAlpha();
    setDefaultStates();
}
void OpenGlDrawContext::putStringWithShadow(IosFont *font, int x, int y, int shadow_x, int shadow_y, const char *text, float dx, float dy) {
	SCOPED_GL_LOCK;
    performDeferredDraw();
    gContext->setCurrent();
    if (text[0] != 0) {
        this->bindFBO();
        OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
        ifont->printWithShadow(x,y,shadow_x,shadow_y,text,m_alpha,'l', dx, dy);
    }
	resetAlpha();
    setDefaultStates();
}

void OpenGlDrawContext::putStringCenteredXY(IosFont *font, int x, int y, const char *text, float dx, float dy) {
	/*SCOPED_GL_LOCK;
	[EAGLContext setCurrentContext:gContext];
	this->bindFBO();
	OpenGlIosFont *ifont = static_cast<OpenGlIosFont*> (font);
	ifont->printCentered(x,y-ifont->getHeight()/2,text);*/
	throw "Invalid call";
}


bool OpenGlDrawContext::resize(int w, int h, void *layer)
{
    SCOPED_GL_LOCK;
    
    // Setup OpenGL projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
	if (((GTGetScreenOrientation() == GT_PORTRAIT) && (h >= w)) || ((GTGetScreenOrientation() == GT_LANDSCAPE) && (w >= h)) || (GTGetScreenOrientation() == GT_ANY_ORIENTATION))  {
        GTLogf("Resize OpenGL(%d, %d) -> Unrotated", w, h);
        this->m_isRotated = false;
        this->w = w;
        this->h = h;
		glLoadIdentity();
		glViewport(0, 0, w, h);
		glOrthof(0.0, (GLfloat)w, (GLfloat)h, 0.0, 0.0, 1.0);
	}
	else {
        GTLogf("Resize OpenGL(%d, %d) -> Rotated", w, h);
        this->m_isRotated = true;
		GLfloat matrix[] = {
			0,1,0,0,
			-1,0,0,0,
			0,0,1,0,
			0,0,0,1
		};
		glLoadMatrixf(matrix);
		glViewport(0, 0, w, h);
		glOrthof(0.0, (GLfloat)h, (GLfloat)w, 0.0, 0.0, 1.0);
        this->w = h;
        this->h = w;
	}
	GL_GET_ERROR();
	glGetFloatv(GL_MODELVIEW_MATRIX, &this->matrix[0]);
	
	// Setup some defaults.
	m_alpha = 1.0f;
	m_red = m_green = m_blue = 1.0f;
	// m_redAdd = m_greenAdd = m_blueAdd = 0.0f;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); GL_GET_ERROR();
    // Allocate color buffer backing based on the current layer size
#if REQUIRE_DEFAULT_RENDERBUFFER
    iglBindFramebufferOES(defaultFramebuffer, NULL); GL_GET_ERROR();
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
#if !defined(IOS)
    // glRenderbufferStorage(GL_RENDERBUFFER_OES, GL_EXT_color_buffer_half_float, this->w, this->h);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_OES, GL_RGBA8, w, h);
#endif
    context->renderbufferStorage(GL_RENDERBUFFER_OES, layer);
	//glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    //glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		GTLogf("Failed to make complete framebuffer object \"%s\"", getFBOStatusString(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES)));
        return false;
    }
#endif

    g_width  = this->w;
    g_height = this->h;
    return true;
}

void OpenGlDrawContext::startFrame()
{
    BENCH.start_frame();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    setDefaultStates();
    // No lighting, no z-buffer
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
}

void OpenGlDrawContext::endFrame()
{
    performDeferredDraw();
    BENCH.end_frame();

#ifdef DISABLED /* DEBUG */
    IosFontRef font = gtCommander->getFont("gfx/font.ttf", 7, Font_WHITE);
    if (font.get() != NULL) {
        std::deque<std::string>::const_iterator it = m_consoleLines.begin();
        int i = 1;
        while (it != m_consoleLines.end()) {
            putString(font, 5, i * 9 * GTGetScaleFactor(), it->c_str(), 1, 0);
            it++;
            ++i;
        }
    }
#endif
}

void OpenGlDrawContext::unrefGlObjects() {
    GTLog("OpenGlDrawContext::unrefGlObjects()");
    SCOPED_GL_LOCK;
    // Give a chance to some object to be fully destroyed
    g_surfaceLibrary.unrefGlObjects();
    g_fontLibrary.unrefGlObjects();
    gametools::gtNotifier.notify("unrefGlObjects", NULL);
}

void OpenGlDrawContext::freeGlObjects() {
    GTLog("OpenGlDrawContext::freeGlObjects()");
    SCOPED_GL_LOCK;
    gContext->setCurrent();
    g_surfaceLibrary.freeGlObjects();
    g_fontLibrary.freeGlObjects();
}

