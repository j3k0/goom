#ifndef GAMEUI_H
#define GAMEUI_H

#include <memory>
#include <vector>
#include "GameControls.h"
#include "ios_fc.h"
#include "vec3.h"
#include "gameloop.h"
#include "CommanderResources.h"
#include "GTAnisoSurface.h"

#define SLOPPY_FOCUS 1

namespace gametools {

  class Widget;
  class WidgetContainer;
  class Box;
  class VBox;
  class HBox;
  class ZBox;
  class SliderContainer;
  class Text;
  class GameUIDefaults;
  class Screen;
  class Button;
  class HScrollList;
  class VScrollList;
  class Separator;
  class Action;
  class ScreenStack;

  enum GameUIEnum {
    USE_MAX_SIZE = 0,
    USE_MIN_SIZE,
    USE_MAX_SIZE_NO_MARGIN,
    ON_ACTION, // Renamed ON_START to make sure I forgot nothing.
    ON_MOUSE_UP, // Mouse UP and DOWN are now distinct events (for list scroll and stuff like that)
    ON_MOUSE_DOWN,
    GAMEUIENUM_LAST
  };

bool isDirectionEvent(event_manager::GameControlEvent *event);

  class Action {
    public:
      virtual void action() {}
      virtual void action(Widget *sender, int actionType, event_manager::GameControlEvent *event) { action(); }
      virtual ~Action() {}
  };


  class GameUIDefaults {
    public:
      static GameUIEnum   CONTAINER_POLICY;
      static float        SPACING;
      static IosFontRef   FONT;
      static IosFontRef   FONT_INACTIVE;
      static IosFontRef   FONT_TEXT;
      static IosFontRef   FONT_SMALL_ACTIVE;
      static IosFontRef   FONT_SMALL_INFO;
      static IosFontRef   FONT_FUNNY;
      static GameLoop    *GAME_LOOP;
      static ScreenStack *SCREEN_STACK;
      static audio_manager::Sound *SLIDE_SOUND;
  };
#define gtScreenStack (*gametools::GameUIDefaults::SCREEN_STACK)
#define gtEventManager (*gametools::GameUIDefaults::GAME_LOOP->getEventManager())
#define gtAudioManager (*gametools::GameUIDefaults::GAME_LOOP->getAudioManager())
    
  class Widget {

      friend class WidgetContainer;
      friend class SliderContainer;
      friend class Box;
      friend class HScrollList;

    public:
      Widget(WidgetContainer *parent = NULL);
      virtual ~Widget();
      void dead() { m_isDead = true; }
      bool isDead() const { return m_isDead; }

      virtual bool drawRequested() const { return _drawRequested; }
      virtual void requestDraw(bool fromParent = false);
      virtual void doDraw(DrawTarget *dt);

      virtual IdleComponent *getIdleComponent() { return NULL; }
      virtual void addToGameLoop(GameLoop *loop);
      virtual void removeFromGameLoopActive();

	  bool isHover() const { return m_hover; }
	  virtual void isHover(bool o) { m_hover = o; }
	  
      virtual void hide();
      virtual void show();
      bool isVisible() const { return !hidden; }

      virtual Vec3 getPreferedSize() const               { return preferedSize; }
      virtual void setPreferedSize(const Vec3 &v3)       { preferedSize = v3;   }

      Vec3 getPosition() const           { return position; }
      Vec3 getSize() const               { return size; }
      IosRect getRect() const { 
        IosRect r;
        r.x = getPosition().x;
        r.y = getPosition().y;
        r.w = getSize().x;
        r.h = getSize().y;
        return r;
      }
      
      bool isMostlyInside(int x, int y) const; // returns true if x,y is inside the widget bbox.

      // default behaviour is to lose the focus
      virtual void eventOccured(event_manager::GameControlEvent *event);

      virtual bool isFocusable() const { return focusable; }

      virtual void giveFocus()       { focus = true;  }
      virtual void lostFocus()       { focus = false; }
      bool haveFocus() const { return focus;  }
      
      virtual void animateShow(float duration) {}
      virtual void animateHide(float duration) {}

      void setAction(GameUIEnum type, Action *action) { actions[type] = action; }
      Action *getAction(GameUIEnum type)              { return actions[type];   }

      bool receiveUpEvents() const { return receiveUp; }
      void setReceiveUpEvents(bool receiveUp) { this->receiveUp = receiveUp; }
      virtual void checkFocus() {}
      
      void forceReceiveAllEvents(bool receiveAll) { this->receiveAll = receiveAll; }
      bool receiveAllEvents() const { return receiveAll; }

      virtual Screen *getParentScreen() const;

      // Notifications
      virtual void onWidgetVisibleChanged(bool visible) { hidden = !visible; }
      virtual void onWidgetAdded(WidgetContainer *parent) {}
      virtual void onWidgetRemoved(WidgetContainer *parent) {}
    protected:
      // To be implemented on each widgets
    virtual void draw(DrawTarget *dt) {     /*SDL_Rect r;
      r.x = getPosition().x;
      r.y = getPosition().y;
      r.h = getSize().y;
      r.w = getSize().x;
      SDL_FillRect(screen,&r,0x2468AC22);*/
    };

    virtual void setBackupPosition(const Vec3 &v3) { backupPosition = v3; }
    virtual const Vec3 &getBackupPosition() const         { return backupPosition; }

      virtual void setPosition(const Vec3 &v3)   { position = v3; }
      virtual void setSize(const Vec3 &v3)       { size     = v3; }
      virtual void setParent(WidgetContainer *p) { parent   =  p; }
      virtual void setFocusable(bool foc);

      virtual void suspendLayout() { }
      virtual void resumeLayout() { }
      virtual void resize(int width, int height) { }

      WidgetContainer *parent;
      
    private:
      Vec3    preferedSize;
      Vec3    size;
    Vec3    backupPosition;
    Vec3    position;
    protected:
      bool    m_isDead;
      bool    hidden;
    private:
      bool    focus;
      bool    focusable;
      bool _drawRequested;
      Action *actions[GAMEUIENUM_LAST];
      bool receiveUp;
      bool receiveAll;
	  bool m_hover;
  };


  class WidgetContainer : public Widget {
    public:
      WidgetContainer(GameLoop *loop = NULL);
      virtual ~WidgetContainer();
      virtual void add (Widget *child);
      virtual void remove (Widget *child);
      GameLoop *getGameLoop();
      virtual void arrangeWidgets() {}

      void setSize(const Vec3 &v3);
      void setPosition(const Vec3 &v3);

      void draw(DrawTarget *dt);
      void requestDraw(bool fromParent = false);
      virtual void widgetMustRedraw(Widget *wid) { requestDraw(); }

      virtual void addToGameLoop(GameLoop *loop);
      virtual void removeFromGameLoopActive();

      bool hasWidget(Widget *wid);
      /** Returns the number of focusable child in the child tree
       */
      int getNumberOfFocusableChilds();

      virtual void resize(int width, int height);
      void suspendLayout();
      void resumeLayout();
	  
	  virtual void isHover(bool o);
      virtual void onWidgetVisibleChanged(bool visible);

      Widget *getChild(int i)     const  { if (i >= 0) return childs[i]; else return NULL; }
      void    changeChild(int i, Widget *w);
      int     getNumberOfChilds() const  { return childs.size(); }

      void setWidgetSize(Widget *w, const Vec3 v3) const { w->setSize(v3); }
      void setWidgetPosition(Widget *w, const Vec3 v3) const { w->setPosition(v3); }
  protected:
      // Accessors to private friend methods of the widget class
      void setWidgetBackupPosition(Widget *w, const Vec3 v3) const { w->setBackupPosition(v3); }
      const Vec3 &getWidgetBackupPosition(Widget *w) const { return w->getBackupPosition(); }
      void drawWidget(Widget *w, DrawTarget *dt) { w->draw(dt); }
      void    sortWidgets();
      bool    layoutSuspended;

    private:
      ios_fc::Vector<Widget> childs;
      bool bubbleSortZ_iteration(int itNumber);
      GameLoop *loop;
      bool addedToGameLoop;
  };


  /**
   * The Box class defines the behaviour of a generic box container
   * It is inherited by the different boxes containers: HBox, VBox, ZBox
   */
  class Box : public WidgetContainer {
    public:
      Box(GameLoop *loop = NULL);
      virtual ~Box() {}
      void setPolicy(GameUIEnum policy);
      void setInnerMargin(int margin) { innerMargin = margin; }
      virtual void arrangeWidgets();
      /**
       * Handles and eventually propagates an event on the widget tree of the box container
       * @param event the game event
       */
      virtual void eventOccured(event_manager::GameControlEvent *event);
      virtual void giveFocus();
      virtual void lostFocus();
      virtual void setFocusable(bool foc);
      virtual void add (Widget *child);
      void checkFocus();
      void focus(Widget *widget);
      int getActiveWidget() const { return activeWidget; }
    protected:
      virtual float getSortingAxe(const Vec3 &v3) const = 0;
      virtual float getOtherAxis(const Vec3 &v3) const = 0;
      virtual void setSortingAxe(Vec3 &v3, float value) = 0;
      virtual void setOtherAxis(Vec3 &v3, float value) = 0;
      virtual bool isPrevEvent(event_manager::GameControlEvent *event) const = 0;
      virtual bool isNextEvent(event_manager::GameControlEvent *event) const = 0;
      virtual bool isOtherDirection(event_manager::GameControlEvent *event) const = 0;

      GameUIEnum policy;
      int     innerMargin;
      int        activeWidget;

      virtual void setActiveWidget(int i);
      virtual bool giveFocusToActiveWidget();

    private:
      void handleMouseFocus(event_manager::GameControlEvent *event);
      void handleKeyboardFocus(event_manager::GameControlEvent *event);
      //Events
      Action *onRollDownAction, *onRollUpAction;
  };


  class VBox : public Box {
    public:
      VBox(GameLoop *loop = NULL) : Box(loop) {}
      virtual ~VBox() {}
    protected:
      float getSortingAxe(const Vec3 &v3) const        { return v3.y;  }
      void  setSortingAxe(Vec3 &v3, float value)       { v3.y = value; }
      virtual float getOtherAxis(const Vec3 &v3) const { return v3.x; }
      virtual void setOtherAxis(Vec3 &v3, float value) { v3.x = value; }
      bool isPrevEvent(event_manager::GameControlEvent *event) const;
      bool isNextEvent(event_manager::GameControlEvent *event) const;
      bool isOtherDirection(event_manager::GameControlEvent *event) const;
  };


  class HBox : public Box {
    public:
      HBox(GameLoop *loop = NULL) : Box(loop) {}
      virtual ~HBox() {}
    protected:
      float getSortingAxe(const Vec3 &v3) const        { return v3.x;  }
      void  setSortingAxe(Vec3 &v3, float value)       { v3.x = value; }
      virtual float getOtherAxis(const Vec3 &v3) const { return v3.y; }
      virtual void setOtherAxis(Vec3 &v3, float value) { v3.y = value; }
      bool isPrevEvent(event_manager::GameControlEvent *event) const;
      bool isNextEvent(event_manager::GameControlEvent *event) const;
      bool isOtherDirection(event_manager::GameControlEvent *event) const;
  };


  class ZBox : public Box {
    public:
      ZBox(GameLoop *loop = NULL) : Box(loop) {}
      virtual ~ZBox() {}
	  // void widgetMustRedraw(Widget *wid);

      virtual void giveFocus() { Box::giveFocus(); }
      virtual void lostFocus() { Box::lostFocus(); }
      virtual void eventOccured(event_manager::GameControlEvent *event);
    protected:
      float getSortingAxe(const Vec3 &v3) const        { return v3.z;  }
      void  setSortingAxe(Vec3 &v3, float value)       { v3.z = value; }
      virtual float getOtherAxis(const Vec3 &v3) const { return v3.x; }
      virtual void setOtherAxis(Vec3 &v3, float value) { v3.x = value; }
      bool isPrevEvent(event_manager::GameControlEvent *event) const;
      bool isNextEvent(event_manager::GameControlEvent *event) const;
      bool isOtherDirection(event_manager::GameControlEvent *event) const;
  };
  
  class AbsoluteContainer : public ZBox {
  public:
    AbsoluteContainer(GameLoop *loop = NULL) : ZBox(loop) {}
    virtual void arrangeWidgets();

      void setWidgetPosition(Widget *w, Vec3 position) {
          ZBox::setWidgetPosition(w, position);
          ZBox::setWidgetBackupPosition(w, position);
      }
      void setWidgetSize(Widget *w, Vec3 size) { ZBox::setWidgetSize(w, size); }
  };

  class SliderContainer;

  /**
   * Represents a slider notification listener
   */
  class SliderContainerListener {
  public:
      /**
       * Notify that the slider is outside of the screen, before sliding back inside
       */
      virtual void onSlideOutside(SliderContainer &slider) {}
      /**
       * Notify that the slider is inside the screen, at the end of its sliding movement
       */
      virtual void onSlideInside(SliderContainer &slider) {}

      virtual ~SliderContainerListener() {}
  };

  /**
   * Represents a slider container, ie a container which slides from one side of the screen
   * to present a content, and which can slide back to the side of the screen to change its
   * content
   */
  class SliderContainer : public ZBox, IdleComponent {
    public:
      /**
       * Constants defining from which side of the screen
       * the slider will slide in and out
       */
      enum SlideFromSide {
          // Perform slide-out then slide-in
          SLIDE_FROM_TOP,
          SLIDE_FROM_LEFT,
          SLIDE_FROM_RIGHT,
          SLIDE_FROM_BOTTOM,
          // Slide-out and slide-in in a single move
          SLIDE_FROM_LEFT_ONE_SHOT,
          SLIDE_FROM_RIGHT_ONE_SHOT,
          // Shows the two widgets fullscreen simulteanously (for custom made transitions)
          SLIDE_CUSTOM
      };
      SliderContainer(GameLoop *loop = NULL);
      virtual ~SliderContainer() {}
      /**
       * Slides the current widget out of the screen, then slides back with a new widget.
       * @param content  The widget to be placed inside the slider when the slider slides back
       */
      void transitionToContent(Widget *content);
      /**
       * Returns the widget contained inside the slider
       * @return  the widget contained inside the slider
       */
      Widget * getContentWidget() const { return contentWidget; }
      /**
       * Changes the background image of the slider
       * @param bg    The new background image
       */
      void setBackground(IosSurface *bg) { this->bg = bg; }
      void setBackgroundVisible(bool visible) { backgroundVisible = visible; }
      /**
       * Change the offset on the background image position.
       * The image will be shifted from its center by the given offset.
       */
      void setBackgroundOffset(const Vec3 &offset) { m_backgroundOffset = offset; }
      void setWhipSound(audio_manager::Sound *whip) { m_whipSound = whip; }
      void setWhopSound(audio_manager::Sound *whop) { m_whopSound = whop; }
      /**
       * Adds a new listener to the events of the SliderContainer widget
       * @param listener   the reference of the new listener object
       */
      void addListener(SliderContainerListener &listener);

      /**
       * Sets current position when not sliding or dest position
       * during slideOut noifications, or inoperant while sliding
       */
      void setPosition(const Vec3 &v3);

      /**
       * Sets the side from where the slider will slide in and out
       */
      void setSlideSide(SlideFromSide slideSide);
      
      void setSlideTime(double slidingTime) { this->slidingTime = slidingTime; }
      
      bool isSliding() const { return sliding; }

      // Implements IdleComponent
      virtual void idle(double currentTime);
      virtual IdleComponent *getIdleComponent() { return this; }

    protected:
      // Implements Widget
      virtual void draw(DrawTarget *dt);
      void eventOccured(event_manager::GameControlEvent *event);
      void addContentWidget();
      void endSlideInside(bool inside);

      // Notifications
      /**
       * Notify that the slider is outside of the screen, before sliding back inside
       */
      virtual void onSlideOutside();
      /**
       * Notify that the slider is inside the screen, at the end of its sliding movement
       */
      virtual void onSlideInside();
      
    private:
      Vec3 m_backgroundOffset;
      SlideFromSide m_slideSide;
      double slidingTime;
      Widget *contentWidget;
      Widget *previousWidget;
      double slideStartTime;
      double currentTime;
      IosSurface *bg;
      bool sliding;
      bool slideout;
      Vec3 backupedPosition;
      bool backgroundVisible;
      std::vector<SliderContainerListener *> listeners;
      int m_outsidePosition;
      audio_manager::Sound *m_whipSound, *m_whopSound;
  };

  /**
   * Represents the root container of a screen
   */
  class ScreenRootContainer : public ZBox {
    public:
        ScreenRootContainer(Screen *parentScreen, GameLoop *loop = NULL) : ZBox(loop), m_parentScreen(parentScreen) {
            forceReceiveAllEvents(true);
        }
        virtual Screen *getParentScreen() const { return m_parentScreen; }
    private:
        Screen *m_parentScreen;
  };

  /**
   * Represents a full screen for containing widgets
   */
  class Screen : public GarbageCollectableItem, public DrawableComponent, public IdleComponent {
    public:
      Screen(GameLoop *loop = NULL);
      Screen(float x, float y, float width, float height, GameLoop *loop = NULL);
      virtual ~Screen() {}
      void draw(DrawTarget *dt);
      virtual void drawAnyway(DrawTarget *dt);
      bool drawRequested() const { if (isVisible()) return rootContainer.drawRequested(); return false;}

      /**
       * Propagates an event on the widget tree of the screen
       * @param event the game event
       */
      virtual void onEvent(event_manager::GameControlEvent *event);
      void remove(Widget *child) { rootContainer.remove(child); }
      void add(Widget *child) { rootContainer.add(child); }
      virtual void hide() { hidden = true; onScreenVisibleChanged(isVisible()); }
      virtual void show() { hidden = false; onScreenVisibleChanged(isVisible());}
      virtual void onDrawableVisibleChanged(bool visible);
      bool isVisible() const { return !hidden; }

      virtual void addToGameLoop(GameLoop *loop) {
          rootContainer.addToGameLoop(loop);
          loop->addDrawable(this);
          loop->addIdle(this);
      }
      virtual void removeFromGameLoopActive() {
          rootContainer.removeFromGameLoopActive();
          getGameLoop()->removeDrawable(this);
          getGameLoop()->removeIdle(this);
      }

      GameLoop *getGameLoop() { return rootContainer.getGameLoop(); }
      void giveFocus();
      void focus(Widget *widget);
      ZBox *getRootContainer() { return &rootContainer; }

      void setAutoRelease(bool autoRelease) { autoReleaseFlag = autoRelease; }
      void autoRelease();

      void grabEventsOnWidget(Widget *widget);
      void ungrabEventsOnWidget(Widget *widget);

      // screen callbacks
      virtual void onScreenVisibleChanged(bool visible);
    
      float getWidth() const { return rootContainer.getSize().x; }
      float getHeight() const { return rootContainer.getSize().y; }

	  void resize(float width, float height) {
		  // rootContainer.setPreferedSize(Vec2(width, height));
		  // rootContainer.setSize(Vec2(width, height));
          rootContainer.resize(width, height);
	  }

	protected:
	  virtual void requestDraw() { DrawableComponent::requestDraw(); rootContainer.requestDraw(true); }

    private:
      void initWithDimensions(float x, float y, float width, float height);
      // The root container of the screen
      ScreenRootContainer rootContainer;
      bool hidden;
      bool autoReleaseFlag;
      std::vector<Widget *> m_grabbedWidgets;
  };

  enum TextAlign {
    TEXT_CENTERED,
    TEXT_LEFT_ALIGN,
    TEXT_RIGHT_ALIGN
  };

  enum ImageAlign {
    IMAGE_CENTERED,
    IMAGE_LEFT_ALIGN,
    IMAGE_RIGHT_ALIGN
  };

  class Text : public Widget, public IdleComponent {
    public:
      Text();
      Text(const ios_fc::String &label, IosFontRef font = NULL, bool autosize = true);
      void setTextAlign(TextAlign align) { m_textAlign = align; }
      TextAlign getTextAlign() const { return m_textAlign; }
      void setAutoSize(bool autoSize) { m_autoSize = autoSize; }
      bool getAutoSize() const { return m_autoSize; }
      void setValue(ios_fc::String value);
	  ios_fc::String getValue() const { return label; }
      void setFont(IosFontRef newFont) { font = newFont; }
      void boing(void);
	  void setShadow(int x, int y);

      // Implements IdleComponent
      virtual void idle(double currentTime);
      virtual IdleComponent *getIdleComponent() { return this; }

    protected:
      void draw(DrawTarget *dt);
      IosFontRef font;
      bool startMoving;

    private:
      ios_fc::String label;
      Vec3 offset;
      double startTime;
      bool moving;
      TextAlign m_textAlign;
      bool m_autoSize;
      audio_manager::Sound *m_slideSound;
	  bool m_shadow;
	  int m_shadow_x; int m_shadow_y;
    public:
      bool mdontMove;
  };
  
  enum ImageFlip {
    IMAGE_XFLIP,
    IMAGE_YFLIP,
    IMAGE_NOFLIP
  };

  class Image : public Widget {
  public:
    Image();
    Image(const AnisoSurface &image, ImageAlign align = IMAGE_LEFT_ALIGN, float scale = 1.0f, bool useGlobalScaleFactor = true);
    ~Image();
    // Properties
    AnisoSurface &getImage() { return m_image; }
    void setImage(const AnisoSurface & image, float scale = 1.0f, bool useGlobalScaleFactor = true);
    void setHoverImage(const AnisoSurface & image) { m_hoverImage = image; }
    void setFocusedImage(const AnisoSurface & image) { m_focusedImage = image; }
    void setAlign(ImageAlign align) { m_align = align; }
    virtual void setFocusable(bool focusable) { Widget::setFocusable(focusable); }
    void setInvertedFocus(bool mode);
    void setFlip(ImageFlip flip) { m_flip = flip; }
    // Notifications
    virtual void onWidgetVisibleChanged(bool visible) { Widget::onWidgetVisibleChanged(visible); }
    virtual void onWidgetAdded(WidgetContainer *parent) { Widget::onWidgetAdded(parent); }
    virtual void onWidgetRemoved(WidgetContainer *parent) { Widget::onWidgetRemoved(parent); }
    virtual void eventOccured(event_manager::GameControlEvent *event);
    // Events
    void setOnAction(Action *onAction) { setAction(ON_ACTION, onAction); setFocusable(true); setReceiveUpEvents(true); }
    void setOnMouseDownAction(Action *onMouseDownAction) { setAction(ON_MOUSE_DOWN, onMouseDownAction); setFocusable(true); }
    void setOnMouseUpAction(Action *onMouseUpAction) { setAction(ON_MOUSE_UP, onMouseUpAction); setFocusable(true); setReceiveUpEvents(true); }
    
    void setColor(float r, float g, float b) { m_red = r; m_green = g; m_blue = b; }
  protected:
    virtual void draw(DrawTarget *dt);
    ImageFlip m_flip;
  private:
    AnisoSurface m_image, m_focusedImage, m_hoverImage;
    bool m_invertFocusMode;
    ImageAlign m_align;
    bool m_useGlobalScaleFactor;
    float m_scale;
    float m_red, m_green, m_blue;
  };

  class Button : public Text {
    public:
      Button(const ios_fc::String &label, IosFontRef fontActive = NULL, IosFontRef fontInactive = NULL);
      Button(const ios_fc::String &label, Action *action);

      void eventOccured(event_manager::GameControlEvent *event);

      void lostFocus();
      void giveFocus();
      void setFocusable(bool foc) { Text::setFocusable(foc); }
    private:
      IosFontRef fontActive;
      IosFontRef fontInactive;
      void init(IosFontRef fontActive, IosFontRef fontInactive);
  };


  class EditField : public Text {
    public:
      EditField(const ios_fc::String &defaultText, Action *action = NULL);
      EditField(const ios_fc::String &defaultText, const ios_fc::String &persistentID);
      ~EditField();
      
      void eventOccured(event_manager::GameControlEvent *event);
      bool handleJoystickEdit(event_manager::GameControlEvent *event);
      void setValue(ios_fc::String value, bool persistent = true);

      void lostFocus();
      void giveFocus();

	    void setEditOnFocus(bool editOnFocus) { this->editOnFocus = editOnFocus; }
      void idle(double currentTime);

    private:
      IosFontRef fontActive;
      IosFontRef fontInactive;
      bool editionMode;
      ios_fc::String persistence;
      ios_fc::String previousValue;
      void init(IosFontRef fontActive, IosFontRef fontInactive);
	  bool editOnFocus;

      // Event repeat related attributes
      bool repeat;
      double repeat_date;
      double repeat_speed;
      std::auto_ptr<event_manager::GameControlEvent> repeatEvent;
  };

  class ControlInputWidget : public Text {
    public:
      ControlInputWidget(int control, bool alternate, Action *action = NULL);

      void eventOccured(event_manager::GameControlEvent *event);

      void lostFocus();
      void giveFocus();

    private:
      int control;
      bool alternate;
      IosFontRef fontActive;
      IosFontRef fontInactive;
      bool editionMode;
      ios_fc::String previousValue;
      void init(IosFontRef fontActive, IosFontRef fontInactive);

      void press(event_manager::GameControlEvent *event);
      void cancel(event_manager::GameControlEvent *event);
      void changeTo(event_manager::GameControlEvent *event);
  };

  class ToggleButton : public Button {
  public:
	  ToggleButton(const ios_fc::String &label, const ios_fc::String &offState, const ios_fc::String &onState, bool initialState, Action *action);
    void setToggle(bool toggleValue);
  private:
	  ios_fc::String unmodifiedLabel, onState, offState;
  };


  class Separator : public Widget {
    public:
      Separator(float width=0., float height=0.);
  };

#ifdef DISABLED
  class ListWidget : public HBox
  {
    public:
      ListWidget(int size, IIM_Surface *downArrow, GameLoop *loop = NULL);
      void set(int pos, Button *widget);
      void add(Button *widget);
      void clear();
    protected:
        virtual void draw(SDL_Surface *screen);
    private:
      int size;
      int used;
      Button button;
      Image downButton, upButton;
      VBox scrollerBox;
      VBox listBox;
  };
#endif

	class ImageButton : public Image
	{
	private:
                Vec2 m_offset;
		ios_fc::String m_text;
		IosFontRef m_font;
	public:
		ImageButton(const AnisoSurface &surface, const char *txt, IosFontRef font, float scale = 1.0f)
              : Image(surface, IMAGE_LEFT_ALIGN, scale), m_offset(0,0), m_text(txt), m_font(font) {}
		virtual void draw(DrawTarget *dt) {
			Image::draw(dt);
			float x = getPosition().x + getSize().x * 0.5f + m_offset.x;
			float y = getPosition().y + getSize().y * 0.5f + m_offset.y;
                        if (m_flip == IMAGE_XFLIP) x -= 2* m_offset.x;
                        if (m_flip == IMAGE_YFLIP) y -= 2* m_offset.y;
                        dt->putStringCenteredXY(m_font.get(), x, y, m_text.c_str());
		}
                void setOffset(const Vec2 &offset) { m_offset = offset; }
                void setLabel(const char *txt) { m_text = txt; }
	};
	
  // Manage a stack of screens.
  class ScreenStack
  {
    public:
      ScreenStack(GameLoop *loop = NULL);
      virtual ~ScreenStack() {}

      void push(Screen *screen);
      void pop();
      Screen * top() const {return stack.top();}

    private:
      ios_fc::Stack<Screen*> stack;
      GameLoop *loop;

      void checkLoop();
  };


  class PushScreenAction : public Action
  {
    public:
      PushScreenAction(Screen *screen, ScreenStack *stack = NULL);
      virtual ~PushScreenAction() {}
      void action();

    private:
      ScreenStack *stack;
      Screen      *screen;
  };

  class PopScreenAction : public Action
  {
    public:
      PopScreenAction(ScreenStack *stack = NULL);
      virtual ~PopScreenAction() {}
      void action();

    private:
      ScreenStack *stack;
  };

    class HScrollList : public HBox {
      public:
  	    HScrollList(GameLoop *loop = NULL);
  	  protected:
  		int getNumberOfVisibleChilds(void);
  	  	Widget* getVisibleChild(int i);
  		void updateShownWidgets(void);
  		void arrangeWidgets(void);
  		void eventOccured(event_manager::GameControlEvent *event);
  		int lastvisible;
  		int firstvisible;
  		bool isItemVisible(int id);
  		Text threedotsbefore;
  		Text threedotsafter;
  	};

};

#include "Frame.h"

#endif

