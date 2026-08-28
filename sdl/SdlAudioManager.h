#ifndef SDLAUDIOMANAGER_H
#define SDLAUDIOMANAGER_H

#include <SDL.h>
#include <deque>

#include "audiomanager.h"
#include "FPDataPathManager.h"

namespace gametools {
namespace audio_manager {

// Mixer data format: 44100 Hz, stereo, signed 16-bit.
const int kSdlAudioRateHz = 44100;
const int kSdlAudioChannels = 2;

class SdlSound : public Sound {
public:
    SdlSound(FPDataPathManager *dpm, const char *fileName);
    // Decoded samples, mono, 16-bit, at kSdlAudioRateHz.
    const std::deque<int16_t> &samples() const { return m_samples; }

private:
    std::deque<int16_t> m_samples;
};

class SdlPlayingSound {
public:
    SdlPlayingSound(SoundPlayingID id, const SdlSound &sound, float volume)
        : m_id(id), m_samples(sound.samples()), m_position(0), m_volume(int(volume * 127.0f)) {}

    SoundPlayingID getID() const { return m_id; }
    // Mix into an interleaved stereo S16 buffer. Returns false when done.
    bool mix(int16_t *out, int outBytes);

private:
    SoundPlayingID m_id;
    const std::deque<int16_t> &m_samples;
    int m_position;
    int m_volume;
};

class SdlAudioManager : public AudioManager
{
public:
    SdlAudioManager(FPDataPathManager *dpm);
    virtual ~SdlAudioManager();

    virtual Music *loadMusic(const char *fileName);
    virtual Sound *loadSound(const char *fileName);
    virtual SoundPlayingID playSound(Sound *sound, float volume = 1.0f, float balance = 0.0f, float pitch = 1.0f);
    virtual bool isPlaying(SoundPlayingID soundID) const;
    virtual void stopSound(SoundPlayingID soundID) const;
    virtual void setSoundEnabled(bool enabled);
    virtual void setSoundVolume(float volume);
    virtual void setMusicEnabled(bool enabled) {}
    virtual void setMusicVolume(float volume) {}
    virtual void playMusic(Music *music) {}
    virtual void stopMusic() {}
    virtual void setMusicPosition(int position) {}
    virtual void setMusicLayerVolume(int layer, float volume) {}
    virtual void resetMusicLayerVolumes() {}

    void start();
    void stop();

    // Called from the SDL audio callback.
    int readData(void *buffer, int maxSize);

private:
    FPDataPathManager *m_dpm;
    SDL_AudioDeviceID  m_device;
    std::deque<SdlPlayingSound*> m_sounds;
    SoundPlayingID m_nextId;
    float m_soundVolume;
    bool  m_soundEnabled;
};

}}
#endif // SDLAUDIOMANAGER_H