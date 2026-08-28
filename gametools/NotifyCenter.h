#ifndef _NOTIFYCENTER_H
#define _NOTIFYCENTER_H

#include "gameui.h"
#include "ios_hash.h"

namespace gametools {
    
    class NotificationResponder {
    public:
        virtual void notificationOccured(ios_fc::String identifier, void * context) {};
        virtual ~NotificationResponder() {};
    };
    
    class NotifyCenter: ios_fc::HashMap {
    public:
        NotifyCenter() {};
        virtual ~NotifyCenter() {};
        void addListener(ios_fc::String identifier, NotificationResponder * listener);
        void removeListener(ios_fc::String identifier, NotificationResponder * listener);
        void notify(ios_fc::String identifier, void * context);
    };
    
    extern NotifyCenter gtNotifier;
}


#endif // _NOTIFYCENTER_H
