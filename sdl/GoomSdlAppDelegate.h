#ifndef GOOM_SDL_APP_DELEGATE_H
#define GOOM_SDL_APP_DELEGATE_H

#include "GameTools.h"
#include "GoomWidget.h"
#include "GoomAudioSourceHeartBeat.h"
#include <memory>
#include <string>
class GoomControlServer;

class GoomSdlAppDelegate : public gametools::GTApplicationDelegate,
                           public gametools::IdleComponent,
                           public GoomWidgetListener {
public:
    GoomSdlAppDelegate(int argc, char **argv);
    virtual ~GoomSdlAppDelegate();

    // GTApplicationDelegate
    virtual const gametools::CommanderDataPackage& initialDataPackage(const char *locale);
    virtual void mainScreenIsReady(gametools::Screen &screen);
    virtual void applicationDidFinishLaunching();

    // IdleComponent
    virtual void idle(double currentTime);

    // GoomWidgetListener
    virtual void onCloseGoomWidget();

private:
    gametools::Screen                    *m_screen;
    std::unique_ptr<GoomAudioSource>      m_audioSource;
    std::unique_ptr<GoomWidget>           m_goomWidget;
    std::string                           m_wavPath;
    bool                                  m_noMic;
    double                                m_exitAfter; // --exit-after N (seconds), 0 = off
    bool                                  m_control;     // --control[=port]
    bool                                  m_controlLan;  // --control-lan
    int                                   m_controlPort;
    std::unique_ptr<GoomControlServer>    m_controlServer;
};

#endif