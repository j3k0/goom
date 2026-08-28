#include "SdlAudioManager.h"

#include <string.h>
#include <deque>

#include "GameTools.h"
#include "ios_fc.h"
#include "ios_mutex.h"

namespace gametools {
namespace audio_manager {

// The SDL audio callback runs on its own thread; playSound/setSoundEnabled
// mutate the same deque from the main thread. Same protection the old
// modplug pipeline had (SCOPED_LOCK in modplugaudiomanager.cpp).
static ios_fc::Mutex &audioMutex() {
    static ios_fc::Mutex mutex;
    return mutex;
}

// ---- WAV decoding ----------------------------------------------------------
// Minimal RIFF/WAVE parser: finds the fmt and data chunks and returns the
// PCM payload converted to mono 16-bit at kSdlAudioRateHz.

namespace {

struct WavData {
    std::deque<int16_t> samples;
};

uint32_t readU32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint16_t readU16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

bool decodeWav(const void *fileData, int fileSize, WavData &out) {
    const uint8_t *p = (const uint8_t *)fileData;
    const int minHeader = 12;
    if (fileSize < minHeader) return false;
    if (memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0) return false;

    int pos = 12;
    uint16_t audioFormat = 0, numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    const uint8_t *dataChunk = NULL;
    int dataChunkSize = 0;

    while (pos + 8 <= fileSize) {
        char chunkId[5] = {0};
        memcpy(chunkId, p + pos, 4);
        uint32_t chunkSize = readU32(p + pos + 4);
        pos += 8;
        if (pos + (int)chunkSize > fileSize) chunkSize = fileSize - pos;

        if (memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16) {
            audioFormat   = readU16(p + pos);
            numChannels   = readU16(p + pos + 2);
            sampleRate    = readU32(p + pos + 4);
            bitsPerSample = readU16(p + pos + 14);
        }
        else if (memcmp(chunkId, "data", 4) == 0) {
            dataChunk = p + pos;
            dataChunkSize = chunkSize;
        }
        pos += chunkSize + (chunkSize & 1); // chunks are word-aligned
    }

    if (dataChunk == NULL || audioFormat != 1 || numChannels == 0
        || (bitsPerSample != 16 && bitsPerSample != 8) || sampleRate == 0)
        return false;

    const int bytesPerSample = bitsPerSample / 8;
    const int frameSize = bytesPerSample * numChannels;
    if (frameSize == 0) return false;

    const int frames = dataChunkSize / frameSize;
    // 16.16 fixed-point linear-interpolated resampling from source rate to
    // mixer rate, downmixing all channels to a single mono sample. Mirrors
    // the old MPlugSound::load behavior (per-channel lerp, then average).
    const uint64_t step = ((uint64_t)sampleRate << 16) / kSdlAudioRateHz;
    const uint64_t outSamples = (((uint64_t)frames) << 16) / step;
    for (uint64_t k = 0; k < outSamples; ++k) {
        const uint64_t pos = k * step;
        const uint64_t ipos = pos >> 16;
        const uint64_t inext = (ipos + 1 < (uint64_t)frames) ? ipos + 1 : ipos;
        const int frac = (int)(pos & 0xFFFF);
        long acc = 0;
        for (int c = 0; c < numChannels; ++c) {
            const uint8_t *s0 = dataChunk + ipos * frameSize + c * bytesPerSample;
            const uint8_t *s1 = dataChunk + inext * frameSize + c * bytesPerSample;
            long v0, v1;
            if (bitsPerSample == 16) { v0 = (int16_t)readU16(s0); v1 = (int16_t)readU16(s1); }
            else                     { v0 = (int8_t)(*s0) * 255;  v1 = (int8_t)(*s1) * 255; }
            acc += (v0 * (0x10000 - frac) + v1 * frac) >> 16;
        }
        out.samples.push_back((int16_t)(acc / numChannels));
    }
    return true;
}

} // anonymous namespace

// ---- SdlSound --------------------------------------------------------------

SdlSound::SdlSound(FPDataPathManager *dpm, const char *fileName)
{
    if (dpm && dpm->hasDataInputStream(fileName)) {
        DataInputStream *f = dpm->openDataInputStream(fileName);
        BufferedStream stream(*f);
        ios_fc::VoidBuffer blob = stream.data();
        WavData wav;
        if (decodeWav(blob.ptr(), blob.size(), wav))
            m_samples.swap(wav.samples);
        else
            GTLogf("SdlSound: unsupported or corrupt WAV: %s", fileName);
        delete f;
    }
    else {
        throw ios_fc::Exception(ios_fc::String("Couldn't open sound file: ") + fileName);
    }
}

// ---- SdlPlayingSound -------------------------------------------------------

bool SdlPlayingSound::mix(int16_t *out, int outBytes)
{
    const int outFrames = outBytes / (2 * kSdlAudioChannels);
    int played = 0;
    const int total = (int)m_samples.size();
    while (played < outFrames && m_position < total) {
        const int value = (m_samples[m_position] * m_volume) >> 8;
        out[2 * played]     = (int16_t)(out[2 * played]     + value);
        out[2 * played + 1] = (int16_t)(out[2 * played + 1] + value);
        ++m_position;
        ++played;
    }
    return m_position < total;
}

// ---- Audio callback --------------------------------------------------------

static void audioCallback(void *userdata, Uint8 *stream, int len)
{
    SdlAudioManager *mgr = (SdlAudioManager *)userdata;
    if (gametools::GTPaused()) {
        memset(stream, 0, len);
        return;
    }
    int got = mgr->readData(stream, len);
    if (got < len)
        memset(stream + got, 0, len - got);
}

// ---- SdlAudioManager -------------------------------------------------------

SdlAudioManager::SdlAudioManager(FPDataPathManager *dpm)
    : m_dpm(dpm), m_device(0), m_nextId(1), m_soundVolume(1.0f), m_soundEnabled(true)
{
    start();
}

SdlAudioManager::~SdlAudioManager() {
    stop();
}

void SdlAudioManager::start() {
    if (m_device != 0) return;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = kSdlAudioRateHz;
    want.format   = AUDIO_S16SYS;
    want.channels = kSdlAudioChannels;
    want.samples  = 1024;
    want.callback = audioCallback;
    want.userdata = this;

    m_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (m_device == 0) {
        GTLogf("SdlAudioManager: SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(m_device, 0);
    GTLog("SdlAudioManager: audio device started");
}

void SdlAudioManager::stop() {
    if (m_device == 0) return;
    SDL_CloseAudioDevice(m_device);
    m_device = 0;
}

Music *SdlAudioManager::loadMusic(const char *fileName) {
    (void)fileName;
    // Music playback (tracker files) was removed with libmodplug.
    // Goom uses WAV file / mic / heartbeat audio sources only.
    return NULL;
}

Sound *SdlAudioManager::loadSound(const char *fileName) {
    GTLogf("+++ Loading Sound %s", fileName);
    return new SdlSound(m_dpm, fileName);
}

SoundPlayingID SdlAudioManager::playSound(Sound *sound, float volume, float balance, float pitch)
{
    (void)balance; (void)pitch;
    ios_fc::Lock lock(audioMutex());
    if (!m_soundEnabled || sound == NULL) return 0;
    SdlSound *sdlSound = static_cast<SdlSound*>(sound);
    SdlPlayingSound *playing = new SdlPlayingSound(m_nextId++, *sdlSound, volume * m_soundVolume);
    m_sounds.push_back(playing);
    return playing->getID();
}

bool SdlAudioManager::isPlaying(SoundPlayingID soundID) const
{
    if (soundID <= 0) return false;
    ios_fc::Lock lock(audioMutex());
    for (std::deque<SdlPlayingSound*>::const_iterator it = m_sounds.begin(); it != m_sounds.end(); ++it)
        if ((*it)->getID() == soundID) return true;
    return false;
}

void SdlAudioManager::stopSound(SoundPlayingID soundID) const
{
    (void)soundID;
}

void SdlAudioManager::setSoundEnabled(bool enabled)
{
    ios_fc::Lock lock(audioMutex());
    m_soundEnabled = enabled;
    if (!enabled) {
        for (std::deque<SdlPlayingSound*>::iterator it = m_sounds.begin(); it != m_sounds.end(); ++it)
            delete *it;
        m_sounds.clear();
    }
}

void SdlAudioManager::setSoundVolume(float volume)
{
    m_soundVolume = volume;
}

int SdlAudioManager::readData(void *buffer, int maxSize)
{
    ios_fc::Lock lock(audioMutex());
    int16_t *out = (int16_t *)buffer;
    const int outBytes = (maxSize / 4) * 4; // whole stereo frames only
    memset(out, 0, outBytes);

    std::deque<SdlPlayingSound*> keep;
    for (std::deque<SdlPlayingSound*>::iterator it = m_sounds.begin(); it != m_sounds.end(); ++it) {
        if ((*it)->mix(out, outBytes))
            keep.push_back(*it);
        else
            delete *it;
    }
    m_sounds = keep;
    return outBytes;
}

}}