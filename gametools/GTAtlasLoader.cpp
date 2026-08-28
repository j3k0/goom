//
//  GTAtlasLoader.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 1/31/13.
//  Copyright (c) 2013 Fovea.cc. All rights reserved.
//

#include "GTAtlasLoader.h"
#include <sstream>

namespace gametools {
    //    virtual bool hasDataInputStream(const char *shortPath) const = 0;
    //    virtual DataInputStream *openDataInputStream(const char *shortPath) const = 0;

    GTAtlasLoader::GTAtlasLoader(DataProvider *provider) : m_provider(provider), m_loaded(false) {}
    
    void GTAtlasLoader::load(const char *fileName, CompositeDrawContext *composite) {
        if (m_loaded) return;
        
        int i = 0;
        do {
            std::stringstream ss;
            ss << "gfx/atlas" << i << ".txt";
            if (m_provider->hasDataInputStream(ss.str().c_str())) {
                DataInputStream *tmp = m_provider->openDataInputStream(ss.str().c_str());
                BufferedStream input(*tmp);
                delete tmp;
                char  line[1024];
                // int   fileStart = 0;

                char atlasFileName[1024];
                // int  fileSize;

                while (input.gets(line, 1024)) {
                    int len = strlen(line);
                    if (line[len-1] == '\n') {
                        line[len-1] = '\0';
                        --len;
                    }
                    if (len < 2) continue;

                    if (line[0] == 'G') {
                        sscanf(line + 2, "%s", atlasFileName);
                    }
                    else if (line[0] == '+') {
                        char imageFileName[1024];
                        int x, y, w, h;
                        sscanf(line + 2, "%s %d %d %d %d", imageFileName, &x, &y, &w, &h);
                        composite->declareCompositeSurface(imageFileName, atlasFileName, x, y, w, h);
                    }
                }
                i += 1;
            }
            else
                i = -1;
        }
        while (i >= 0);
        
        m_loaded = true;
    }
}
