//
// Platform support hooks for the SDL desktop binding: logging and
// per-thread setup (the Cocoa binding uses these for autorelease pools).
//

#include "GTLog.h"
#include <stdio.h>

namespace ios_fc {
    void *beginUnixThread() {
        return NULL;
    }
    void endUnixThread(void *data) {
    }
}

namespace gametools {

    void GTLog(const char *txt) {
        fprintf(stderr, "%s\n", txt);
    }

    void GTLogCheckpoint(const char *name) {
        fprintf(stderr, "CHECKPOINT: %s\n", name);
    }

}
