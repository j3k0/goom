#ifndef _NULLAUDIOMANAGER_H
#define _NULLAUDIOMANAGER_H

#include "audiomanager.h"

namespace gametools {
namespace audio_manager {

class NullAudioManager : public AudioManager
{
public:
    virtual void setMusicEnabled(bool enabled) {}
    virtual void setMusicVolume(float volume)  {}
    virtual Music *loadMusic(const char *fileName) {
        return new Music();
    }
    virtual void playMusic(Music *music) {}
    virtual void stopMusic() {}
    virtual void setMusicPosition(int position) {}

    virtual void setSoundEnabled(bool enabled) {}
    virtual void setSoundVolume(float volume)  {}
    virtual Sound *loadSound(const char *fileName) {
        return new Sound();
    }
	virtual SoundPlayingID playSound(Sound *sound, float volume, float balance, float pitch) {
		return 0;
	}

    virtual void setMusicLayerVolume(int layer, float volume) {}
    virtual void resetMusicLayerVolumes() {}
};

}
}

#endif // _NULLAUDIOMANAGER_H

