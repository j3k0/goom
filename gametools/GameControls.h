#ifndef _GAME_CONT_H
#define _GAME_CONT_H

#include <stdint.h>
#include "ios_fc.h"
#include "vec3.h"

namespace gametools {

namespace event_manager
{

/**
 * Corresponds to an event interpreted during game phase
 */
enum GameEventType {
    kGameNone,
    kPauseGame,
    kPlayer1Left,
    kPlayer1Right,
    kPlayer1TurnLeft,
    kPlayer1TurnRight,
    kPlayer1Down,
    kPlayer2Left,
    kPlayer2Right,
    kPlayer2TurnLeft,
    kPlayer2TurnRight,
    kPlayer2Down,
    kGameLastKey
};

/**
 * Corresponds to an event during GUI navigation
 */
enum CursorEventType {
    kCursorNone,
    kUp,
    kDown,
    kLeft,
    kRight,
    kStart,
    kBack,
    kMenu,
    kQuit,
    kGameMouseMoved,
    kGameMouseDown,
    kGameMouseUp,
    kJoystickAxis,
    kJoystickButton,
    kCursorLastKey
};
    
#define kJoystickAxisMax 512

/**
 * Corresponds to an event during keyboard input
 */
enum KeyboardEventType {
    kKeyboardNone,
    kKeyboardDown,
    kKeyboardUp,
    kKeyboardLastKey
};

enum SpecialUnicode {
    kKeyUp       = 0xf700,
    kKeyDown     = 0xf701,
    kKeyLeft     = 0xf702,
    kKeyRight    = 0xf703,
    kKeyDelete   = 0xf728,
    kKeyPageUp   = 0xf72c,
    kKeyPageDown = 0xf72d
};

/**
 * Representation of any kind of user input event happening
 * in the game. Depending on the scope of the object monitoring
 * the event, it can be either/both:
 *  - a game event
 *  - a cursor event (navigation in the GUI, including mouse handling)
 *  - a keyboard event (for keyboard input)
 * GameControlEvent is independant of the underlying
 * event handling backend.
 */
class GameControlEvent {
protected:
	GameControlEvent() : caught(false) {}
public:
    GameEventType     gameEvent;
    CursorEventType   cursorEvent;
    KeyboardEventType keyboardEvent;
    bool isUp;
    bool isJoystick;
	bool caught;
    int x, y;
    uint16_t unicodeKeySym;
    uint16_t keySym;
    unsigned int mouseId;  // FOR MULTITOUCH DEVICES.
    unsigned int buttonId; // FOR Joystick Button Events
	void setCaught() { caught = true; }
	bool isMouse() const { return (cursorEvent == kGameMouseMoved) || (cursorEvent == kGameMouseDown) || (cursorEvent == kGameMouseUp); }
    virtual GameControlEvent *clone() = 0;
};

/**
 * Abstract interface to an event handling backend
 */
class EventManager
{
public:
    virtual ~EventManager() {}
    virtual GameControlEvent *createGameControlEvent() const = 0;
    virtual bool pollEvent(GameControlEvent &controlEvent) = 0;
    virtual void pushMouseEvent(unsigned int mouseId, int x, int y, CursorEventType type) = 0;
    // Control settings handling
    virtual ios_fc::String getControlName(int controlType, bool alternate) = 0;
    virtual bool   changeControl(int controlType, bool alternate, GameControlEvent &event) = 0;
    virtual void   saveControls() = 0;
	virtual Vec3 accelerationInXYZ() const { return Vec3(0,0,0); } // Optional support for accelerometer
};

// TODO: move all that crap elsewhere

enum {
    kPlayer1LeftControl             = 0,
    kPlayer1RightControl            = 1,
    kPlayer1DownControl             = 2,
    kPlayer1ClockwiseControl        = 3,
    kPlayer1CounterclockwiseControl = 4,
    kPlayer2LeftControl             = 5,
    kPlayer2RightControl            = 6,
    kPlayer2DownControl             = 7,
    kPlayer2ClockwiseControl        = 8,
    kPlayer2CounterclockwiseControl = 9
};

void getKeyName(int control, bool alternate, char *keyName);


} // namespace
}

#endif

