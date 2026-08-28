#include "gameloop.h"
#include "gameui.h"
#include "GameTools.h"
#include <unistd.h>

#ifdef DEBUG
// #define BENCHMARK_GAMELOOP 1
#endif

#if BENCHMARK_GAMELOOP
extern "C" void glFlush();
#endif

using namespace gametools;
using namespace event_manager;


namespace gametools {

DrawableComponent::DrawableComponent()
  : parentLoop(NULL), _drawRequested(false)
{
}

DrawableComponent::~DrawableComponent()
{
  if (parentLoop != NULL)
    parentLoop->removeDrawable(this);
  parentLoop = NULL;
}

bool DrawableComponent::drawRequested() const
{
  return _drawRequested;
}

void DrawableComponent::requestDraw()
{
  _drawRequested = true;
}

void DrawableComponent::doDraw(DrawTarget *dt)
{
  if (!_drawRequested) requestDraw();
  draw(dt);
  _drawRequested = false;
}

bool DrawableComponent::moveToFront()
{
    return parentLoop->moveToFront(this);
}

bool DrawableComponent::moveToBack(DrawableComponent *gc)
{
    return parentLoop->moveToBack(this);
}

// IDLE COMPONENT

IdleComponent::IdleComponent()
  : parentLoop(NULL), paused(false)
{
}

IdleComponent::~IdleComponent()
{
  if (parentLoop != NULL)
    parentLoop->removeIdle(this);
  parentLoop = NULL;
}

void IdleComponent::setPause(bool paused)
{
  this->paused = paused;
}

bool IdleComponent::getPause() const
{
  return paused;
}

// CYCLE COMPONENT

CycledComponent::CycledComponent(double cycleTime)
  : IdleComponent(), cycleTime(cycleTime)
{
  reset();
}

void   CycledComponent::setCycleTime(double time)
{
  cycleTime = time;
  reset();
}

int CycledComponent::getCycleNumber() const
{
  return (int)cycleNumber;
}

double CycledComponent::getCycleTime() const
{
  return cycleTime;
}

#define time_tolerence 0.25

void   CycledComponent::idle(double currentTime)
{
  if (cycleNumber < -0.5) {
    firstCycleTime = currentTime;
    cycleNumber = 0.0;
  }

  bool requireCycle = (currentTime + time_tolerence > firstCycleTime + cycleTime * cycleNumber);

  if (requireCycle && (!paused)) {
    cycleNumber += 1.0;
    cycle();
  }
}

void CycledComponent::setPause(bool paused)
{
  this->paused = paused;
  if (!paused) reset();
}

void CycledComponent::reset()
{
  firstCycleTime = 0.0;
  cycleNumber    = -1.0;
  paused         = false;
}

bool CycledComponent::isLate(double currentTime) const
{
  if (cycleNumber < 0.0)
    return false;
  double idealTime = firstCycleTime + cycleNumber * cycleTime;
  return (currentTime > idealTime + time_tolerence);
}
    
#pragma mark SCHEDULER
    
void Scheduler::idle(double currentTime) {
    float dt = currentTime - m_lastTime;
    m_lastTime = currentTime;
    if (dt > 0.1f) dt = 0.1f;
    int toRemove = -1;
    for (int i=0; i<m_actions.size(); ++i) {
        ScheduledActionInstance &a = m_actions[i];
        a.m_timeLeft -= dt;
        if (a.m_timeLeft < 0.0f) {
            if (a.m_timeLeft > -999.9f) {
                a.m_action->action(a.m_name, a.m_data);
                a.m_timeLeft = -9999.0f;
            }
            toRemove = i;
        }
    }
    if (toRemove >= 0)
        m_actions.removeAt(toRemove);
}

#pragma mark GAME LOOP

#if VIDEOSHOT_MODE_30FPS
double GameLoop::videoshotCurrentTime = 0.0;
#endif

GameLoop::GameLoop() : timeDrift(0), lastDrawTime(getCurrentTime()), deltaDrawTimeMax(2.) {
  finished = false;
}

void GameLoop::addDrawable(DrawableComponent *gc)
{
  for (int i = 0; i < drawables.size(); ++i)
    if (drawables[i] == gc) return;

#ifdef DEBUG_GAMELOOP
  printf("Drawable %x added to gameloop!\n", gc);
#endif
  drawables.add(gc);
  gc->parentLoop = this;
  gc->onDrawableVisibleChanged(true);
}

void GameLoop::addIdle(IdleComponent *gc)
{
  for (int i = 0; i < idles.size(); ++i)
    if (idles[i] == gc) return;

#ifdef DEBUG_GAMELOOP
  printf("Idle %x added to gameloop!\n", gc);
#endif
  idles.add(gc);
  gc->parentLoop = this;
}

void GameLoop::removeDrawable(DrawableComponent *gc)
{
  for (int i = 0; i < drawables.size(); ++i) {
    if (gc == drawables[i]) {
#ifdef DEBUG_GAMELOOP
      printf("Drawable %x removed from gameloop!\n", gc);
#endif
      drawables[i] = NULL;
      gc->parentLoop = NULL;
      gc->onDrawableVisibleChanged(false);
    }
  }
}

void GameLoop::removeIdle(IdleComponent *gc)
{
  for (int i = 0; i < idles.size(); ++i) {
    if (gc == idles[i]) {
#ifdef DEBUG_GAMELOOP
      printf("Idle %x removed from gameloop!\n", gc);
#endif
      idles[i] = NULL;
      gc->parentLoop = NULL;
      }
  }
}

void GameLoop::garbageCollect(GarbageCollectableItem *item)
{
    garbageCollector.add(item);
}

void GameLoop::run()
{
  draw();
  while (!finished) {
      double currentTime = getCurrentTime();
      idle(currentTime);
      if ((m_dc != NULL) && drawRequested()) {
        currentTime = getCurrentTime();
        // Ensure at least a frame every deltaDrawTimeMax gets drawn
        if ((!isLate(currentTime)) || (currentTime - lastDrawTime > deltaDrawTimeMax)) {
          draw();
          lastDrawTime = currentTime;
        }
      }
  }
}

void GameLoop::garbageCollectNow() {
  while (garbageCollector.size() > 0) {
#ifdef DEBUG_GAMELOOP
    printf("GARBAGE COLLECTABLE REMOVED\n");
#endif
    delete garbageCollector[0];
    garbageCollector.removeAt(0);
  }
}

static double lastStep = 0.0;
static double smoothedStepTime = 0.0;
static double smoothedIdleTime = 0.0;
static double smoothedDrawTime = 0.0;
static int    stepsToIgnore   = 2;

double benchmarkStepDurationAvg() { return smoothedStepTime; }
double benchmarkIdleDurationAvg() { return smoothedIdleTime; }
double benchmarkDrawDurationAvg() { return smoothedDrawTime; }
void   benchmarkIgnoreSteps(int i) { stepsToIgnore += i * 2; }
bool   benchmarkIsIgnoringSteps()  { return stepsToIgnore > 0; }

void GameLoop::idle(double currentTime)
{
    double beforeIdleTime = getCurrentTime();

  int i, idles_size_at_start;

  idles_size_at_start = idles.size();

  //Vector<IdleComponent> idlesCpy = idles.dup();

  // 1- process events
  std::auto_ptr<GameControlEvent> controlEvent(m_em->createGameControlEvent());;
  while (m_em->pollEvent(*controlEvent)) {
    for (i=0; i < idles_size_at_start; ++i) {
      if (idles[i]) {
#ifdef DEBUG_GAMELOOP
	printf("onEvent called on idle %x!\n", idles[i]);
	fflush(stdout);
#endif
        idles[i]->onEvent(controlEvent.get());
      }
    }
    if (controlEvent->cursorEvent == kQuit) { // TODO: Laisser libre l'utilisateur ?
      exit(0);
    }
  }

  // 2- call idles
  for (i = 0; i < idles_size_at_start; ++i) {
    IdleComponent *gc = idles[i];
    if (gc != NULL)
      gc->callIdle(currentTime);
  }

  // 3- check components to remove/kill
  // 3a- active remove
  for (i = 0; i<idles.size();) {
    IdleComponent *gc = idles[i];
    if (gc == NULL) {
#ifdef DEBUG_GAMELOOP
      printf("IDLE %d REMOVED############\n", i);
#endif
      idles.removeAt(i);
    }
    else i++;
  }
  for (i = 0; i<drawables.size();) {
    DrawableComponent *gc = drawables[i];
    if (gc == NULL) {
#ifdef DEBUG_GAMELOOP
    printf("DRAWABLE %d REMOVED############\n", i);
#endif
      drawables.removeAt(i);
    }
    else i++;
  }

  // 3b- garbage collector for passive remove
  garbageCollectNow();
    
  double afterIdleTime = getCurrentTime();
  if (lastStep == 0.0) lastStep = afterIdleTime;
  if (stepsToIgnore <= 0) {
    smoothedIdleTime = smoothedIdleTime * 0.85 + (afterIdleTime - beforeIdleTime) * 0.15;
    double stepTime = afterIdleTime - lastStep;
    smoothedStepTime = smoothedStepTime * 0.85 + stepTime * 0.15;
  }
  else
      stepsToIgnore--;
  lastStep = afterIdleTime;
    
#if VIDEOSHOT_MODE_30FPS
    videoshotCurrentTime += 1.0 / 30.0;
#endif
}

bool GameLoop::drawRequested() const
{
  for (int i = 0; i < drawables.size(); ++i) {
    if ((drawables[i] != NULL) && (drawables[i]->drawRequested()))
      return true;
  }
  return false;
}

bool GameLoop::isLate(double currentTime) const
{
  for (int i = 0; i < idles.size(); ++i) {
    if (idles[i] == NULL) continue;
    if (idles[i]->isLate(currentTime))
      return true;
  }
  return false;
}

void GameLoop::draw(bool flip)
{
    double beforeDrawTime = getCurrentTime();
  DrawContext *dc = getDrawContext();
  if (dc == NULL) return;

  for (int i = 0; i < drawables.size(); ++i) {
	if (drawables[i] != NULL)
        drawables[i]->doDraw(dc);
    else
		GTLog("INVALID DRAWABLE");
  }
#if BENCHMARK_GAMELOOP
    glFlush();
#endif
    if (stepsToIgnore <= 0) {
        double afterDrawTime = getCurrentTime();
        smoothedDrawTime = smoothedDrawTime * 0.85 + (afterDrawTime - beforeDrawTime) * 0.15;
    }
    else
        stepsToIgnore--;
#if BENCHMARK_GAMELOOP
    char txt[256];
    sprintf(txt, "s: %.0fms\ni:%.0fms\nd:%.0fms", smoothedStepTime * 1000.0, smoothedIdleTime * 1000.0, smoothedDrawTime * 1000.0);
    dc->putString(gtCommander->getFont("gfx/numberfont.ttf", 9, Font_WHITE), 6, 50, txt);
#endif
  if (!flip)
      return;
  if (m_dc != NULL) m_dc->flip();
}

bool GameLoop::moveToFront(DrawableComponent *gc)
{
    if (drawables[drawables.size()-1] == gc)
        return false;
    removeDrawable(gc);
    addDrawable(gc);
    return true;
}

bool GameLoop::moveToBack(DrawableComponent *gc)
{
    return false;
}

}
