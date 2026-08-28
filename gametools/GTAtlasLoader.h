//
//  GTAtlasLoader.h
//  Project
//
//  Created by Jean-Christophe Hoelt on 1/31/13.
//  Copyright (c) 2013 Fovea.cc. All rights reserved.
//

#ifndef Project_GTAtlasLoader_h
#define Project_GTAtlasLoader_h

#include "DataPathManager.h"
#include "CompositeDrawContext.h"

namespace gametools {
  
    class GTAtlasLoader {
        DataProvider *m_provider;
        bool          m_loaded;
    public:
        GTAtlasLoader(DataProvider *provider);
        void load(const char *fileName, CompositeDrawContext *atlas);
    };
    
};

#endif
