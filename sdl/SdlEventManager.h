#ifndef SDLEVENTMANAGER_H
#define SDLEVENTMANAGER_H

#include "GameControls.h"
#include <deque>

namespace gametools {
namespace event_manager {

class SdlGameControlEvent : public GameControlEvent {
public:
    virtual GameControlEvent *clone() { return new SdlGameControlEvent(*this); }
};

class SdlEventManager : public EventManager
{
public:
    SdlEventManager();
    ~SdlEventManager();

    virtual GameControlEvent *createGameControlEvent() const { return new SdlGameControlEvent(); }

    void idle();

    virtual bool pollEvent(GameControlEvent &controlEvent);
    virtual void pushMouseEvent(unsigned int mouseId, int x, int y, CursorEventType type);
    virtual void pushKeyboardEvent(uint16_t c, KeyboardEventType type);
    void pushJoystickAxis(int x, int y);
    void pushJoystickButton(unsigned int buttonId, bool isUp);

    // Control settings handling
    virtual ios_fc::String getControlName(int controlType, bool alternate);
    virtual bool   changeControl(int controlType, bool alternate, GameControlEvent &event);
    virtual void   saveControls();

private:
    std::deque<SdlGameControlEvent> m_eventQueue;
};

}}

#endif // SDLEVENTMANAGER_H
