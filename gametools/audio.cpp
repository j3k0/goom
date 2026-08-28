#include <string.h>
#include "audio.h"
#include "GTPreferences.h"

#define TIMEMS_BETWEEN_SAME_SOUND 25.0

#include <vector>
#include <map>
#include <string>

#include "Commander.h"

using namespace ios_fc;
namespace gametools {

void GTLog(const char *);

static std::map<std::string, double> lastUsedTimestamp;
static bool   audio_supported = false;

static float sound_volume   = 1.0f;
static float music_volume   = 1.0f;

static bool sound_on = false;
static bool music_on = false;
static float music_stress = 1.0f;
static float music_relax = 1.0f;
// static bool music_starting = false;

static std::string music_current = "";
static std::string music_command = "";

static const char * kMusicVolume = "AudioManager.Music.Volume";
static const char * kSoundVolume = "AudioManager.FX.Volume";
static const char * kMusic       = "AudioManager.Music.State";
static const char * kSound       = "AudioManager.FX.State";

audio_manager::AudioManager * AudioManager::m_audioManager;
const MusicPackage *AudioManager::m_MusicPackage;

void AudioManager::init(const MusicPackage &musicPackage)
{
    m_audioManager = GameUIDefaults::GAME_LOOP->getAudioManager();
	m_MusicPackage = &musicPackage;

    audio_supported = true;
    if (!audio_supported) return;

    music_on = GetBoolPreference(kMusic,true);
    sound_on = GetBoolPreference(kSound,true);

    music_volume = ((float)GetIntPreference(kMusicVolume, 100))/100.0f;
    sound_volume = ((float)GetIntPreference(kSoundVolume, 100))/100.0f;

	loadMusic(musicPackage);
	if (!music_on)
		m_audioManager->setMusicEnabled(false);
}

void AudioManager::close()
{
    if (!audio_supported) return;
    clearMusicCache();
    clearSoundCache();
}

AudioManager::AudioManager()
{
    gtNotifier.addListener(String(kMusicVolume), this);
    gtNotifier.addListener(String(kSoundVolume), this);
    gtNotifier.addListener(String(kMusic), this);
    gtNotifier.addListener(String(kSound), this);
}

AudioManager::~AudioManager()
{
    gtNotifier.removeListener(String(kMusicVolume), this);
    gtNotifier.removeListener(String(kSoundVolume), this);
    gtNotifier.removeListener(String(kMusic), this);
    gtNotifier.removeListener(String(kSound), this);
}

void AudioManager::notificationOccured(String identifier, void * context)
{
    if (identifier == kMusicVolume) {
        // musicVolume((float)*(int *)context);
    } else
        if (identifier == kSoundVolume) {
            soundVolume((float)*(int *)context);
        } else
            if (identifier == kMusic) {
                musicOnOff(*(bool *)context);
            } else
                if (identifier == kSound) {
                    soundOnOff(*(bool *)context);
                }
}

void AudioManager::clearSoundCache()
{
}

void AudioManager::clearMusicCache()
{
}

void AudioManager::preloadMusic(const MusicPackage &musicPackage)
{
    gtCommander->cacheMusic(FilePath("music").combine(musicPackage.getFileName().c_str()));
}

void AudioManager::loadMusic(const MusicPackage &musicPackage)
{
    GTLog("loadMusic");
	m_MusicPackage = &musicPackage;
    MusicRef music = gtCommander->getMusic(FilePath("music").combine(musicPackage.getFileName().c_str()));
    if (music.get() != NULL)
        m_audioManager->playMusic(music);
    GTLog("loadMusic OK");
}

void AudioManager::music(const char *command)
{
	if ((!music_on) || (!audio_supported) || (music_command == command)) {
        music_command = command;
        return;
    }
	music_command = command;
	int position = m_MusicPackage->getMusicPosition(music_command.c_str());
	if (position >= 0) {
		m_audioManager->setMusicEnabled(music_on);
		m_audioManager->setMusicPosition(position);
	}
	else {
		m_audioManager->setMusicEnabled(false);
	}
}

void AudioManager::stress(float f)
{
    if ((!music_on) || (!audio_supported) || (music_stress == f)) {
        music_stress = f;
        return;
    }
    music_stress = f;
    const MusicPackage::MusicLayerList &list = m_MusicPackage->getMusicStressLayers();
    for (int i=0; i<list.size(); ++i)
        m_audioManager->setMusicLayerVolume(list[i], f);
}

void AudioManager::relax(float f)
{
    if ((!music_on) || (!audio_supported) || (music_relax == f)) {
        music_relax = f;
        return;
    }
    music_relax = f;
    const MusicPackage::MusicLayerList &list = m_MusicPackage->getMusicRelaxLayers();
    for (int i=0; i<list.size(); ++i)
        m_audioManager->setMusicLayerVolume(list[i], f);
}
    
SoundRef AudioManager::preloadSound(const char *fileName, float volume)
{
    return gtCommander->getSound(FilePath("sfx").combine(fileName));
}

void AudioManager::playSound(const char *fileName, float volume, float balance, float pitch)
{
    double currentTime = ios_fc::getTimeMs();
    std::map<std::string, double>::iterator iter =
        lastUsedTimestamp.find(fileName);
    if ((iter != lastUsedTimestamp.end())
        && (currentTime - iter->second < TIMEMS_BETWEEN_SAME_SOUND))
        return;
    SoundRef sound   = gtCommander->getSound(FilePath("sfx").combine(fileName));
    m_audioManager->playSound(sound, volume, balance, pitch);
    lastUsedTimestamp[fileName] = currentTime;
}

void AudioManager::musicVolume(float volume)
{
    //if (!audio_supported) return;
    if (music_on) {
        m_audioManager->setMusicVolume(volume);
    }
    music_volume = volume;
}

void AudioManager::soundVolume(float volume)
{
    //if (!audio_supported) return;
    if (sound_on) {
        m_audioManager->setSoundVolume(volume);
    }
    sound_volume = volume;
}

const char * AudioManager::musicVolumeKey(void) { return kMusicVolume; }
const char * AudioManager::soundVolumeKey(void) { return kSoundVolume; }
const char * AudioManager::musicOnOffKey(void)  { return kMusic; }
const char * AudioManager::soundOnOffKey(void)  { return kSound; }

void AudioManager::musicOnOff(bool state)
{
  if ((!audio_supported) || (music_on == state)) return;
  music_on = state;
  m_audioManager->setMusicEnabled(state);

  if (music_on)
  {
    loadMusic(*m_MusicPackage);
    std::string lastCommand = music_command;
    music_command = ""; // Force music() to play this command.
    music(lastCommand.c_str());
      float copy_stress = music_stress;
      music_stress = -1.0f;
      stress(copy_stress);
      float copy_relax = music_relax;
      music_relax = -1.0f;
      relax(copy_relax);
  }
}

void AudioManager::soundOnOff(bool state)
{
  if ((!audio_supported) || (sound_on == state)) return;
  sound_on = state;
  m_audioManager->setSoundEnabled(state);
}

bool AudioManager::isMusicOn()
{
  return music_on;
}

bool AudioManager::isSoundOn()
{
  return sound_on;
}

}
