#ifndef GTUI_H
#define GTUI_H

#include "drawcontext.h"
#include "GameControls.h"
#include <string>
#include <memory>
#include <map>

namespace gametools {
    namespace ui {
        class WidgetContainer;

        class Widget : public BaseSurface {
            IosRect m_rect;
            Vec2f  m_size;
        public:
            Widget() : m_rect(0,0,0,0) {}
            void           setRect(const IosRect &rect) { m_rect = rect; }
            const IosRect &getRect() const { return m_rect; }
            void size(const Vec2f &s) { m_size = s; }
            // BaseSurface
            virtual Vec2f size() const { return m_size; }
            // To be overloaded
            virtual void eventOccured(WidgetContainer *parent, event_manager::GameControlEvent *event) {}
        };

        class IconButton : public Widget {
            private:
                std::auto_ptr<BaseSurface> m_background;
                std::auto_ptr<BaseSurface> m_backgroundActive;
                std::auto_ptr<BaseSurface> m_icon;
                std::auto_ptr<BaseSurface> m_text;
                float                      m_iconScale;
                bool                       m_active;
                int                        m_activePointerId;

            public:
                // All objects passed to IconButton will be destroyed with it.
                IconButton(BaseSurface *background, BaseSurface *backgroundActive, BaseSurface *icon, BaseSurface *text)
                  : m_background(background), m_backgroundActive(backgroundActive), m_icon(icon), m_text(text), m_iconScale(0.9f), m_active(false), m_activePointerId(-1) {}

                IconButton()
                  : m_background(NULL), m_backgroundActive(NULL), m_icon(NULL), m_text(NULL), m_iconScale(0.9f), m_active(false), m_activePointerId(-1) {}

                IconButton *background(BaseSurface *background) { m_background.reset(background); return this; }
                IconButton *backgroundActive(BaseSurface *background) { m_backgroundActive.reset(background); return this; }
                IconButton *icon(BaseSurface *icon) { m_icon.reset(icon); return this; }
                IconButton *text(BaseSurface *text) { m_text.reset(text); return this; }
                IconButton *iconScale(float scale)  { m_iconScale = scale; return this; }

                virtual void draw(DrawTarget *dt, IosRect *rect, float alpha);
                virtual void eventOccured(WidgetContainer *parent, event_manager::GameControlEvent *event);
        };

        class Text : public IconButton {
            public:
                Text(BaseSurface *text) : IconButton(text,NULL,NULL,NULL) {}
        };

        typedef std::map<std::string,Widget*> WidgetMap;
        typedef std::map<std::string,IosRect> IosRectMap;
        class WidgetContainer {
             WidgetMap m_widget;
             IosRectMap m_rect;
        public:
            typedef WidgetMap::iterator iterator;
            typedef WidgetMap::const_iterator const_iterator;

            ~WidgetContainer() {
                clear();
            }
            void clear();
            void addWidget(const std::string &key, Widget *widget) {
                iterator it = m_widget.find(key);
                if (it != m_widget.end())
                    delete it->second;
                m_widget[key] = widget;
            }
            void eventOccured(event_manager::GameControlEvent *event);
            void draw(DrawTarget *dt);

            Widget *getWidget(const std::string &key);
            const Widget *getWidget(const std::string &key) const;

            IosRect *getWidgetRect(const std::string &key);
            const IosRect *getWidgetRect(const std::string &key) const;

            void setWidgetRect(const std::string &key, const IosRect &rect);
            
            const_iterator begin() const { return m_widget.begin(); }
            const_iterator end() const   { return m_widget.end(); }
            iterator begin() { return m_widget.begin(); }
            iterator end() { return m_widget.end(); }

            virtual void action(gametools::ui::Widget *sender) {} // Called by widgets when actionned...
        };

        // Example use.
        // new IconButton(gtAnisoSurfaces.get("xyz"), gtCommander->getSurface(...), new TextSurfaceRef(font, "xxx"));
    }
}
#endif
