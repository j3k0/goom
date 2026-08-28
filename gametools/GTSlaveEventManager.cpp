//
//  IPhoneEventManager.mm
//  flobopop
//
//  Created by Florent Boudet on 15/11/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#include "GTSlaveEventManager.h"

using namespace gametools;
using namespace gametools::event_manager;

SlaveGameControlEvent::SlaveGameControlEvent()
{
}

GameControlEvent *SlaveGameControlEvent::clone()
{
    return new SlaveGameControlEvent(*this);
}

GameControlEvent *SlaveEventManager::createGameControlEvent() const
{
    return new SlaveGameControlEvent;
}

bool SlaveEventManager::pollEvent(GameControlEvent &controlEvent)
{
    if (m_eventQueue.empty())
        return false;
    controlEvent = m_eventQueue.front();
    m_eventQueue.pop_front();
    return true;
}

void SlaveEventManager::pushMouseEvent(unsigned int mouseId, int x, int y, CursorEventType type)
{
    SlaveGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = type;
    newEvent.keyboardEvent = kKeyboardNone;
    if (type == kGameMouseUp)
        newEvent.isUp = true;
    else newEvent.isUp = false;
    newEvent.isJoystick = false;
	newEvent.caught = false;
    newEvent.x = x; newEvent.y = y;
    newEvent.unicodeKeySym = 0;
    newEvent.mouseId = mouseId;
    m_eventQueue.push_back(newEvent);
}

void SlaveEventManager::pushCursorEvent(CursorEventType type, bool isDown)
{
    SlaveGameControlEvent newEvent;
    newEvent.gameEvent = kGameNone;
    newEvent.cursorEvent = type;
    newEvent.keyboardEvent = kKeyboardNone;
    newEvent.isUp = !isDown;
    newEvent.isJoystick = false;
	newEvent.caught = false;
    newEvent.x = 0;
    newEvent.y = 0;
    newEvent.unicodeKeySym = 0;
    newEvent.mouseId = -1;
    m_eventQueue.push_back(newEvent);
}

// Control settings handling
ios_fc::String SlaveEventManager::getControlName(int controlType, bool alternate)
{
    return "Null";
}

bool SlaveEventManager::changeControl(int controlType, bool alternate, GameControlEvent &event)
{
    return true;
}

void SlaveEventManager::saveControls()
{
}


