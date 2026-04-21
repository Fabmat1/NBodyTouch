#include "app.h"
#include "assets.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef PLATFORM_ANDROID
#endif

int main(int argc, char *argv[]) {
    bool  debug   = false;
    bool  quiet   = false;
    float timeout = 300.0f;
    int   fps     = 60;
    int   width   = SCREEN_W;
    int   height  = SCREEN_H;

#ifndef PLATFORM_ANDROID
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            debug = true;
        }
        else if ((strcmp(argv[i], "--width") == 0 || strcmp(argv[i], "-w") == 0) && i + 1 < argc) {
            width = atoi(argv[++i]);
        }
        else if ((strcmp(argv[i], "--height") == 0 || strcmp(argv[i], "-h") == 0) && i + 1 < argc) {
            height = atoi(argv[++i]);
        }
        else if ((strcmp(argv[i], "--root") == 0 || strcmp(argv[i], "-r") == 0) && i + 1 < argc) {
            SetAssetRoot(argv[++i]);
        }
        else if ((strcmp(argv[i], "--timeout") == 0 || strcmp(argv[i], "-t") == 0) && i + 1 < argc) {
            timeout = (float)atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            quiet = true;
        }
        else if ((strcmp(argv[i], "--fps") == 0 || strcmp(argv[i], "-f") == 0) && i + 1 < argc) {
            fps = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("  -d, --debug          Show FPS and debug info\n");
            printf("  -w, --width  <int>   Window width  (default %d)\n", SCREEN_W);
            printf("  -h, --height <int>   Window height (default %d)\n", SCREEN_H);
            printf("  -r, --root   <path>  Program root directory for assets\n");
            printf("  -t, --timeout <sec>  Inactivity timeout in seconds (default 300, 0=off)\n");
            printf("  -q, --quiet          Suppress terminal output\n");
            printf("  -f, --fps    <int>   Target FPS (default 60, 0 = uncapped)\n");
            return 0;
        }
    }
#else
    width  = 0;
    height = 0;
#endif

    App app;
    app.debugMode         = debug;
    app.quietMode         = quiet;
    app.targetFPS         = fps;
    app.inactivityTimeout = timeout;
    if (width > 0)  app.screenW = width;
    if (height > 0) app.screenH = height;
    app.run();
    return 0;
}