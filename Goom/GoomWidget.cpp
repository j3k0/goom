//
//  GoomWidget.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 2/5/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#include "GoomWidget.h"
#include "goom.h"
#include <cstdlib>
using namespace gametools;

GoomWidget::GoomWidget(GoomWidgetListener &listener, GoomAudioSource &source, int w, int h)
  : m_listener(listener), m_source(&source), m_rgbaBuffer(NULL), m_currentTime(0.0), m_nextFrame(0.0)
{
    setSize(Vec2(w,h));
    setFocusable(true);
    setReceiveUpEvents(true);

    // Experimental (or empirical) values for the resolution of Goom
    switch (GTGetPlatform()) {
        case GT_IPAD3:     m_goomW = 512; break;
        case GT_IPAD_MINI: m_goomW = 512; break; // Tested on device
        case GT_IPAD2:     m_goomW = 512; break; // Tested on device
        case GT_IPHONE4S:  m_goomW = 480; break;
        case GT_IPAD1:     m_goomW = 409; break;
        case GT_IPHONE4:   m_goomW = 427; break; // Tested on device
        case GT_IPHONE3GS: m_goomW = 320; break;
        case GT_IPHONE3G:  m_goomW = 240; break;
        default:           m_goomW = w / 2;
    }
    // Per-frame cost of the zoom filter + fx chain grows with the canvas:
    // the same build measured 15-20 fps at 1266x585 and a locked 60 fps at
    // 512 on an A15. Caps by platform: desktop machines have headroom for
    // more, mobile stays at 512 (the largest canvas the empirical table
    // ever chose). GOOM_RESOLUTION overrides the cap on any platform.
    int goomWCaps = 512;
    if (GTGetPlatform() == GT_DESKTOP)
        goomWCaps = 1024;
    const char *resEnv = getenv("GOOM_RESOLUTION");
    if (resEnv != NULL && *resEnv != '\0')
        goomWCaps = atoi(resEnv);
    if (m_goomW > goomWCaps)
        m_goomW = goomWCaps;
    if (GTPlatformIsSlowerThan(GT_IPHONE3G))
        m_goomW = 200;
    
    m_goomH = m_goomW * h / w;
    GTLogf("GoomWidget: canvas %dx%d (drawable %dx%d, cap %d)", m_goomW, m_goomH, w, h, goomWCaps);

    // Init Goom
    m_goom = goom_init(m_goomW, m_goomH);
    m_goomSurface.reset(GameUIDefaults::GAME_LOOP->getDrawContext()->getImageLibrary().createImage(IMAGE_RGBA, m_goomW, m_goomH));
    
    m_lastFrame = ios_fc::getTimeMs() / 1000.0;
    m_fps = 24.0f;
    m_firstUpdate = true;
    m_titleDirty = false;
}

GoomWidget::~GoomWidget() {
    goom_close(m_goom);
}

void GoomWidget::setTitleText(const char *t) {
    // Keep a copy: the caller's buffer (e.g. a string that changes when the
    // next track starts) may differ by the time the 30 fps update picks it up.
    // An empty title blanks the overlay: goom keeps drawing the last text it
    // was given until it expires, so a space is sent to clear it.
    std::string value = (t != NULL && t[0] != '\0') ? std::string(t) : std::string(" ");
    // Setting the title that is already current does nothing: goom animates a
    // title for its own 200 cycles and re-sending it resets that timer, which
    // would pin it on screen. This is what lets a host call this every frame.
    if (value == m_titleText) return;
    m_titleText = value;
    m_titleDirty = true;
}

void GoomWidget::eventOccured(gametools::event_manager::GameControlEvent *event) {
    if (event->cursorEvent == event_manager::kGameMouseUp)
        m_listener.onCloseGoomWidget();
#ifdef GOOM_SDL_DESKTOP
    if (event->keyboardEvent == event_manager::kKeyboardDown && event->unicodeKeySym == 27)
        m_listener.onCloseGoomWidget();
#endif
}

void GoomWidget::action(gametools::Widget *sender, int actionType, gametools::event_manager::GameControlEvent *event) {
    ActionContainer::action(sender, actionType, event);
}

void GoomWidget::idle(double currentTime) {
    ActionContainer::idle(currentTime);
    m_currentTime = currentTime;
    requestDraw();
}

void GoomWidget::updateGoom() {
    
    if (abs(m_currentTime - m_nextFrame) > 1.0)
        m_nextFrame = m_currentTime;
    
    if (m_currentTime > m_nextFrame) {
        // Compute FPS
        double t      = ios_fc::getTimeMs() / 1000.0;
        float  newFps = 1.0f / (t - m_lastFrame);
        m_fps         = m_fps * 0.9f + newFps * 0.1f;
        m_lastFrame   = t;
        
        // Update Goom
        short soundData[2][512];
        m_source->getSample(soundData);
        // A title set through setTitleText (and not shown yet) wins over the
        // first-update string, so apps that announce the track at startup get
        // their title shown instead of "Goom.2k26". goom copies the string,
        // so it is handed over once and kept here for the dedup in
        // setTitleText.
        const char *songTitle = NULL;
        if (m_titleDirty) {
            songTitle = m_titleText.c_str();
            m_titleDirty = false;
        }
        else if (m_firstUpdate) {
            songTitle = "Goom.2k26";
        }
        m_rgbaBuffer = (char *)goom_update(m_goom, soundData, 0, /*m_fps*/0,
                                           (char *)songTitle, NULL);
        m_firstUpdate = false;
        
        // Alpha Magic
        // TODO: have goom set the alpha value properly, or have a RGB texture and change the data alignment in opengl code
        int imax = m_goomW * m_goomH * 4;
        for (int i = 3 ; i < imax ; i += 4)
            m_rgbaBuffer[i] = 0xFF;
        m_nextFrame += 1.0 / 30.0;
    }
}

void GoomWidget::draw(gametools::DrawTarget *dt) {
    drawGoom(dt, IMAGE_COPY, 1, 1, 1, 1);
    ActionContainer::draw(dt);
}

void GoomWidget::drawGoom(gametools::DrawTarget *dt, gametools::ImageBlendMode mode, float r, float g, float b, float a) {
    updateGoom();
    if (m_rgbaBuffer) {
        m_goomSurface->setData(0, 0, m_goomW, m_goomH, m_rgbaBuffer);
        //m_goomSurface->setBlendMode(IMAGE_ADD);
        //dt->nextDrawAlpha(a);
        //dt->nextDrawColor(r,g,b);
        IosRect dest = getRect();
        dt->draw(m_goomSurface.get(), NULL, &dest);
    }
}
