//
//  GoomAudioSourceSdlMic.cpp
//  goom-sdl: live microphone input via SDL2 capture device.
//

#include "GoomAudioSourceSdlMic.h"
#include "GTLog.h"
#include <string.h>

using namespace gametools;

namespace {
    const int kSampleRate = 48000;
}

GoomAudioSourceSdlMic::GoomAudioSourceSdlMic()
    : m_device(0), m_writePos(0),
      m_max(0), m_absoluteMax(0), m_windowPeak(0), m_lastAdjustment(0.0)
{
    memset(m_ring, 0, sizeof(m_ring));

    // SdlMain already initialized SDL_INIT_AUDIO for playback; capture
    // shares the subsystem. Init only if somehow absent.
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 &&
        SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        GTLogf("goom: mic unavailable (SDL_InitSubSystem: %s)", SDL_GetError());
        return;
    }

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq     = kSampleRate;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 512;
    want.callback = &GoomAudioSourceSdlMic::audioCallback;
    want.userdata = this;

    m_device = SDL_OpenAudioDevice(NULL, SDL_TRUE,
                                   &want, &have, 0);
    if (m_device == 0) {
        GTLogf("goom: mic unavailable (SDL_OpenAudioDevice: %s), "
               "falling back to heartbeat", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(m_device, 0);
    GTLogf("goom: mic capture started at %d Hz", have.freq);
}

GoomAudioSourceSdlMic::~GoomAudioSourceSdlMic() {
    if (m_device != 0)
        SDL_CloseAudioDevice(m_device);
}

void SDLCALL GoomAudioSourceSdlMic::audioCallback(void *userdata, Uint8 *stream, int len) {
    // SDL holds the device lock while invoking this callback, so no
    // additional locking here — and SDL mutexes are non-recursive, so
    // SDL_LockAudioDevice in this callback would deadlock, not no-op.
    GoomAudioSourceSdlMic *self = static_cast<GoomAudioSourceSdlMic*>(userdata);
    const int16_t *samples = reinterpret_cast<const int16_t*>(stream);
    int n = len / (int)sizeof(int16_t);
    for (int i = 0; i < n; ++i) {
        self->m_ring[self->m_writePos] = samples[i];
        self->m_writePos = (self->m_writePos + 1) & (kRingSize - 1);
    }
}

void GoomAudioSourceSdlMic::idle(double currentTime) {
    // Auto-gain, ported from Goom/GoomAudioSourceMic.cpp (iOS reference):
    // track recent peak, decay 5% per second, clamp to absoluteMax/2.
    if (m_windowPeak > m_max)
        m_max = m_windowPeak;
    else if (currentTime - m_lastAdjustment > 1.0) {
        m_lastAdjustment = currentTime;
        m_max = (int)(m_max * 0.95);
    }
    if (m_max > m_absoluteMax)
        m_absoluteMax = m_max;
    if (m_max > m_absoluteMax / 2)
        m_max = m_absoluteMax / 2;
    m_windowPeak = 0;
}

void GoomAudioSourceSdlMic::getSample(short sound[2][512]) const {
    if (m_device == 0) {
        memset(sound, 0, sizeof(short) * 2 * 512);
        return;
    }
    GoomAudioSourceSdlMic *self = const_cast<GoomAudioSourceSdlMic*>(this);
    SDL_LockAudioDevice(self->m_device);
    int readPos = (self->m_writePos - 512) & (kRingSize - 1);
    int peak = 0;
    for (int j = 0; j < 512; ++j) {
        int v = self->m_ring[readPos];
        sound[0][j] = (short)v;
        sound[1][j] = (short)v;
        int a = v < 0 ? -v : v;
        if (a > peak) peak = a;
        readPos = (readPos + 1) & (kRingSize - 1);
    }
    SDL_UnlockAudioDevice(self->m_device);

    // Window peak feeds next idle()'s auto-gain; normalize to +-31000
    // exactly like the iOS reference.
    if (peak > self->m_windowPeak)
        self->m_windowPeak = peak;
    int max = (self->m_max == 0 ? 1 : self->m_max);
    for (int j = 0; j < 512; ++j) {
        sound[0][j] = (short)(31000 * (int)sound[0][j] / max);
        sound[1][j] = (short)(31000 * (int)sound[1][j] / max);
    }
}
