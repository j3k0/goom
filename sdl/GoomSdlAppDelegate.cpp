#include "GoomSdlAppDelegate.h"
#include "GTLog.h"
#include "GoomAudioSourceSdlMic.h"
#include "GoomAudioSourceWav.h"
#include "GoomControlServer.h"
#include "ios_fc.h"
#include <string.h>

using namespace gametools;

GoomSdlAppDelegate::GoomSdlAppDelegate(int argc, char **argv)
    : m_screen(NULL), m_noMic(false), m_exitAfter(0.0)
, m_control(false), m_controlLan(false), m_controlPort(8090)
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc)
            m_wavPath = argv[++i];
        else if (strcmp(argv[i], "--no-mic") == 0)
            m_noMic = true;
        else if (strcmp(argv[i], "--exit-after") == 0 && i + 1 < argc)
            m_exitAfter = atof(argv[++i]);
        else if (strcmp(argv[i], "--control") == 0)
            m_control = true;
        else if (strncmp(argv[i], "--control=", 10) == 0) {
            m_control = true;
            int p = atoi(argv[i] + 10);
            if (p > 0 && p < 65536) m_controlPort = p;
        }
        else if (strcmp(argv[i], "--control-lan") == 0) {
            m_control = true;
            m_controlLan = true;
        }
    }
}
GoomSdlAppDelegate::~GoomSdlAppDelegate() {}

const CommanderDataPackage& GoomSdlAppDelegate::initialDataPackage(const char *locale) {
    (void)locale;
    static CommanderDataPackage data;

    // MusicPackage is dereferenced by Commander::initWithGUI even though
    // initAudio's body is compiled out. Supply an inert one.
    static MusicPackage music("");
    data.setMusicPackage(music);

    // initFonts unconditionally loads 3 fonts. goom-sdl never draws text;
    // the engine now null-guards missing font files, so no ttf is staged
    // and the font path is inert.
    static FontPackage font;
    font.setDefaultFont("gfx/MyriadWebPro.ttf");
    font.setSmallFont("gfx/MyriadWebPro.ttf");
    font.setFunnyFont("gfx/MyriadWebPro.ttf");
    font.setDefaultSize(10);
    font.setSmallSize(8);
    font.setFontActiveFX(Font_WHITE);
    font.setFontInactiveFX(Font_GREY);
    font.setFontTextFX(Font_WHITE);
    data.setFontPackage(font);

    return data;
}

void GoomSdlAppDelegate::mainScreenIsReady(Screen &screen) {
    m_screen = &screen;

    // Priority: --file > mic > heartbeat.
    const char *sourceName = "heartbeat";
    if (!m_wavPath.empty()) {
        try {
            m_audioSource.reset(new GoomAudioSourceWav(m_wavPath.c_str()));
            sourceName = "wav";
        } catch (const ios_fc::Exception &e) {
            GTLogf("%s", e.what() ? e.what() : "goom: WAV load failed");
            GTExit(1);
            return;
        }
    } else if (!m_noMic) {
        std::unique_ptr<GoomAudioSourceSdlMic> mic(new GoomAudioSourceSdlMic());
        if (mic->isAvailable()) {
            m_audioSource.reset(mic.release());
            sourceName = "mic";
        }
    }
    if (!m_audioSource)
        m_audioSource.reset(new GoomAudioSourceHeartBeat());
    GTLogf("goom: audio source: %s", sourceName);

    m_goomWidget.reset(new GoomWidget(*this, *m_audioSource,
                                      screen.getWidth(), screen.getHeight()));
    m_screen->add(m_goomWidget.get());
    if (m_control) {
        m_controlServer.reset(new GoomControlServer(
            m_goomWidget->getGoomInfo(), m_controlPort, m_controlLan));
    }
}

void GoomSdlAppDelegate::applicationDidFinishLaunching() {
    GameUIDefaults::GAME_LOOP->addIdle(this);
    gtCommander->disableScreenSaver();
}

void GoomSdlAppDelegate::idle(double currentTime) {
    m_audioSource->idle(currentTime);
    // The widget must idle itself: GoomWidget::idle advances m_currentTime
    // (which gates goom_update at 30 fps) and calls requestDraw(), the only
    // thing that makes GameLoop::run() redraw. Without this the loop draws
    // once at startup and then never again.
    m_goomWidget->idle(currentTime);
    if (m_controlServer)
        m_controlServer->tick();
    if (m_exitAfter > 0.0 && currentTime >= m_exitAfter) {
        GTLogf("goom: --exit-after reached, exiting");
        GTExit(0);
    }
}

void GoomSdlAppDelegate::onCloseGoomWidget() {
    GTExit(0);
}