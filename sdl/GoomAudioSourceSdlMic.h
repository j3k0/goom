//
//  GoomAudioSourceSdlMic.h
//  goom-sdl: live microphone input via SDL2 capture device.
//

#ifndef GOOM_AUDIO_SOURCE_SDL_MIC_H
#define GOOM_AUDIO_SOURCE_SDL_MIC_H

#include "GoomAudioSource.h"
#include <SDL.h>

// Captures mono mic input through an SDL2 capture device and serves the
// most recent 512 frames (duplicated to stereo) per goom frame, with the
// same auto-gain normalization as the iOS reference
// (Goom/GoomAudioSourceMic.cpp): m_max tracks the recent peak with a 5%/s
// decay and is clamped to absoluteMax/2 so quiet rooms don't over-amplify.
//
// If the device cannot be opened (permission denied, no hardware) the
// object is still valid but isAvailable() returns false — the delegate
// then falls back to another audio source.
class GoomAudioSourceSdlMic : public GoomAudioSource {
public:
    GoomAudioSourceSdlMic();
    virtual ~GoomAudioSourceSdlMic();

    bool isAvailable() const { return m_device != 0; }

    virtual void idle(double currentTime);
    virtual void getSample(short sound[2][512]) const;

private:
    static void SDLCALL audioCallback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID m_device;

    // Ring of the most recent mono frames. Written on SDL's audio thread
    // (under the device lock), read on the main thread in getSample (also
    // under the device lock). Overwrites oldest on overflow.
    static const int kRingSize = 1024; // power of two, >= 2x512
    short         m_ring[kRingSize];
    volatile int  m_writePos;   // next write index, 0..kRingSize-1

    // Auto-gain state (main thread only).
    int    m_max;
    int    m_absoluteMax;
    int    m_windowPeak;
    double m_lastAdjustment;
};

#endif
