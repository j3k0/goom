#ifndef _AUDIO_H
#define _AUDIO_H

#include "audiomanager.h"
#include "NotifyCenter.h"
#include "MusicPackage.h"
#include "CommanderResources.h"

/// AUDIO
namespace gametools {

class AudioManager : public gametools::NotificationResponder
{
  public:

  AudioManager();
  ~AudioManager();

  static void init(const MusicPackage &musicPackage);
  static void close();

  static void preloadMusic(const MusicPackage &musicPackage);
  static void loadMusic(const MusicPackage &musicPackage);
  static void music(const char *command);
  static void stress(float f); // Set "stress" level.
  static void relax(float f); // Set "stress" level.
  static void clearMusicCache();

  static SoundRef preloadSound(const char *sName, float volume = 1.0);
  static void playSound(const char *sName, float volume = 1.0, float balance = 0.0f, float pitch = 1.0f);
  static void clearSoundCache();

  static const char * musicVolumeKey(void);
  static const char * soundVolumeKey(void);
  static const char * musicOnOffKey(void);
  static const char * soundOnOffKey(void);

  static void musicVolume(float volume); // Volume in [0.0f, 1.0f] interval
  static void soundVolume(float volume); // Volume in [0.0f, 1.0f] interval
  static void musicOnOff(bool state);
  static void soundOnOff(bool state);

  static bool isMusicOn();
  static bool isSoundOn();

	void notificationOccured(ios_fc::String identifier, void * context);

private:
    static audio_manager::AudioManager *m_audioManager;
	static const MusicPackage *m_MusicPackage;
};

}

#endif
