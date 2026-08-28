#include "GTUI.h"

namespace gametools {
    namespace ui {

        void IconButton::draw(DrawTarget *dt, IosRect *rect, float alpha) {
            IosRect left  = rect->left(0.25f);
            if (m_active && m_backgroundActive.get())
                m_backgroundActive->draw(dt, rect, alpha);
            else if (m_background.get())
                m_background->draw(dt, rect, alpha);
            if (m_icon.get()) {
                // Adjust icon aspect
                left.y += left.h * 0.5f;
                left.h  = left.w * m_icon->size().y / m_icon->size().x;
                left.y -= left.h * 0.5f;
                IosRect leftScaled = left.scaled(m_iconScale);
                m_icon->draw(dt, &leftScaled, alpha);
            }
            if (m_text.get()) {
                // 
                IosRect right = *rect;
                right.x = left.x + left.w * 1.2f;
                right.w = m_text->size().x;
                if (right.x + right.w > rect->x + rect->w)
                    right.w = rect->x + rect->w - right.x;
                m_text->draw(dt, &right, alpha);
            }
        }

        void IconButton::eventOccured(WidgetContainer *parent, event_manager::GameControlEvent *event) {
            if (event->isMouse() && getRect().isInside(event->x, event->y)) {
                switch (event->cursorEvent) {
                    case event_manager::kGameMouseDown:
                        m_active = true;
                        m_activePointerId = event->mouseId;
                        break;
                    case event_manager::kGameMouseUp:
                        if (m_active && m_activePointerId == event->mouseId)
                            parent->action(this);
                        m_active = false;
                        break;
                }
            }
            else if (event->isMouse() && m_active && m_activePointerId == event->mouseId)
                    m_active = false;
        }

        void WidgetContainer::clear() {
            WidgetMap::iterator it = m_widget.begin();
            while (it != m_widget.end()) {
                delete it->second;
                ++it;
            }
            m_widget.clear();
        }

        void WidgetContainer::eventOccured(event_manager::GameControlEvent *event) {
            WidgetMap::iterator it = m_widget.begin();
            while (it != m_widget.end()) {
                it->second->eventOccured(this, event);
                ++it;
            }
        }

        void WidgetContainer::draw(DrawTarget *dt) {
            WidgetMap::iterator it = m_widget.begin();
            while (it != m_widget.end()) {
                it->second->draw(dt, getWidgetRect(it->first), 1.0f);
                ++it;
            }
        }

        Widget *WidgetContainer::getWidget(const std::string &key) {
            iterator it = m_widget.find(key);
            if (it != m_widget.end())
                return (it->second);
            else
                return NULL;
        }

        const Widget *WidgetContainer::getWidget(const std::string &key) const {
            const_iterator it = m_widget.find(key);
            if (it != m_widget.end())
                return (it->second);
            else
                return NULL;
        }

        IosRect *WidgetContainer::getWidgetRect(const std::string &key) {
            IosRectMap::iterator it = m_rect.find(key);
            if (it != m_rect.end())
                return &(it->second);
            else
                return NULL;
        }

        const IosRect *WidgetContainer::getWidgetRect(const std::string &key) const {
            IosRectMap::const_iterator it = m_rect.find(key);
            if (it != m_rect.end())
                return &(it->second);
            else
                return NULL;
        }

        void WidgetContainer::setWidgetRect(const std::string &key, const IosRect &rect) {
            IosRectMap::iterator it = m_rect.find(key);
            if (it != m_rect.end())
                it->second = rect;
            else
                m_rect[key] = rect;
            Widget *w = getWidget(key);
            if (w)
                w->setRect(rect);
        }

    }
}

