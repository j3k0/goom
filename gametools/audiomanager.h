#ifndef _AUDIOMANAGER_H
#define _AUDIOMANAGER_H

#include <stdint.h>

namespace gametools {

void GTSetAudioPolicy(int policy);
extern int GTAudioPolicy;

// Return pitch for given number of semi-tones.
float GTSemiTone(float num);

namespace audio_manager
{
enum AudioPolicy
{
    UsesOutput = 0x00,
    UsesInputAndOutput = 0x01
};
enum AudioPolicyModifier
{
    ForceOutputOnSpeaker = 0x0100,
    MixWithOther = 0x1000
};
    
class Music
{
public:
    virtual ~Music() {}
};

class Sound
{
public:
    virtual ~Sound() {}
};

typedef int SoundPlayingID;

class SoundInputCallback {
public:
    virtual ~SoundInputCallback() {}
    virtual void samplesAvailable(int nSamples, int16_t *samples) = 0;
};

class AudioManager
{
public:
    virtual ~AudioManager() {}
    virtual void setMusicEnabled(bool enabled) = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual Music *loadMusic(const char *fileName) = 0;
    virtual void playMusic(Music *music) = 0;
    virtual void stopMusic() = 0;
    virtual void setMusicPosition(int position) = 0;
    virtual void setMusicLayerVolume(int layer, float volume) = 0;
    virtual void resetMusicLayerVolumes() = 0;

    virtual void setSoundEnabled(bool enabled) = 0;
    virtual void setSoundVolume(float volume) = 0;
    virtual Sound *loadSound(const char *fileName) = 0;
	virtual SoundPlayingID playSound(Sound *sound, float volume = 1.0, float balance = 0.0f, float pitch = 1.0f) = 0;
	virtual bool isPlaying(SoundPlayingID soundID) const { return false; }
	virtual void stopSound(SoundPlayingID soundID) const { }
	virtual void updateSoundVolume(SoundPlayingID soundID, float volume) const { }
	virtual void updateSoundBalance(SoundPlayingID soundID, float balance) const { }
    
    virtual void setMicEnabled(bool enabled) {}
    virtual void setSoundInputCallback(SoundInputCallback *callback) {}
};

}
}

#endif // _AUDIOMANAGER_H

