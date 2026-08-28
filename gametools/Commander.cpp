/* strings to translate */

#include "Commander.h"
#include "GTPreferences.h"
#include "audio.h"
#include "GTLog.h"

#ifdef THREADED_RESOURCE_MANAGER
#include "ThreadedResourceManager.h"
#endif
using namespace gametools;
using namespace event_manager;
using namespace ios_fc;

namespace gametools {
void GTLog(const char *txt);
Commander *gtCommander = NULL;
IosFont *storyFont; // TODO: remove

#ifdef PRODUCE_CACHE_FILE
FILE *cacheOutputGsl;
#endif

IosSurface *IosSurfaceFactory::create(const IosSurfaceResourceKey &resourceKey)
{
    try {
#ifdef PRODUCE_CACHE_FILE
        fprintf(cacheOutputGsl, "  [cache_picture: path=\"%s\" mode=%d]\n",
                resourceKey.path.c_str(), resourceKey.type);
#endif
        ImageLibrary &iimLib = GameUIDefaults::GAME_LOOP->getDrawContext()->getImageLibrary();
        IosSurface *newSurface = iimLib.loadImage(resourceKey.type, resourceKey.path.c_str(), resourceKey.specialAbility);
        if (newSurface != NULL)
            newSurface->enableExceptionOnDeletion(true);
        return newSurface;
    }
    catch (Exception e) {
        return NULL;
    }
}

void IosSurfaceFactory::destroy(IosSurface *res)
{
    res->enableExceptionOnDeletion(false);
    delete res;
}

IosFont *IosFontFactory::create(const IosFontResourceKey &resourceKey)
{
#ifdef PRODUCE_CACHE_FILE
    fprintf(cacheOutputGsl, "  [cache_picture: path=\"%s\" mode=%d]\n",
            resourceKey.path.c_str(), resourceKey.type);
#endif
    GTLogf("Trying to create font %s [0x%x]", resourceKey.path.c_str(), &m_dataPathManager);
    if (m_dataPathManager.hasDataInputStream(resourceKey.path.c_str())) {
        ImageLibrary &iimLib = GameUIDefaults::GAME_LOOP->getDrawContext()->getImageLibrary();
        IosFont *newFont = iimLib.createFont(resourceKey.path.c_str(), resourceKey.size, resourceKey.fx);
        return newFont;
    }
    else {
        GTLogf("File not found: %s", resourceKey.path.c_str());
        return NULL;
    }
}

void IosFontFactory::destroy(IosFont *res)
{
    delete res;
}

audio_manager::Sound * SoundFactory::create(const std::string &path)
{
#ifdef PRODUCE_CACHE_FILE
    fprintf(cacheOutputGsl, "  [cache_sound: path=\"%s\"]\n", path.c_str());
#endif
    if (m_dataPathManager.hasDataInputStream(path.c_str())) {
        audio_manager::Sound *newSound = GameUIDefaults::GAME_LOOP->getAudioManager()->loadSound(path.c_str());
        return newSound;
    }
    else {
        GTLogf("File not found: %s", path.c_str());
        return NULL;
    }
}

void SoundFactory::destroy(audio_manager::Sound *res)
{
    delete res;
}

audio_manager::Music * MusicFactory::create(const std::string &path)
{
#ifdef PRODUCE_CACHE_FILE
    fprintf(cacheOutputGsl, "  [cache_music: path=\"%s\"]\n", path.c_str());
#endif
    if (m_dataPathManager.hasDataInputStream(path.c_str())) {
        // String fullPath = m_dataPathManager.getPath(path.c_str());
        audio_manager::Music *newMusic = GameUIDefaults::GAME_LOOP->getAudioManager()->loadMusic(path.c_str());
        return newMusic;
    }
    else {
        GTLogf("File not found: %s", path.c_str());
        return NULL;
    }
}

void MusicFactory::destroy(audio_manager::Music *res)
{
    delete res;
}


/* Build the Commander */

Commander::Commander(DataPathManager &dataPathManager, AchievementsManager &achievements)
  : m_surfaceFactory(dataPathManager),
	m_fontFactory(dataPathManager),
    m_soundFactory(dataPathManager),
	m_musicFactory(dataPathManager),
	dataPathManager(dataPathManager),
    m_achievements(achievements)
{
#ifdef PRODUCE_CACHE_FILE
  cacheOutputGsl = fopen("cache.gsl", "w");
#endif
  loop = GameUIDefaults::GAME_LOOP;
  gtCommander = this;


  createResourceManagers();
}

void Commander::initWithGUI(const CommanderDataPackage &data)
{
  // initLocale();
  initAudio(data.getMusicPackage());
  initFonts(data.getFontPackage());
}
/*
void Commander::initWithoutGUI()
{
  initLocale();
}
*/
Commander::~Commander()
{
}

extern char *dataFolder;
#include <string>

/* Initialise the default dictionnary */
void Commander::initLocale(const char *defaultLocale)
{
    Locales_SetDefault(defaultLocale);
    locale = new LocalizedDictionary(dataPathManager, "locale", "main");
}

/* Global translator */
const char * Commander::getLocalizedString(const char * originalString) const
{
  return locale->getLocalizedString(originalString);
}
    
void Commander::setLocalizedString(const char *key, const char *value)
{
    locale->setLocalizedString(key, value);
}


/* Initialize the audio if necessary */
void Commander::initAudio(const MusicPackage &musicPackage)
{
#if 0
	// Deactivated by Jeko on 2011-03-03
    m_slideSound = getSound(FilePath("sfx").combine("slide.wav"));
    GameUIDefaults::SLIDE_SOUND = m_slideSound;
#endif
    AudioManager::init(musicPackage);
}


/* load fonts and set them for use in the GUI */
void Commander::initFonts(const FontPackage &fontPackage)
{
    GTLogf("Commander::initFonts");
    GTLogf("  >> Init locales");
    Locales_Init(); // Make sure locales are detected.
    GTLogf("  >> Compute localized font name");
    m_localizedFontName = /*locale->getLocalizedString(*/fontPackage.getDefaultFont().c_str()/*)*/;
    GTLogf("  >> Load font %s", fontPackage.getDefaultFont().c_str());
    GameUIDefaults::FONT = getFont(/*locale->getLocalizedString(*/fontPackage.getDefaultFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontActiveFX());
	GameUIDefaults::FONT_INACTIVE = getFont(/*locale->getLocalizedString(*/fontPackage.getDefaultFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontInactiveFX());
	GameUIDefaults::FONT_TEXT = getFont(/*locale->getLocalizedString(*/fontPackage.getDefaultFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontTextFX());
    GTLogf("  >> Load font %s", fontPackage.getSmallFont().c_str());
	GameUIDefaults::FONT_SMALL_ACTIVE = getFont(/*locale->getLocalizedString(*/fontPackage.getSmallFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontActiveFX());
	GameUIDefaults::FONT_SMALL_INFO = getFont(/*locale->getLocalizedString(*/fontPackage.getSmallFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontTextFX());
    GTLogf("  >> Load font %s", fontPackage.getFunnyFont().c_str());
	GameUIDefaults::FONT_FUNNY = getFont(/*locale->getLocalizedString(*/fontPackage.getFunnyFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontTextFX());
    m_funnyFont = getFont(/*locale->getLocalizedString(*/fontPackage.getFunnyFont().c_str()/*)*/, fontPackage.getDefaultSize(), fontPackage.getFontTextFX());
    storyFont   = GameUIDefaults::FONT_SMALL_INFO;
	m_fontPackage = fontPackage;
    GTLogf("  >> DONE");
}
	
void Commander::changeLocale(const char *newLocale) {
	Locales_Init(newLocale);
	initLocale();
}

ScreenTransitionWidget *Commander::createScreenTransition(Screen &fromScreen) const
{
    // DoomMelt (and all screen transitions) were removed with the goom-sdl
    // trim: goom-sdl never triggers transitions. Return NULL if ever called.
    (void)fromScreen;
    return NULL;
}

// Resource management
void Commander::cacheSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility)
{
    m_surfaceResManager->cacheResource(IosSurfaceResourceKey(type, path, specialAbility));
}

IosSurfaceRef Commander::getSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility)
{
	if (path[0] == 0) return NULL;
    IosSurfaceRef ref = m_surfaceResManager->getResource(IosSurfaceResourceKey(type, path, specialAbility));
	if (ref.get() == NULL) {
		GTLog((std::string("Warning: image file not found: ") + path).c_str());
	}
	return ref;
}
    
bool Commander::hasSurface(ImageType type, const char *path, ImageSpecialAbility specialAbility) {
    if (path[0] == 0) return false;
    return m_surfaceResManager->hasResource(IosSurfaceResourceKey(type, path, specialAbility));
}

IosSurfaceRef Commander::getSurface(ImageType type, const char *path, const ImageOperationList &list)
{
	if (path[0] == 0) return NULL;
    ImageSpecialAbility specialAbility = GameUIDefaults::GAME_LOOP->getDrawContext()->guessRequiredImageAbility(list);
    return m_surfaceResManager->getResource(IosSurfaceResourceKey(type, path, specialAbility));
}

void Commander::cacheFont(const char *path, int size, IosFontFx fx)
{
//    m_fontResManager->cacheResource(IosFontResourceKey(path, size, fx));
	m_fontResManager->getResource(IosFontResourceKey(path, size, fx));
}

IosFontRef Commander::getFont(const char *path, int size, IosFontFx fx)
{
    return m_fontResManager->getResource(IosFontResourceKey(path, size, fx));
}

void Commander::cacheSound(const char *path)
{
//    m_soundResManager->cacheResource(path);
	m_soundResManager->getResource(path);
}

SoundRef Commander::getSound(const char *path)
{
    return m_soundResManager->getResource(path);
}

void Commander::cacheMusic(const char *path)
{
    m_musicResManager->cacheResource(path);
}

MusicRef Commander::getMusic(const char *path)
{
    return m_musicResManager->getResource(path);
}

void Commander::freeUnusedResources()
{
    static int    warningLevel = 0;
	static double lastWarningMs = -5001.0;
	double t = getTimeMs();
	if (t - lastWarningMs < 5000.0) {
		printf("Got 2 memory warnings in a row. Freeing kept resources.\n");
		freeKeptResources();
        // Update warning level.
        warningLevel ++;
        if (warningLevel >= 3) {
            printf("3 warnings in a row... I guess we can't do anything about it.\n");
            lastWarningMs = t + 45000.0f; // Ignore all warnings for 45 seconds.
            return;
        }
	}
    else {
        warningLevel = 0;
    }
    
	lastWarningMs = t;
	
    m_surfaceResManager->freeUnusedResources();
    //m_fontResManager->freeUnusedResources();
    m_soundResManager->freeUnusedResources();
    //m_musicResManager->freeUnusedResources();
}

static std::vector<IosSurfaceRef> g_keptImages;
static std::vector<SoundRef> g_keptSounds;

// Keep those resource loaded except for huge need
void Commander::keepRef(const IosSurfaceRef &image) {
	for (unsigned int i=0; i<g_keptImages.size(); ++i)
		if (g_keptImages[i].get() == image.get()) return;
	g_keptImages.push_back(image);
}
void Commander::keepRef(const SoundRef &sound) {
	for (unsigned int i=0; i<g_keptSounds.size(); ++i)
		if (g_keptSounds[i].get() == sound.get()) return;
	g_keptSounds.push_back(sound);
}
void Commander::freeKeptResources() {
	g_keptImages.clear();
	g_keptSounds.clear();
}
	

void Commander::createResourceManagers()
{
#ifdef THREADED_RESOURCE_MANAGER
    m_surfaceResManager.reset(new ThreadedResourceManager<IosSurface, IosSurfaceResourceKey>(m_surfaceFactory));
    m_fontResManager.reset(new ThreadedResourceManager<IosFont, IosFontResourceKey>(m_fontFactory));
    m_soundResManager.reset(new ThreadedResourceManager<audio_manager::Sound>(m_soundFactory));
    m_musicResManager.reset(new ThreadedResourceManager<audio_manager::Music>(m_musicFactory));
#else
    m_surfaceResManager.reset(new SimpleResourceManager<IosSurface, IosSurfaceResourceKey>(m_surfaceFactory));
    m_fontResManager.reset(new SimpleResourceManager<IosFont, IosFontResourceKey>(m_fontFactory));
    m_soundResManager.reset(new SimpleResourceManager<audio_manager::Sound>(m_soundFactory));
    m_musicResManager.reset(new SimpleResourceManager<audio_manager::Music>(m_musicFactory));
#endif
}

}
