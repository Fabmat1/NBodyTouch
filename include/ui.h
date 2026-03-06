#pragma once
#include "raylib.h"
#include "simulation.h"
#include "locale.h"
#include <vector>

struct UISlider {
    Rectangle bounds;
    float     minVal;
    float     maxVal;
    float     value;
    float     snapStep  = 0.0f;  
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
    bool  debugMode      = false;
    bool  quitRequested  = false;
    float zoomRequest    = 0.0f;
    float cameraZoom     = 1.0f;

    void init(int screenW, int screenH);
    void update(Simulation &sim, float &outMass);
    void draw(const Simulation &sim, float mass);
    void shutdown();

    bool        isOverUI(Vector2 pos) const;
    const char *loc(LKey key) const;
    void drawText(const char *text, float x, float y, float size, Color col) const;
    int  measureText(const char *text, float size) const;
    
private:
    int sw = 0, sh = 0;
    float scale() const;

    Font uiFont;
    bool fontLoaded = false;

    int       currentLang = 0;
    Texture2D flagTextures[LANGUAGE_COUNT] = {};
    bool      flagsLoaded[LANGUAGE_COUNT]  = {};
    Rectangle flagRects[LANGUAGE_COUNT]    = {};

    UISlider  massSlider;
    UISlider  timeSlider;

    Rectangle rightPanel      = {};
    Rectangle bottomLeftPanel = {};
    Rectangle zoomPanel       = {};
    Rectangle btnQuit         = {};
    Rectangle btnPause        = {};
    Rectangle btnReset        = {};
    Rectangle btnZoomIn       = {};
    Rectangle btnZoomOut      = {};
    Rectangle hrRect          = {};

    std::vector<HRPoint> hrData;

    void loadHRData(const char *path);
    void loadFlags();
    void layout();
    void updateLanguageSelector();

    void drawHRDiagram(float mass) const;
    void drawTimeControls(const Simulation &sim) const;
    void drawZoomControls() const;
    void drawLanguageSelector() const;
    void drawPanelBackgrounds() const;
};