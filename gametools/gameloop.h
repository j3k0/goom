#ifndef _GAMELOOP_H
#define _GAMELOOP_H

#include "ios_fc.h"
#include "drawcontext.h"
#include "audiomanager.h"
#include "GameControls.h"

namespace gametools {

class GameLoop;

class DrawableComponent
{
  public:
    DrawableComponent();
    virtual ~DrawableComponent();
    virtual bool drawRequested() const;
    // Immediately draws the DrawableComponent
	void doDraw(DrawTarget *dt);
    // Reordering of drawable elements
    bool moveToFront();
    bool moveToBack(DrawableComponent *gc);

    // Notifications
    virtual void onDrawableVisibleChanged(bool visible) {}
  protected:
    GameLoop *parentLoop;
	virtual void requestDraw();
    virtual void draw(DrawTarget *dt) {}
    friend class GameLoop;
  private:
    bool _drawRequested;
};

class IdleComponent
{
  public:
    IdleComponent();
    virtual ~IdleComponent();

    void callIdle(double currentTime) { if (!paused) idle(currentTime); }

    virtual void idle(double currentTime)         {}

    /// return true if you want the GameLoop to skip some frames.
    virtual bool isLate(double currentTime) const { return false; }

    /// perform some computation if you're interested in events.
    virtual void onEvent(event_manager::GameControlEvent *event) {}

    virtual void setPause(bool paused);
    bool getPause() const;

  protected:
    GameLoop *parentLoop;
    bool paused;
    friend class GameLoop;
};


class CycledComponent : public IdleComponent
{
  public:
    CycledComponent(double cycleTime);

    /// called 1 time every cycleTime seconds.
    virtual void cycle()             {}

    void   setCycleTime(double time);
    double getCycleTime() const;

    int    getCycleNumber() const;

    void idle(double currentTime);
    bool isLate(double currentTime) const;

    virtual void setPause(bool paused);
    void reset();

  private:
    double cycleTime;

    double cycleNumber;
    double firstCycleTime;
};
	
class Ticker : public CycledComponent {
	int  m_tickCount;
public:
	Ticker(double speed) : gametools::CycledComponent(speed), m_tickCount(0) {}
	virtual void cycle() { m_tickCount++; }
	int getTickCount() const { return m_tickCount; }
};
	
class ScheduledAction {
public:
    virtual ~ScheduledAction() {}
    virtual void action(const char *name, void *data) {}
};
    
struct ScheduledActionInstance {
    ios_fc::SharedPtr<ScheduledAction> m_action;
    float        m_timeLeft;
    const char * m_name;
    void       * m_data;
};
    
class Scheduler : public IdleComponent {
    double m_lastTime;
    ios_fc::SelfVector<ScheduledActionInstance> m_actions;
public:
    Scheduler() : m_lastTime(0.0f) {}
    void idle(double currentTime);
    void performAfter(float time, ios_fc::SharedPtr<ScheduledAction> action, const char *name = NULL, void *data = NULL) {
        ScheduledActionInstance *a = new ScheduledActionInstance();
        a->m_timeLeft = time;
        a->m_action = action;
        a->m_name = name;
        a->m_data = data;
        m_actions.add(a);
    }
    double time() const { return m_lastTime; }
};

class GarbageCollectableItem {
public:
    virtual ~GarbageCollectableItem() {}
};

/**
 * The GameLoop class manages the main loop of the game
 * and schedules the drawin, timing and events of the game
 */
class GameLoop
{
  public:
    GameLoop();

    void setDrawContext(DrawContext *dc) { m_dc = dc; }
    DrawContext *getDrawContext() const { return m_dc; }
    void setEventManager(event_manager::EventManager *em) { m_em = em; }
    event_manager::EventManager *getEventManager() const { return m_em; }
    void setAudioManager(audio_manager::AudioManager *am) { m_am = am; }
    audio_manager::AudioManager *getAudioManager() const { return m_am; }

    void addDrawable(DrawableComponent *gc);
    void addIdle(IdleComponent *gc);
    void removeDrawable(DrawableComponent *gc);
    void removeIdle(IdleComponent *gc);
    void garbageCollect(GarbageCollectableItem *item);
    void garbageCollectNow(); // run garbage collector.
    void run();     // Run the loop

    // Reordering of drawable elements
    bool moveToFront(DrawableComponent *gc);
    bool moveToBack(DrawableComponent *gc);

    void idle(double currentTime);
    void draw(bool flip = true);

    bool drawRequested() const;
    bool isLate(double currentTime) const;

    static inline double getCurrentTime() {
#if VIDEOSHOT_MODE_30FPS
        return videoshotCurrentTime;
#else
		static double time = 0;
		static double lastTime = 0;
		double t  = ios_fc::getUnixTime();
		if (lastTime == 0) lastTime = t;
		double dt = t - lastTime;
		lastTime = t;
		if (dt > 0.5) dt = 0.5;
		time += dt;
		return time;
#endif
    }

  private:
    DrawContext *m_dc;
    event_manager::EventManager *m_em;
    audio_manager::AudioManager *m_am;

    double timeDrift;
    double lastDrawTime, deltaDrawTimeMax;
#if VIDEOSHOT_MODE_30FPS
    static double videoshotCurrentTime;
#endif
    
	ios_fc::Vector<DrawableComponent> drawables;
    ios_fc::Vector<IdleComponent>     idles;
    ios_fc::Vector<GarbageCollectableItem> garbageCollector;
    bool finished;
};

double benchmarkStepDurationAvg();   // Total time between two calls to Idle
double benchmarkIdleDurationAvg();   // Time spent inside Idle
double benchmarkDrawDurationAvg();   // Time spent inside Draw
void   benchmarkIgnoreSteps(int i); // Ignore a few steps
bool   benchmarkIsIgnoringSteps();

}
#endif // _GAMELOOP_H

