#pragma once
#include "raylib.h"
#include "simulation.h"
#include "localization.h"
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

    bool  trackingCOM        = false;
    float trackBlend         = 0.0f;
    Rectangle btnTrackCOM    = {};

    bool  showIntroDialog       = true;
    int   introPage             = 0;
    bool  resetCameraRequested  = false;

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
    Rectangle btnInfo         = {};
    Rectangle btnResetCamera  = {};
    static constexpr int SCENARIO_COUNT = 6;
    Rectangle scenarioBtns[SCENARIO_COUNT] = {};
    Texture2D scenarioImages[SCENARIO_COUNT] = {};
    bool      scenarioImagesLoaded[SCENARIO_COUNT] = {};

    static constexpr int INTRO_PAGE_COUNT  = 5;
    static constexpr int INTRO_IMAGE_COUNT = 6;
    Texture2D introImages[INTRO_IMAGE_COUNT]     = {};
    bool      introImagesLoaded[INTRO_IMAGE_COUNT] = {};

    Rectangle introTooltipRect = {};
    Rectangle introBtnClose    = {};
    Rectangle introBtnNext     = {};
    Rectangle introBtnBack     = {};
    bool  introConsumedInput   = false;

    void layoutIntroPage();

    std::vector<HRPoint> hrData;

    void loadHRData(const char *path);
    void loadFlags();
    void loadIntroImages();
    void layout();
    void updateLanguageSelector();
    void updateIntroDialog();

    void loadScenarioImages();
    void drawScenarios() const;
    void loadScenario(int idx, Simulation &sim);

    void drawHRDiagram(float mass) const;
    void drawTimeControls(const Simulation &sim) const;
    void drawZoomControls() const;
    void drawLanguageSelector() const;
    void drawPanelBackgrounds() const;
    void drawIntroDialog();
    void drawTextWrapped(const char *text, Rectangle bounds,
                         float fontSize, Color color) const;
};