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
    // Touch-first phone layout (always on for Android, --mobile on desktop).
    // Must be set before init().
    bool  mobile         = false;
    bool  debugMode      = false;
    bool  quitRequested  = false;
    float zoomRequest    = 0.0f;
    float cameraZoom     = 1.0f;

    void init(int screenW, int screenH);
    void update(Simulation &sim, float &outMass);
    void draw(const Simulation &sim, float mass);
    void shutdown();

    bool        isOverUI(Vector2 pos) const;
    float       uiScale() const { return scale(); }
    const char *loc(LKey key) const;
    void drawText(const char *text, float x, float y, float size, Color col) const;
    int  measureText(const char *text, float size) const;

    bool  trackingCOM        = false;
    bool  trackToggleRequested = false;
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

    void drawHRDiagram(float mass) const;
    void drawTimeControls(const Simulation &sim) const;
    void drawZoomControls() const;
    void drawLanguageSelector() const;
    void drawPanelBackgrounds() const;
    void drawIntroDialog();
    void drawTextWrapped(const char *text, Rectangle bounds,
                         float fontSize, Color color) const;

    // ── Mobile layout (ui_mobile.cpp) ─────────────────────────────────
    float dpScale = 1.0f;   // pixels per density-independent pixel

    Rectangle mHelpBtn = {}, mLangBtn = {}, mLangMenu = {};
    Rectangle mLangItems[LANGUAGE_COUNT] = {};
    Rectangle mSimCard = {}, mBtnPlay = {}, mBtnSlower = {}, mSpeedLabel = {},
              mBtnFaster = {}, mBtnClear = {};
    Rectangle mCamCard = {}, mBtnTrack = {}, mBtnResetView = {};
    Rectangle mMassChip = {};
    Rectangle mSheet = {}, mSheetClose = {}, mStatsRect = {};
    static constexpr int MASS_PRESET_COUNT = 6;
    Rectangle mPresetRects[MASS_PRESET_COUNT] = {};

    bool  langMenuOpen = false;
    bool  sheetOpen    = false;
    float sheetAnim    = 0.0f;   // 0 = hidden, 1 = fully open
    bool  hrDragging   = false;

    char  toastText[128] = {};
    float toastTimer     = 0.0f;

    void initMobileMetrics();
    void detectSystemLanguage();
    float wrappedTextHeight(const char *text, float width, float fontSize) const;
    void layoutMobile();
    void layoutIntroMobile();
    void updateMobile(Simulation &sim);
    void drawMobile(const Simulation &sim, float mass);
    void drawIntroMobile();
    bool isOverUIMobile(Vector2 pos) const;
    Rectangle introAnchorMobile() const;
    Rectangle sheetRectAnimated() const;
    Rectangle hrPlotRect() const;
    void showToast(const char *text);
    float drawSolarValue(const char *value, const char *unit, float x, float y,
                         float size, Color col) const;
    float measureSolarValue(const char *value, const char *unit, float size) const;
};