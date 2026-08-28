//
// SDL2 desktop platform binding — entry point and main loop.
//
// Plays the role of macosx/OsxMain.mm + OsxMainControler.mm: owns the
// window, the GL context, the event pump and the frame loop, and
// provides GTInit/GTMain/GTExit/GTPaused for the generic game main()
// (e.g. AliHood/AliMain.cpp).
//
// Options:
//   --fullscreen          borderless fullscreen at desktop resolution
//   --size WxH            windowed mode size (default 1280x800)
//   --resources <dir>     resources root (default: next to the executable);
//                         game data is read from <dir>/data, cloud saves
//                         fall back to <dir>/<Game>-Cloud/
//

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_STEAM
#include "steam/steam_api.h"
#ifndef STEAM_APPID
#define STEAM_APPID 480 // Spacewar test appid; override in the Makefile
#endif
#endif

#include "GTPlatform.h"
#include "GTPreferences.h"
#include "GameTools.h"
#include "LocalAchievementsManager.h"
#include "GTAtlasLoader.h"
#include "GTTestGroup.h"
#include "FPDataPathManager.h"

#include "SdlGraphics.h"
#include "SdlEventManager.h"
#include "SdlAudioManager.h"

using namespace gametools;
using namespace gametools::event_manager;
using namespace gametools::audio_manager;
using namespace gametools::graphics_manager;

namespace gametools {
    // SdlGtCloud.cpp
    void SdlGtCloudSetPaths(const char *userDir, const char *resourcesDir);
}

#ifndef SDL_GAME_TITLE
#define SDL_GAME_TITLE "Ali Hood"
#endif
#ifndef SDL_GAME_ORG
#define SDL_GAME_ORG "Fovea"
#endif
#ifndef SDL_GAME_NAME
#define SDL_GAME_NAME "AliHood"
#endif

static bool gAppPaused = false;
static bool gAppRunning = true;
static bool gIsInitialized = false;

static SDL_Window     *gWindow = NULL;
static SDL_GLContext   gGlContext = NULL;

static SdlOpenGlDrawContext *gOpenGlDrawContext = NULL;
static SdlDrawContext       *gDrawContext = NULL;
static SdlAudioManager      *gAudioManager = NULL;
static SdlEventManager      *gEventManager = NULL;
static Screen               *gMainScreen = NULL;
static FPDataPathManager    *gDataPathManager = NULL;
static LocalAchievementsManager *gAchievements = NULL;

static bool gOptFullscreen = false;
static int  gOptWidth  = 1280;
static int  gOptHeight = 800;
static const char *gOptResources = NULL;

#ifdef GOOM_SDL_DESKTOP
// Set by the 's' shortcut, drained by the main loop once the frame is drawn.
static bool gScreenshotRequested = false;

static bool fileExists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

// Save the current frame to ~/Desktop/goom-<timestamp>.bmp (cwd fallback).
static void saveScreenshot()
{
    char stamp[32];
    time_t now = time(NULL);
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", localtime(&now));

    const char *home = getenv("HOME");
    char dir[1024];
    if (home != NULL && *home != '\0')
        snprintf(dir, sizeof(dir), "%s/Desktop", home);
    else
        snprintf(dir, sizeof(dir), ".");

    // Shots within the same second would collide; suffix them -2, -3, ...
    char path[1280];
    snprintf(path, sizeof(path), "%s/goom-%s.bmp", dir, stamp);
    for (int i = 2; fileExists(path) && i < 1000; ++i)
        snprintf(path, sizeof(path), "%s/goom-%s-%d.bmp", dir, stamp, i);

    if (SdlGraphics_SaveScreenshot(path))
        GTLogf("goom: screenshot saved: %s", path);
    else
        GTLogf("goom: screenshot failed: %s", path);
}
#endif // GOOM_SDL_DESKTOP

static void setPaused(bool paused) {
    if (!gIsInitialized) return;
    if (paused == gAppPaused) return;
    gAppPaused = paused;
    if (paused && gtApp) gtApp->applicationWillGoInactive();
}

static void initGame(const char *resourcesDir, float w, float h)
{
    // No A/B testing on desktop. Random groups can also pick level
    // variants missing from the shipped data (e.g. group 'a' wants
    // MainQuest-Introduction-145, only -143 is bundled).
    GTSetTestGroup(TESTGROUP_NONE);

    if (h > 400.0f)
        GTSetScaleFactor(2.0f * h / 768.0f);
    GTSetPhysicalSize(0.5f * (w + h) / 2730.0f);

    GTLog("Creating Data Manager");
    GTLog(resourcesDir);
    gDataPathManager = new FPDataPathManager(ios_fc::String(resourcesDir) + "/data");
    GTLog("Creating Draw Context");
    gOpenGlDrawContext = new SdlOpenGlDrawContext(gDataPathManager, w, h);
    gDrawContext = new SdlDrawContext(gOpenGlDrawContext);
    GTLog("Loading Atlas");
    GTAtlasLoader(gDataPathManager).load("gfx/atlas.txt", gDrawContext);
    GTLog("Creating Event Manager");
    gEventManager = new SdlEventManager();
    GTLog("Creating Audio Manager");
    gAudioManager = new SdlAudioManager(gDataPathManager);
    gAchievements = new LocalAchievementsManager();

    GameUIDefaults::GAME_LOOP->setDrawContext(gDrawContext);
    GameUIDefaults::GAME_LOOP->setAudioManager(gAudioManager);
    GameUIDefaults::GAME_LOOP->setEventManager(gEventManager);

    gtCommander = new Commander(*gDataPathManager, *gAchievements);
    gtCommander->initLocale();

    const CommanderDataPackage &dp = gtApp->initialDataPackage(gtCommander->getLocaleName());
    gtCommander->initWithGUI(dp);

    gMainScreen = new Screen(0, 0, w, h, GameUIDefaults::GAME_LOOP);
    gtApp->mainScreenIsReady(*gMainScreen);
    GTGetOverlayManager().setOutputRect(0, 0, w, h);

    gtApp->applicationDidFinishLaunching();
    GameUIDefaults::SCREEN_STACK->push(gMainScreen);
    gIsInitialized = true;
}

static uint16_t mapKeysym(const SDL_Keysym &ks)
{
    switch (ks.sym) {
        case SDLK_UP:        return kKeyUp;
        case SDLK_DOWN:      return kKeyDown;
        case SDLK_LEFT:      return kKeyLeft;
        case SDLK_RIGHT:     return kKeyRight;
        case SDLK_BACKSPACE: return kKeyDelete;
        case SDLK_DELETE:    return kKeyDelete;
        case SDLK_PAGEUP:    return kKeyPageUp;
        case SDLK_PAGEDOWN:  return kKeyPageDown;
        case SDLK_KP_ENTER:  return '\r';
        default:
            if (ks.sym > 0 && ks.sym < 0x80)
                return (uint16_t)ks.sym;
            return 0;
    }
}

// Single game controller, like the Cocoa binding's single DDHid joystick.
static SDL_GameController *gController = NULL;
static int gJoyX = 0, gJoyY = 0;

static void openFirstController()
{
    if (gController != NULL) return;
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            gController = SDL_GameControllerOpen(i);
            if (gController) {
                GTLogf("Game controller connected: %s", SDL_GameControllerName(gController));
                break;
            }
        }
    }
}

static void handleControllerAxis(const SDL_ControllerAxisEvent &ev)
{
    int v = (int)ev.value * kJoystickAxisMax / 32768;
    if (ev.axis == SDL_CONTROLLER_AXIS_LEFTX) gJoyX = v;
    else if (ev.axis == SDL_CONTROLLER_AXIS_LEFTY) gJoyY = v;
    else return;
    gEventManager->pushJoystickAxis(gJoyX, gJoyY);
}

static void handleEvent(const SDL_Event &event)
{
    switch (event.type) {
        case SDL_QUIT:
            gAppRunning = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (gIsInitialized && event.button.button == SDL_BUTTON_LEFT) {
                float lx, ly;
                SdlGraphics_WindowToLogical((float)event.button.x, (float)event.button.y, &lx, &ly);
                gEventManager->pushMouseEvent(0, (int)lx, (int)ly, kGameMouseDown);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (gIsInitialized && event.button.button == SDL_BUTTON_LEFT) {
                float lx, ly;
                SdlGraphics_WindowToLogical((float)event.button.x, (float)event.button.y, &lx, &ly);
                gEventManager->pushMouseEvent(0, (int)lx, (int)ly, kGameMouseUp);
            }
            break;
        case SDL_MOUSEMOTION:
            if (gIsInitialized && (event.motion.state & SDL_BUTTON_LMASK)) {
                float lx, ly;
                SdlGraphics_WindowToLogical((float)event.motion.x, (float)event.motion.y, &lx, &ly);
                gEventManager->pushMouseEvent(0, (int)lx, (int)ly, kGameMouseMoved);
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            if (!gIsInitialized) break;
            const SDL_Keysym &ks = event.key.keysym;
            // Alt+Enter and F11 toggle fullscreen (routed through the same
            // preference the in-game switch uses; the main loop applies it).
            if (event.type == SDL_KEYDOWN &&
                (ks.sym == SDLK_F11 ||
                 (ks.sym == SDLK_RETURN && (ks.mod & KMOD_ALT))
#ifdef GOOM_SDL_DESKTOP
                 // goom: plain 'f' mirrors F11. The repeat guard keeps a held
                 // key from oscillating the window mode.
                 || (ks.sym == SDLK_f && !event.key.repeat &&
                     (ks.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0)
#endif
                 )) {
                bool fs = GetBoolPreference("AliHood.Fullscreen", false);
                SetBoolPreference("AliHood.Fullscreen", !fs);
                break;
            }
#ifdef GOOM_SDL_DESKTOP
            // goom: plain 's' saves a screenshot to ~/Desktop. Deferred to
            // the main loop so the readback sees a fully drawn frame.
            if (event.type == SDL_KEYDOWN && ks.sym == SDLK_s &&
                !event.key.repeat &&
                (ks.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0) {
                gScreenshotRequested = true;
                break;
            }
            // goom: plain 'p' toggles pause — the only pause control now that
            // losing window focus no longer pauses the app.
            if (event.type == SDL_KEYDOWN && ks.sym == SDLK_p &&
                !event.key.repeat &&
                (ks.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0) {
                setPaused(!gAppPaused);
                break;
            }
#endif
            uint16_t c = mapKeysym(ks);
            if (c != 0)
                gEventManager->pushKeyboardEvent(c,
                    event.type == SDL_KEYDOWN ? kKeyboardDown : kKeyboardUp);
            break;
        }
        case SDL_CONTROLLERDEVICEADDED:
            openFirstController();
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (gController && event.cdevice.which ==
                    SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gController))) {
                SDL_GameControllerClose(gController);
                gController = NULL;
                openFirstController();
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (gIsInitialized) handleControllerAxis(event.caxis);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (gIsInitialized) gEventManager->pushJoystickButton(event.cbutton.button, false);
            break;
        case SDL_CONTROLLERBUTTONUP:
            if (gIsInitialized) gEventManager->pushJoystickButton(event.cbutton.button, true);
            break;
        case SDL_WINDOWEVENT:
#ifndef GOOM_SDL_DESKTOP
            // AliHood keeps the Cocoa-style focus pause. goom doesn't: the
            // app keeps rendering while unfocused, and 'p' is the pause.
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                setPaused(true);
            else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                setPaused(false);
#endif
            break;
    }
}

static void parseArgs(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--fullscreen") == 0) {
            gOptFullscreen = true;
        }
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            int w = 0, h = 0;
            if (sscanf(argv[++i], "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
                gOptWidth = w;
                gOptHeight = h;
            }
        }
        else if (strcmp(argv[i], "--resources") == 0 && i + 1 < argc) {
            gOptResources = argv[++i];
        }
    }
}

namespace gametools {

void GTInit(int argc, char *argv[])
{
    GTSetPlatform(GT_DESKTOP);
    GTSetPlatformVersion(1.0f);
    parseArgs(argc, argv);
}

int GTMain(int argc, char *argv[])
{
#ifdef HAVE_STEAM
    if (SteamAPI_RestartAppIfNecessary(STEAM_APPID))
        return 1;
    if (!SteamAPI_Init())
        fprintf(stderr, "SteamAPI_Init failed (no Steam client?), continuing without Steam\n");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    std::string resources;
    if (gOptResources != NULL) {
        resources = gOptResources;
    }
    else {
        char *base = SDL_GetBasePath();
        resources = base ? base : ".";
        SDL_free(base);
    }
    char *prefPath = SDL_GetPrefPath(SDL_GAME_ORG, SDL_GAME_NAME);
    SdlGtCloudSetPaths(prefPath ? prefPath : ".", resources.c_str());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // The persisted preference is the single source of truth for the in-game
    // switch (AliHood.Fullscreen). The --fullscreen launch flag seeds it on
    // first run so the switch and the window always agree.
    if (gOptFullscreen)
        SetBoolPreference("AliHood.Fullscreen", true);
    gOptFullscreen = GetBoolPreference("AliHood.Fullscreen", false);
    Uint32 flags = SDL_WINDOW_OPENGL;
    int winW = gOptWidth, winH = gOptHeight;
    if (gOptFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(0, &mode) == 0) {
            winW = mode.w;
            winH = mode.h;
        }
    }

    gWindow = SDL_CreateWindow(SDL_GAME_TITLE,
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               winW, winH, flags);
    if (gWindow == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    gGlContext = SDL_GL_CreateContext(gWindow);
    if (gGlContext == NULL) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    SdlOpenGlContext_setWindow(gWindow);

    int drawW = winW, drawH = winH;
    SDL_GL_GetDrawableSize(gWindow, &drawW, &drawH);

    openFirstController();
    initGame(resources.c_str(), (float)drawW, (float)drawH);

    // The engine renders into its own FBO at the fixed logical size; tell the
    // GL context wrapper so presentRenderbuffer can blit it letterboxed to the
    // real drawable (which changes when the window goes fullscreen).
    gOpenGlDrawContext->setupScreenBlit();

    // Apply fullscreen changes live: the in-game switch / Alt+Enter / F11 only
    // flip the preference; here we reconcile it with the actual window state.
    bool currentFullscreen =
        (SDL_GetWindowFlags(gWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;

    while (gAppRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handleEvent(event);

        if (gAppPaused) {
            SDL_Delay(50);
            continue;
        }

        // Reconcile the fullscreen preference with the window (live toggle).
        bool wantFullscreen = GetBoolPreference("AliHood.Fullscreen", currentFullscreen);
        if (wantFullscreen != currentFullscreen) {
            SDL_SetWindowFullscreen(gWindow,
                wantFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            currentFullscreen = wantFullscreen;
        }
        gEventManager->idle();
        double currentTime = GameUIDefaults::GAME_LOOP->getCurrentTime();
        GameUIDefaults::GAME_LOOP->idle(currentTime);
        GameUIDefaults::GAME_LOOP->draw(true);
#ifdef GOOM_SDL_DESKTOP
        // After draw(): the engine FBO holds the full frame (it persists
        // across the swap, unlike the window back buffer).
        if (gScreenshotRequested) {
            gScreenshotRequested = false;
            saveScreenshot();
        }
#endif
#ifdef HAVE_STEAM
        SteamAPI_RunCallbacks();
#endif
    }

    if (gAudioManager) gAudioManager->stop();
#ifdef HAVE_STEAM
    SteamAPI_Shutdown();
#endif
    SDL_GL_DeleteContext(gGlContext);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
    return 0;
}

void GTExit(int exitCode) {
    if (gAudioManager) gAudioManager->stop();
    SDL_Quit();
    exit(exitCode);
}

bool GTPaused() {
    return gAppPaused;
}

}
