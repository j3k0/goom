#ifndef FREE_NEHE_H
#define FREE_NEHE_H

#ifdef GT_NO_FREETYPE
// GT_NO_FREETYPE disables the freetype-backed font backend: either the macro
// must go, or this translation unit must be dropped from the build. Compiling
// both would define gametools::g_fontLibrary twice and give GLFont two
// different layouts in one program.
#error "GT_NO_FREETYPE defined: drop the macro or drop GLFreeType from the build"
#endif

#include <map>
#include <vector>
#include <list>
#include <string>

#include "GTOpenGl.h"
#include "GTLog.h"
#include "ios_memory.h"
#include <assert.h>

/*
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#import <OpenGLES/ES1/gl.h>
#elif defined(__APPLE_CC__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
*/

//FreeType Headers
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

namespace gametools {

    class GLFont;
    typedef std::list<GLFont*> GLFontList;

    class GLFontLibrary {
        GLFontList m_fontList;
        public:
            void registerFont(GLFont *font)   { m_fontList.push_back(font); }
            void unregisterFont(GLFont *font) { m_fontList.remove(font); }

            void unrefGlObjects();
            void freeGlObjects();
    };
    extern GLFontLibrary g_fontLibrary;
    
    struct GlyphData {
        GlyphData();
        ~GlyphData();
        GLuint texture;
        float letter_width;
        float texcoord[8];
        float vertices[8];
        unsigned short faces[4];
    };
    void make_dlist(FT_Face face, unsigned short ch, float h, GlyphData &glyphData);
    
    class GLFont {
        
    private:
        friend class GLFontLibrary;
        FT_Library library;
        FT_Face face;
        void *m_data;
        int   m_dataSize;
        float h;			///< Holds the height of the font.

        typedef std::map<unsigned int, GlyphData *> GlyphMap;
        GlyphMap nonAsciiGlyphes;
        // typedef std::vector<unsigned short> Line;
        // typedef std::vector<Line> MultiLine;
        
        inline GlyphData & getGlyphData(unsigned short ch) {
            GlyphMap::iterator iter = nonAsciiGlyphes.find(ch);
            if (iter != nonAsciiGlyphes.end())
                return *(iter->second);
            GlyphData *newGlyph = new GlyphData();
            make_dlist(face, ch, h, *newGlyph);
            nonAsciiGlyphes[ch] = newGlyph;
            return *newGlyph;
        }

        inline void freeGlObjects() {
            GlyphMap::iterator it = nonAsciiGlyphes.begin();
            while (it != nonAsciiGlyphes.end()) {
                delete it->second;
                ++it;
            }
            nonAsciiGlyphes.clear();
        }
        inline void unrefGlObjects() {
            GlyphMap::iterator it = nonAsciiGlyphes.begin();
            while (it != nonAsciiGlyphes.end()) {
                it->second->texture = 0;
                delete it->second;
                ++it;
            }
            nonAsciiGlyphes.clear();
        }
        
#define kMaxLineWidth 2048
#define kMaxLines      256

        struct Line {
            int width;
            unsigned short line[kMaxLineWidth];
            void clear() { width = 0; }
            void push_back(unsigned short n) { assert(width >= 0); line[width++] = n; assert(width < kMaxLineWidth); }
            int size() const { return width; }
        };
        
        struct MultiLine {
            int numLines;
            Line lines[kMaxLines];
            MultiLine()  { clear(); }

            void clear()          { numLines = 0; lines[0].clear(); }
            Line &nextAvailable() { return lines[numLines]; }
            void push_back()      { assert(numLines >= 0); numLines++; lines[numLines].clear(); assert(numLines < kMaxLines); }
            int  size() const     { return numLines; }
        };
        
        inline void textToLines(const unsigned short *text, MultiLine &lines) {
            lines.clear();
            // Here is some code to split the text that we have been
            // given into a set of lines.  
            // This could be made much neater by using
            // a regular expression library such as the one avliable from
            // boost.org (I've only done it out by hand to avoid complicating
            // this tutorial with unnecessary library dependencies).
            const unsigned short *start_line = text;
            const unsigned short *c;
            for(c=text;*c;c++) {
                if(*c=='\n') {
                    Line &line = lines.nextAvailable();
                    for (const unsigned short *n=start_line;n<c;n++)
                        line.push_back(*n);
                    lines.push_back();
                    start_line=c+1;
                }
            }
            if(start_line) {
                Line &line = lines.nextAvailable();
                for(const unsigned short *n=start_line;n<c;n++)
                    line.push_back(*n);
                lines.push_back();
            }
        }
		
		std::string fname;
        
    public:
        GLFont(void *data, int dataSize, unsigned int h, float letter_spacing, float line_spacing) {
            init(data, dataSize, h, letter_spacing, line_spacing);
            g_fontLibrary.registerFont(this);
        }
        GLFont() { g_fontLibrary.registerFont(this); }
        ~GLFont() {
            // if (m_data != NULL)
            //     gametools::GTLogf("MEMORY WARNING: GLFont hasn't been cleaned.");
            g_fontLibrary.unregisterFont(this);
        }
        
		const char *getFileName() const { return fname.c_str(); }
        float letter_spacing;
        float line_spacing;
        float getHeight() const { return h; }
        
        //The init function will create a font of
        //of the height h from the file fname.
        void init(void *data, int dataSize, float height, float letter_spacing, float line_spacing);
        
        //Free all the resources assosiated with the font.
        void clean();
        
        //The flagship function of the library - this thing will print
        //out text at window coordinates x,y, using the font ft_font.
        //The current modelview matrix will also be applied to the text. 
        void printUnicode(float size, float x, float y, const unsigned short *text, char dir, float dx, float dy) ;
        void printCenteredUnicode(float size, float x, float y, const unsigned short *text, float dx, float dy);
        float getWidthUnicode(float size, const unsigned short *text);
        float getHeightUnicode(float size, const unsigned short *text);
        //void print(float x, float y, const wchar_t *fmt, ...) ;
        //void printCentered(float x, float y, const wchar_t *fmt, ...);
        //float getWidth(const wchar_t *fmt, ...);
    };
}

#endif
