//
//  GTAnisoSurface.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 5/31/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#include "GTAnisoSurface.h"
#include "Commander.h"
#include "GTLog.h"

#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

namespace gametools {
    AnisoSurfaceManager gtAnisoSurfaces;

    void AnisoSurface::cache() const {
        if (this == NULL) return;
        if (m_surface.get() == NULL && m_path.length() > 0) {
            m_cached = true;
            m_surface = gtCommander->getSurface(m_type, m_path.c_str());
        }
    }

    IosSurfaceRef getPlaceholderSurface() {
        static IosSurfaceRef cached;
        if (cached.get() == NULL && gtCommander != NULL) {
            cached = gtCommander->getSurface(IMAGE_RGBA, "gfx/null.png", IMAGE_CENTERED);
        }
        return cached;
    }

    AnisoSurface *getPlaceholderAnisoSurface() {
        // 64x64 advertised so callers dividing by ->height() get a sane scalar.
        static AnisoSurface *cached = NULL;
        if (cached == NULL) {
            cached = new AnisoSurface(IMAGE_RGBA, "gfx/null.png", 64.0f, 64.0f);
        }
        return cached;
    }

    void assertOnMissingAsset(const char *kind, const char *key) {
#if ALIHOOD_STRICT_ASSETS >= 2
        // Audit mode: log once per (kind,key) with a tag for grep, do not abort.
        static std::set<std::string> warned;
        std::string tag = std::string(kind) + ":" + (key ? key : "(null)");
        if (!warned.insert(tag).second) return;
        char buf[1024];
        std::snprintf(buf, sizeof buf,
                      "STRICT_AUDIT: missing %s \"%s\"",
                      kind, key ? key : "(null)");
        GTLog(buf);
        std::fprintf(stderr, "%s\n", buf);
#else
        char buf[1024];
        std::snprintf(buf, sizeof buf,
                      "STRICT ASSET FAULT (%s): missing \"%s\"",
                      kind, key ? key : "(null)");
        GTLog(buf);
        std::fprintf(stderr, "%s\n", buf);
        std::abort();
#endif
    }
}
