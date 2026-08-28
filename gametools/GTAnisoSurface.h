//
//  GTAnisoSurface.h
//  Project
//
//  Created by Jean-Christophe Hoelt on 5/31/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#ifndef Project_GTAnisoSurface_h
#define Project_GTAnisoSurface_h

#include "drawcontext.h"
#include "CommanderResources.h"
#include "GTLog.h"
#include <vector>
#include <set>
#include <string>
#include <cstdio>
#include "ios_hash.h"

// Strict mode: missing assets abort with a tagged log instead of being
// silently substituted by the placeholder. Default ON for asset audits;
// override with -DALIHOOD_STRICT_ASSETS=0 to boot through.
#ifndef ALIHOOD_STRICT_ASSETS
#define ALIHOOD_STRICT_ASSETS 1
#endif

namespace gametools {

class AnisoSurface;
// Implemented in GTAnisoSurface.cpp. Singleton placeholders backed by
// gfx/null.png (shipped under base.000/gfx/). Used by ckSurface() and
// AnisoSurfaceManager::get() when the requested asset is missing.
IosSurfaceRef getPlaceholderSurface();
AnisoSurface *getPlaceholderAnisoSurface();
void assertOnMissingAsset(const char *kind, const char *key);
}

namespace gametools {

//
// Anisomorphic Surfaces
// 
// Define a transformation of a surfaces that has been saved in a size different
// than what it should look like (for instance to make it a power of two)
//
// Classes
// -------
// AnisoSurface: Wrapper around an IosSurface
// AnisoSurfaceManager: Map of AnisoSurface.
// gtAnisoSurfaces: an instance of AnisoSurfaceManager, provided for conveniance.
//
// Example
// -------
//    - gfx/bob.png was a 200x240 image. It has been saved with a 256x256 resolution.
//    - gtAnisoSurfaces.registerSurface("bob", IMAGE_RGBA, "gfx/bob.png", 200, 240);
//  => the Manager will contain a image identified as "bob". Storing its size as 200x240.
//    - gtAnisoSurfaces.get("bob")->width()  returns 200
//    - gtAnisoSurfaces.get("bob")->height() returns 240
//
// Note
// ----
// It can be used as well to register the world-space size of an image.
// (as the Defender Framework does)    
//

class AnisoSurface : public BaseSurface {
    mutable bool m_cached;
    gametools::ImageType m_type;
    float m_width;
    float m_height;
    std::string m_path;
    mutable IosSurfaceRef m_surface; // Mutable because surface can be lazy-loaded during a "get".

    void setAsNull() {
        m_cached = false;
        m_type = gametools::IMAGE_RGB;
        m_path = "";
        m_width = m_height = -1.f;
        m_surface = NULL;
    }
    
public:
    AnisoSurface() { setAsNull(); }

    AnisoSurface(ImageType type, const char *path, float width, float height)
    : m_cached(false), m_type(type), m_width(width), m_height(height), m_path(path)
    {}

    AnisoSurface(gametools::IosSurfaceRef surface) : m_cached(false) {
        if (surface.get() == NULL)
            setAsNull();
        else {
            m_surface = surface;
            m_path   = surface->name; // Maybe this surface has no path (procedurally generated)... Hopefully not.
            m_type   = surface->isOpaque() ? gametools::IMAGE_RGB : gametools::IMAGE_RGBA;
            m_width  = surface->w;
            m_height = surface->h;
        }
    }
    
    AnisoSurface(const AnisoSurface *surface) : m_cached(false) {
        if (surface == NULL)
            setAsNull();
        else {
            m_surface = surface->m_surface;
            m_path = surface->m_path; // Maybe this surface has no path (procedurally generated)... Hopefully not.
            m_type = surface->m_type;
            m_width = surface->m_width;
            m_height = surface->m_height;
            m_cached = surface->m_cached;
        }
    }

    const IosSurface *get() const { if (m_surface.get() == NULL) cache();   return m_surface.get(); }
          IosSurface *get()       { if (m_surface.get() == NULL) cache();   return m_surface.get(); }
    
    const IosSurfaceRef ref() const { if (m_surface.get() == NULL) cache(); return m_surface; }
          IosSurfaceRef ref()       { if (m_surface.get() == NULL) cache(); return m_surface; }
    
    float width()  const { return m_width;  }
    float height() const { return m_height; }

    // Implementation of the BaseSurface interface
    virtual Vec2f size()  const { return Vec2f(m_width, m_height); }
    virtual void draw(DrawTarget *dt, IosRect *rect, float alpha) {
        dt->nextDrawAlpha(alpha);
        dt->draw(this->get(), NULL, rect);
    }

    void cache() const;
    void uncache() const { if (m_cached) m_surface = NULL; } // Make sure we can restore the surface.
    
    IosRect getRect(const Vec2f &center) const {
        return IosRect(center.x - m_width*0.5f, center.y - m_height*0.5f, m_width, m_height);
    }
    
    IosRect getRect(const Vec2f &center, float w, float h) const {
        if (h < 0.0f) h = w * m_height / m_width;
        else if (w < 0.0f) w = h * m_width / m_height;
        return IosRect(center.x - w*0.5f, center.y - h*0.5f, w, h);
    }
    
    static AnisoSurface null() { return AnisoSurface(NULL); }
    bool isNull() const { return m_path.size() == 0; }
};

class AnisoSurfaceManager {
    ios_fc::HashMap m_list;
public:
    void registerSurface(const char *key, gametools::ImageType type, const char *path, float width, float height) {
        ios_fc::HashValue *value = m_list.get(key);
        if (value != NULL) {
            AnisoSurface *surface = static_cast<AnisoSurface*>(value->value.ptr);
            *surface = AnisoSurface(type, path, width, height);
        }
        else
            m_list.put(key, static_cast<void*>(new AnisoSurface(type, path, width, height)));
    }
    
    class UncacheAnisoHashMapAction : public ios_fc::HashMapAction {
    public:
        virtual void action(ios_fc::HashValue *value) {
            static_cast<AnisoSurface*>(value->value.ptr)->uncache();
        }
    };

    class FreeAnisoHashMapAction : public ios_fc::HashMapAction {
    public:
        virtual void action(ios_fc::HashValue *value) {
            static_cast<AnisoSurface*>(value->value.ptr)->uncache();
            delete static_cast<AnisoSurface*>(value->value.ptr);
            value->value.ptr = NULL;
        }
    };

    void freeSurfaces() {
        UncacheAnisoHashMapAction action;
        m_list.forEach(&action);
    }
    bool has(const char *key) const { return m_list.get(key) != NULL; }
    AnisoSurface *get(const char *key) const {
        ios_fc::HashValue *value = m_list.get(key);
        if (value)
            return static_cast<AnisoSurface*>(value->value.ptr);
#if ALIHOOD_STRICT_ASSETS >= 1
        assertOnMissingAsset("gtAnisoSurfaces", key);
#endif
        // Log once per key (dedup via the static set of seen-misses).
        static std::set<std::string> warned;
        if (key && warned.insert(key).second) {
            char tmp[1024];
            std::snprintf(tmp, sizeof tmp, "ERROR: AnisoSurface \"%s\" not registered", key);
            GTLog(tmp);
        }
        return getPlaceholderAnisoSurface();
    }

    // Silent variant of get(): returns NULL on miss. No log, no abort,
    // no placeholder substitution. For callers that have their own
    // fallback ready (e.g., filesystem lookup via getSurface).
    AnisoSurface *tryGet(const char *key) const {
        ios_fc::HashValue *value = m_list.get(key);
        return value ? static_cast<AnisoSurface*>(value->value.ptr) : NULL;
    }

    ~AnisoSurfaceManager() {
        FreeAnisoHashMapAction action;
        m_list.forEach(&action);
    }
};
    
extern AnisoSurfaceManager gtAnisoSurfaces;

}

#endif
