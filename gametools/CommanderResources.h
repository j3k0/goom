/*
 *  CommanderResources.h
 *  empty2d
 *
 *  Created by Jean-Christophe Hoelt on 3/4/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef CommanderResources_H
#define CommanderResources_H

#include <vector>
#include "ResourceManager.h"
#include "DataPathManager.h"
#include "audiomanager.h"

namespace gametools {

/**
 * Resource key for IosSurface management.
 * Makes an unique key for an ImageType, path, Ability association
 */
class IosSurfaceResourceKey {
public:
    IosSurfaceResourceKey(ImageType type, const std::string &path, ImageSpecialAbility specialAbility)
	: path(path), type(type), specialAbility(specialAbility) {}
    bool operator ==(const IosSurfaceResourceKey k) const {
        return  ((this->path == k.path) &&
                 (this->type == k.type) &&
                 (this->specialAbility == k.specialAbility));
    }
    bool operator < (const IosSurfaceResourceKey k) const {
        if (this->path == k.path) {
            if (this->type == k.type)
                return this->specialAbility < k.specialAbility;
            return this->type < k.type;
        }
        return (this->path < k.path);
    }
    std::string path;
    ImageType type;
    ImageSpecialAbility specialAbility;
};

/**
 * Resource key for IosFont management.
 * Makes an unique key for an ImageType, path, Ability association
 */
class IosFontResourceKey {
public:
    IosFontResourceKey(const std::string &path, int size, IosFontFx fx = Font_STD)
    : path(path), size(size), fx(fx) {}
    bool operator ==(const IosFontResourceKey k) const {
        return  ((this->path == k.path) &&
                 (this->size == k.size) &&
                 (this->fx.ID() == k.fx.ID()));
    }
    bool operator < (const IosFontResourceKey k) const {
        if (this->path == k.path) {
            if (this->size == k.size)
                return this->fx.ID() < k.fx.ID();
            return this->size < k.size;
        }
        return (this->path < k.path);
    }
    std::string path;
    int size;
    IosFontFx fx;
};

/**
 * Factory for IosSurface resources
 */
class IosSurfaceFactory : public ResourceFactory<IosSurface, IosSurfaceResourceKey>
{
public:
    IosSurfaceFactory(DataPathManager &dataPathManager)
	: m_dataPathManager(dataPathManager) {}
    virtual IosSurface *create(const IosSurfaceResourceKey &resourceKey);
    virtual void destroy(IosSurface *res);
private:
    DataPathManager &m_dataPathManager;
};

/**
 * Factory for IosFont resources
 */
class IosFontFactory : public ResourceFactory<IosFont, IosFontResourceKey>
{
public:
    IosFontFactory(DataPathManager &dataPathManager)
    : m_dataPathManager(dataPathManager) {}
    virtual IosFont *create(const IosFontResourceKey &resourceKey);
    virtual void destroy(IosFont *res);
private:
    DataPathManager &m_dataPathManager;
};

/**
 * Factory for Sound resources
 */
class SoundFactory : public ResourceFactory<audio_manager::Sound>
{
public:
    SoundFactory(DataPathManager &dataPathManager)
	: m_dataPathManager(dataPathManager) {}
    virtual audio_manager::Sound *create(const std::string &path);
    virtual void destroy(audio_manager::Sound *res);
private:
    DataPathManager &m_dataPathManager;
};

/**
 * Factory for Music resources
 */
class MusicFactory : public ResourceFactory<audio_manager::Music>
{
public:
    MusicFactory(DataPathManager &dataPathManager)
	: m_dataPathManager(dataPathManager) {}
    virtual audio_manager::Music *create(const std::string &path);
    virtual void destroy(audio_manager::Music *res);
private:
    DataPathManager &m_dataPathManager;
};

// IosSurface resources
class IosSurfaceRef : public ResourceReference<IosSurface>, public BaseSurface {
    public:
    IosSurfaceRef() : ResourceReference<IosSurface>() {}
    IosSurfaceRef(ResourceHolder<IosSurface> *ownerResHolder) : ResourceReference<IosSurface>(ownerResHolder) {}
    IosSurfaceRef(const ResourceReference<IosSurface> &res) : ResourceReference<IosSurface>(res) {}

    virtual Vec2f size() const { return (get() ? get()->size() : Vec2f(1,1)); }
    virtual void draw(DrawTarget *dt, IosRect *rect, float alpha) {
        if (get()) get()->draw(dt,rect,alpha);
    }
};

typedef ResourceManager<IosSurface, IosSurfaceResourceKey> IosSurfaceResourceManager;
// Font resources
typedef ResourceReference<IosFont> IosFontRef;
typedef ResourceManager<IosFont, IosFontResourceKey> IosFontResourceManager;
// Sound resources
typedef ResourceReference<audio_manager::Sound> SoundRef;
typedef ResourceManager<audio_manager::Sound> SoundResourceManager;
// Music resources
typedef ResourceReference<audio_manager::Music> MusicRef;
typedef ResourceManager<audio_manager::Music> MusicResourceManager;

class TextSurfaceRef : public TextSurface {
    public:
        TextSurfaceRef(const IosFontRef &font, const std::string &text) : TextSurface(font.get(), text), m_fontRef(font) {}
    private:
        IosFontRef m_fontRef;
};

// Surface Reference not using the resource manager.
struct LonelySurfaceRef {
    IosSurface                 *surface;
    ResourceHolder<IosSurface> *holder;
    IosSurfaceRef              *ref;
    
    LonelySurfaceRef(IosSurface *s) {
        surface = s;
        holder  = new ResourceHolder<IosSurface>(s);
        ref     = new IosSurfaceRef(holder);
    }
    
    ~LonelySurfaceRef() {
        delete ref;
        delete holder;
        delete surface;
    }
};
}

#endif
