#pragma once
#include "simulation.h"
#include "renderer.h"
#include "input_handler.h"
#include "ui.h"

class App {
public:
    bool debugMode = false;
    int  screenW   = SCREEN_W;
    int  screenH   = SCREEN_H;
    void run();

private:
    Simulation   sim;
    Renderer     renderer;
    InputHandler input;
    UI           ui;
};