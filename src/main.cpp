#include "app.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int argc, char *argv[]) {
    bool debug = false;
    int width  = SCREEN_W;
    int height = SCREEN_H;

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
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("  -d, --debug          Show FPS and debug info\n");
            printf("  -w, --width  <int>   Window width  (default %d)\n", SCREEN_W);
            printf("  -h, --height <int>   Window height (default %d)\n", SCREEN_H);
            return 0;
        }
    }

    App app;
    app.debugMode = debug;
    app.screenW   = width;
    app.screenH   = height;
    app.run();
    return 0;
}