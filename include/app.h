#pragma once
#include "simulation.h"
#include "renderer.h"
#include "input_handler.h"
#include "ui.h"

class App {
public:
    bool  debugMode         = false;
    bool  quietMode         = false;
    float inactivityTimeout = 300.0f;
    int   targetFPS         = 60;
    int   screenW           = SCREEN_W;
    int   screenH           = SCREEN_H;
    void run();

private:
    Simulation   sim;
    Renderer     renderer;
    InputHandler input;
    UI           ui;
};