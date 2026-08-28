#ifndef _Commander_H
#define _Commander_H

#include "gameui.h"
#include "DataPathManager.h"
#include "LocalizedDictionary.h"
#include "GameCursor.h"
#include "ScreenTransition.h"
#include "FontPackage.h"
#include "CommanderResources.h"
#include "audio.h"
#include "GTStore.h"
#include "AchievementsManager.h"
#include <memory>

// using namespace gameui;

namespace gametools {
	
class GTApplicationState
{
public:
    enum UIState {
		IN_GAME, IN_MENU
	};
	GTApplicationState() : ui(IN_MENU), difficulty(0), level(0), playerScore(0) {}

	UIState ui;
	int difficulty;
	int level;
	int playerScore;
};

class CommanderDataPackage
{
public:
	CommanderDataPackage() {}
	void setMusicPackage(const MusicPackage &musicPackage) { m_musicPackage = &musicPackage; }
	const MusicPackage &getMusicPackage() const { return *m_musicPackage; }
	void setFontPackage(const FontPackage &fontPackage) { m_fontPackage = &fontPackage; }
	const FontPackage &getFontPackage() const { return *m_fontPackage; }
	
private:
	const MusicPackage *m_musicPackage;
	const FontPackage *m_fontPackage;
};

class Commander
{
  public:
    Commander(DataPathManager &dataPathManager, AchievementsManager &achievements);

	// Must be called first
	void initLocale(const char *defaultLocale = NULL);
    void initWithGUI(const CommanderDataPackage &dataPackage);
    void initWithoutGUI();

	void changeLocale(const char *newLocale);

    virtual ~Commander();
    // Transition widget factory
    virtual ScreenTransitionWidget *createScreenTransition(Screen &fromScreen) const;
    // Resource managers
    void cacheSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility = 0);
    IosSurfaceRef getSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility = 0);
    IosSurfaceRef getSurface(ImageType type, const char *path, const ImageOperationList &list);
    bool hasSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility = 0);
    void cacheFont(const char *path, int size, IosFontFx fx = Font_STD);
    IosFontRef getFont(const char *path, int size, IosFontFx fx = Font_STD);
    void cacheSound(const char *path);
    SoundRef getSound(const char *path);
    void cacheMusic(const char *path);
    MusicRef getMusic(const char *path);
	
    void freeUnusedResources();

	// Keep those resource loaded except for huge need
	void keepRef(const IosSurfaceRef &image);
	void keepRef(const SoundRef &sound);
	void freeKeptResources();
	
    // Data path management
    const DataPathManager &getDataPathManager() { return dataPathManager; }
    void setLocalizedString(const char *key, const char *value);
    const char * getLocalizedString(const char * originalString) const;
    const char * getLocaleName() const { return locale->getName(); }

    // Common resources accessor
    const ios_fc::String &getLocalizedFontName() const { return m_localizedFontName; }
	
    // param format: "key:value,key:value,key:value"
    virtual void notifyAnalytics(const char *level, const char *params = NULL) {}
    virtual void openURL(const char *url) {}
    virtual bool supportsFacebookShare() const { return false; }
	virtual void shareFacebook(const char *title, const char *text, const char *url) {}
    virtual bool supportsTwitterShare() const  { return false; }
	virtual void shareTwitter(const char *text) {}
    virtual bool hasReviewOnITunes() { return false; }
    virtual void forceReviewOnITunes(const char *title, const char *text, const char *url) {}
    virtual void reviewOnITunes(const char *title, const char *text, const char *url) {}
    virtual void disableScreenSaver() {}

	double getTimeMs() const { return GameUIDefaults::GAME_LOOP->getCurrentTime() * 1000.0; }
	double getTime() const   { return GameUIDefaults::GAME_LOOP->getCurrentTime(); }
    
    // Store Access
    bool hasStore() const { return m_store.get() != NULL; }
    GTStore &getStore() { return *m_store.get(); }
    
    // Achievements
    virtual const AchievementsManager &achievements() const { return m_achievements; }
    virtual AchievementsManager &achievements() { return m_achievements; }

  protected:
    // Resource manager factory
    virtual void createResourceManagers();
    IosSurfaceFactory m_surfaceFactory;
    std::auto_ptr<IosSurfaceResourceManager> m_surfaceResManager;
    IosFontFactory m_fontFactory;
    std::auto_ptr<IosFontResourceManager> m_fontResManager;
    SoundFactory m_soundFactory;
    std::auto_ptr<SoundResourceManager> m_soundResManager;
    MusicFactory m_musicFactory;
    std::auto_ptr<MusicResourceManager> m_musicResManager;
    // Data path management
    DataPathManager &dataPathManager;
    // Localization management
    LocalizedDictionary * locale;
    // Store - InAppPurchase
    std::auto_ptr<GTStore> m_store;
    // Achievements Manager
    AchievementsManager &m_achievements;

private:
    void initAudio(const MusicPackage &musicPackage);
    void initFonts(const FontPackage &fontPackage);

    GameLoop   *loop;
    AudioManager globalAudioManager;
	FontPackage m_fontPackage;

protected:
    ios_fc::String m_localizedFontName;
    IosFontRef m_funnyFont;
    SoundRef m_slideSound;
};

extern class Commander *gtCommander;

}
	
#endif // _Commander
