//
//  GoomAudioSourceWav.h
//  goom-sdl: looping WAV file audio source (--file).
//

#ifndef GOOM_AUDIO_SOURCE_WAV_H
#define GOOM_AUDIO_SOURCE_WAV_H

#include "GoomAudioSource.h"
#include <SDL.h>

// Loops a WAV converted to 48 kHz stereo S16, serving 512 frames per goom
// frame. Selected via --file <path>. Constructor throws ios_fc::Exception
// if the file cannot be loaded or converted.
class GoomAudioSourceWav : public GoomAudioSource {
public:
    explicit GoomAudioSourceWav(const char *path);
    virtual ~GoomAudioSourceWav();

    virtual void idle(double currentTime);
    virtual void getSample(short sound[2][512]) const;

private:
    Sint16          *m_buffer;      // interleaved stereo, SDL_malloc'd
    unsigned int     m_totalFrames; // stereo frame count
    mutable unsigned int m_playPos;
};

#endif