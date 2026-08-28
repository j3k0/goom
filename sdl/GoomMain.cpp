//
//  GoomMain.cpp
//  goom-sdl desktop entry point.
//

#include "GameTools.h"
#include "GoomSdlAppDelegate.h"

#include <stdio.h>
#include <string.h>

using namespace gametools;

static void printUsage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("\nGoom - SDL2 desktop visualizer (FoveaGoom).\n");
    printf("\nWindow options (shared with alihood-sdl):\n");
    printf("  --fullscreen         borderless fullscreen at desktop resolution\n");
    printf("  --size WxH           windowed mode size (default 1280x800)\n");
    printf("  --resources <dir>    resources root (default: next to the executable);\n");
    printf("                       game data is read from <dir>/data\n");
    printf("\nGoom options:\n");
    printf("  --file <path>        play <path> (wav) as the audio source\n");
    printf("  --no-mic             disable microphone input (internal heartbeat)\n");
    printf("  --exit-after <secs>  exit automatically after N seconds (testing)\n");
    printf("\nRemote control (HTTP+JSON, off by default):\n");
    printf("  --control[=port]     serve the parameter control UI on localhost\n");
    printf("                       (default port 8090; tries port..port+9)\n");
    printf("  --control-lan        also allow LAN access (tune from a phone)\n");
    printf("\nKeyboard controls:\n");
    printf("  s                    save a screenshot (BMP) to ~/Desktop\n");
    printf("  f                    toggle fullscreen (also F11 / Alt+Enter)\n");
    printf("  p                    toggle pause\n");
    printf("  Esc                  quit\n");
    printf("\n  -h, --help           show this help and exit\n");
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
    }
    GTInit(argc, argv);
    GTSetDelegate(*new GoomSdlAppDelegate(argc, argv));
    GTSetScreenOrientation(GT_LANDSCAPE);
    return GTMain(argc, argv);
}