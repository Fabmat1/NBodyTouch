#pragma once
#include "raylib.h"
#include "simulation.h"
#include <vector>

struct UISlider {
    Rectangle bounds;
    float     minVal;
    float     maxVal;
    float     value;
    const char *label;
    const char *fmt;
    bool       dragging = false;

    void draw(Font font, float fontSize) const;
    bool update();
};

struct HRPoint {
    float teff;
    float luminosity;
};

class UI {
public:
    bool debugMode    = false;
    bool quitRequested = false;

    void init(int screenW, int screenH);
    void update(Simulation &sim, float &outMass);
    void draw(const Simulation &sim, float mass);
    void shutdown();

    bool isOverUI(Vector2 pos) const;


private:
    int sw = 0, sh = 0;

    Font uiFont;
    bool fontLoaded = false;

    UISlider massSlider;
    UISlider timeSlider;

    Rectangle btnQuit;

    Rectangle btnPause;
    Rectangle btnReset;
    Rectangle hrRect;

    std::vector<HRPoint> hrData;
    void loadHRData(const char *path);

    void drawHRDiagram(float mass) const;
    void drawTimeControls(Simulation &sim);
    void drawPanelBackgrounds() const;
    void drawText(const char *text, float x, float y, float size, Color col) const;
    int  measureText(const char *text, float size) const;
};