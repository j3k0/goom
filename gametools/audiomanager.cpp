//
//  audiomanager.cpp
//  Project
//
//  Created by Florent Boudet on 11/03/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "audiomanager.h"

namespace gametools {
    
    int GTAudioPolicy = audio_manager::UsesOutput;

    void GTSetAudioPolicy(int policy)
    {
        GTAudioPolicy = policy;
    }
    
    // Return pitch for given number of semi-tones.
    float GTSemiTone(float num) {
        if (num > 0.1f) {
            float pitch = 1.0f;
            while (num > 0.1f) {
                pitch *= 1.05946f;
                num -= 1.0f;
            }
            return pitch;
        }
        else if (num < -0.1f) {
            float pitch = 1.0f;
            while (num < -0.1f) {
                pitch *= 0.94387f;
                num += 1.0f;
            }
            return pitch;
        }
        return 1.0f;
    }

}