//
//  GoomAudioSourceWav.cpp
//  goom-sdl: looping WAV file audio source (--file).
//

#include "GoomAudioSourceWav.h"
#include "GTLog.h"
#include "ios_fc.h"
#include <string.h>

using namespace gametools;

GoomAudioSourceWav::GoomAudioSourceWav(const char *path)
    : m_buffer(NULL), m_totalFrames(0), m_playPos(0)
{
    SDL_AudioSpec spec;
    Uint8 *raw = NULL;
    Uint32 rawLen = 0;
    if (SDL_LoadWAV(path, &spec, &raw, &rawLen) == NULL)
        throw ios_fc::Exception(ios_fc::String("goom: cannot load ") + path + ": " + SDL_GetError());

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
                          AUDIO_S16SYS, 2, 48000) < 0) {
        SDL_FreeWAV(raw);
        throw ios_fc::Exception(ios_fc::String("goom: cannot convert ") + path);
    }
    cvt.buf = (Uint8*)SDL_malloc((size_t)rawLen * (size_t)cvt.len_mult);
    if (cvt.buf == NULL) {
        SDL_FreeWAV(raw);
        throw ios_fc::Exception(ios_fc::String("goom: out of memory converting ") + path);
    }
    memcpy(cvt.buf, raw, rawLen);
    cvt.len = rawLen;
    SDL_FreeWAV(raw);
    if (SDL_ConvertAudio(&cvt) != 0) {
        SDL_free(cvt.buf);
        throw ios_fc::Exception(ios_fc::String("goom: conversion failed for ") + path);
    }
    m_buffer = reinterpret_cast<Sint16*>(cvt.buf);
    // len_cvt is the authoritative converted byte count after
    // SDL_ConvertAudio (bytes past it are undefined per SDL_audio.h);
    // len*len_ratio is only SDL_BuildAudioCVT's pre-conversion estimate.
    m_totalFrames = (unsigned int)cvt.len_cvt / 4; // 2 ch * 2 bytes
    if (m_totalFrames == 0) {
        SDL_free(cvt.buf);
        m_buffer = NULL;
        throw ios_fc::Exception(ios_fc::String("goom: empty audio in ") + path);
    }
    GTLogf("goom: loaded %s (%u frames)", path, m_totalFrames);
}

GoomAudioSourceWav::~GoomAudioSourceWav() {
    if (m_buffer) SDL_free(m_buffer);
}

void GoomAudioSourceWav::idle(double currentTime) {
    (void)currentTime;
}

void GoomAudioSourceWav::getSample(short sound[2][512]) const {
    for (int j = 0; j < 512; ++j) {
        unsigned int p = m_playPos;
        sound[0][j] = m_buffer[p * 2];
        sound[1][j] = m_buffer[p * 2 + 1];
        m_playPos = (p + 1) % m_totalFrames;
    }
}