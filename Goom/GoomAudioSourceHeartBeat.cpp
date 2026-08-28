//
//  GoomAudioSourceHeartBeat.cpp
//  Project
//
//  Created by Jean-Christophe Hoelt on 2/6/12.
//  Copyright (c) 2012 Fovea.cc. All rights reserved.
//

#include "GoomAudioSourceHeartBeat.h"
#include <math.h>
#include "GameTools.h"

using namespace gametools;

GoomAudioSourceHeartBeat::GoomAudioSourceHeartBeat() {
    m_lastBeat = 0.0;
    m_amplitude = 0.0f;
}

void GoomAudioSourceHeartBeat::getSample(short sound[2][512]) const {
    
    float t = 1.0f;
    for (int i=0; i<2; ++i) {
        for (int j=0; j<512; ++j) {
            sound[i][j] = m_amplitude * 32767.0f * sinf(t);
            t += 0.05f * m_amplitude;
        }
    }
}

void GoomAudioSourceHeartBeat::idle(double currentTime) {
    float t = currentTime - m_lastBeat;

    if (t > 2.0f)
        m_lastBeat = currentTime;
    if (t > 1.1f) {
        m_lastBeat += 1.1;
        AudioManager::playSound("heartbeat.wav");
        t = currentTime - m_lastBeat;
    }
    
    const float peak1 = 0.075f;
    const float end1 = 0.21f;
    const float start2 = 0.39f;
    const float peak2 = 0.425f;
    const float end2 = 0.61f;
    
    if (t < peak1)
        m_amplitude = t / peak1;
    else if (t < end1)
        m_amplitude = (end1 - t) / (end1 - peak1);
    else if (t < start2)
        m_amplitude = 0.0f;
    else if (t < peak2)
        m_amplitude = (t - start2) / (peak2 - start2);
    else if (t < end2)
        m_amplitude = (end2 - t) / (end2 - peak2);
    else
        m_amplitude = 0.0f;
}