#include "SdlEventManager.h"

namespace gametools {
namespace event_manager {

SdlEventManager::SdlEventManager() {
}

SdlEventManager::~SdlEventManager() {
}

void SdlEventManager::idle() {
}

bool SdlEventManager::pollEvent(GameControlEvent &controlEvent)
{
    if (m_eventQueue.empty())
        return false;
    controlEvent = m_eventQueue.front();
    m_eventQueue.pop_front();
    return true;
}

void SdlEventManager::pushMouseEvent(unsigned int mouseId, int x, int y, CursorEventType type) {
    SdlGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = type;
    newEvent.keyboardEvent = kKeyboardNone;
    newEvent.isUp = (type == kGameMouseUp);
    newEvent.isJoystick = false;
    newEvent.caught = false;
    newEvent.x = x;
    newEvent.y = y;
    newEvent.unicodeKeySym = 0;
    newEvent.mouseId = mouseId;
    m_eventQueue.push_back(newEvent);
}

void SdlEventManager::pushKeyboardEvent(uint16_t c, KeyboardEventType type) {
    SdlGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = kCursorNone;
    newEvent.keyboardEvent = type;
    newEvent.isUp = (type == kKeyboardUp);
    newEvent.isJoystick = false;
    newEvent.caught = false;
    newEvent.x = 0;
    newEvent.y = 0;
    newEvent.unicodeKeySym = c;
    newEvent.keySym = c;
    newEvent.mouseId = 0;
    m_eventQueue.push_back(newEvent);
}

void SdlEventManager::pushJoystickAxis(int x, int y) {
    SdlGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = kJoystickAxis;
    newEvent.keyboardEvent = kKeyboardNone;
    newEvent.isUp = false;
    newEvent.isJoystick = true;
    newEvent.caught = false;
    newEvent.x = x;
    newEvent.y = y;
    newEvent.unicodeKeySym = 0;
    newEvent.mouseId = 0;
    m_eventQueue.push_back(newEvent);
}

void SdlEventManager::pushJoystickButton(unsigned int buttonId, bool isUp) {
    SdlGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = kJoystickButton;
    newEvent.keyboardEvent = kKeyboardNone;
    newEvent.isUp = isUp;
    newEvent.isJoystick = true;
    newEvent.caught = false;
    newEvent.x = 0;
    newEvent.y = 0;
    newEvent.unicodeKeySym = 0;
    newEvent.buttonId = buttonId;
    newEvent.mouseId = 0;
    m_eventQueue.push_back(newEvent);
}

// Control settings handling
ios_fc::String SdlEventManager::getControlName(int controlType, bool alternate) {
    return "";
}

bool SdlEventManager::changeControl(int controlType, bool alternate, GameControlEvent &event) {
    return true;
}

void SdlEventManager::saveControls() {
}

}}
