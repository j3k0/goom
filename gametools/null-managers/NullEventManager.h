#ifndef NULLEVENTMANAGER_H
#define NULLEVENTMANAGER_H

#include "GameControls.h"

namespace gametools {
namespace event_manager {

class NullEventManager : public EventManager
{
public:
    virtual bool pollEvent(GameControlEvent &controlEvent) { return false; }
    virtual void pushMouseEvent(unsigned int mouseId, int x, int y, CursorEventType type) { }

    virtual ios_fc::String getControlName(int controlType, bool alternate) { return "None"; }
    virtual bool   changeControl(int controlType, bool alternate, GameControlEvent &event) { return true; }
    virtual void   saveControls() { }
    virtual Vec3 accelerationInX() const { return Vec3(0,0,0); } // Optional support for accelerometer
};

}
}

#endif // NULLEVENTMANAGER_H
