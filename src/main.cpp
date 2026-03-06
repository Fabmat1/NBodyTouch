#include "app.h"
#include "assets.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// src/main.cpp — replace the existing main function signature area only

#ifdef PLATFORM_ANDROID
// On Android, raylib provides its own entry point via NativeActivity.
// We just need main() to exist — raylib calls it internally.
#endif

int main(int argc, char *argv[]) {
    bool debug = false;
    int width  = SCREEN_W;
    int height = SCREEN_H;

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
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("  -d, --debug          Show FPS and debug info\n");
            printf("  -w, --width  <int>   Window width  (default %d)\n", SCREEN_W);
            printf("  -h, --height <int>   Window height (default %d)\n", SCREEN_H);
            printf("  -r, --root   <path>  Program root directory for assets\n");
            return 0;
        }
    }
#else
    // On Android, use screen dimensions and no asset root
    // (raylib reads from APK assets/ automatically)
    width  = 0;  // 0 = use screen size
    height = 0;
#endif

    App app;
    app.debugMode = debug;
    if (width > 0)  app.screenW = width;
    if (height > 0) app.screenH = height;
    app.run();
    return 0;
}