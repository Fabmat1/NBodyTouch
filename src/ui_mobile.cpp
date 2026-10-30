// Touch-first phone layout. Sizes are in density-independent pixels (dp)
// multiplied by dpScale, so controls keep a ~48 dp touch target regardless
// of the phone's resolution.
//
//  ┌──────────────────────────────────────────────────────────────┐
//  │ [?][flag]  hint / star count       toast                     │
//  │                                                              │
//  │                     simulation                               │
//  │                                                              │
//  │ [▶ − 1× + 🗑] [⌖ ↺]                            [● 1.0 M☉ ‹]  │
//  └──────────────────────────────────────────────────────────────┘
//  The mass chip opens a sheet from the right with the mass slider,
//  presets, derived properties and an H-R diagram you can drag on.

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "ui.h"
#include "star.h"
#include "pointer.h"

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
#include <android/configuration.h>
extern "C" struct android_app *GetAndroidApp(void);
#endif

namespace {

const Color PANEL_BG      = {12, 14, 22, 215};
const Color PANEL_BORDER  = {50, 55, 75, 200};
const Color BTN_BG        = {35, 38, 55, 240};
const Color BTN_DOWN      = {60, 65, 100, 255};
const Color BTN_BORDER    = {90, 95, 130, 200};
const Color TEXT_MAIN     = {225, 228, 240, 255};
const Color TEXT_DIM      = {140, 145, 170, 255};
const Color ACCENT_BG     = {40, 60, 120, 240};
const Color ACTIVE_BG     = {40, 90, 60, 255};
const Color ACTIVE_BORDER = {80, 200, 120, 220};
const Color DANGER_BG     = {55, 30, 30, 240};
const Color DANGER_BORDER = {130, 80, 80, 200};

const float SPEED_STEPS[]  = {0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 5.0f};
const int   SPEED_STEP_COUNT = sizeof(SPEED_STEPS) / sizeof(SPEED_STEPS[0]);
const float MASS_PRESETS[] = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f};

// Main-sequence relation used by stellarModel(): T = 5778 K * M^0.475
constexpr float MS_TEMP_EXPONENT = 0.475f;

Color withAlpha(Color c, float a) {
    c.a = (unsigned char)std::clamp(c.a * a, 0.0f, 255.0f);
    return c;
}

Rectangle shifted(Rectangle r, float dx) { r.x += dx; return r; }

bool heldIn(Rectangle r) {
    return Pointer::down() && CheckCollisionPointRec(Pointer::position(), r);
}

float roundnessFor(Rectangle r, float radius) {
    float m = fminf(r.width, r.height);
    return m > 0.0f ? std::clamp(2.0f * radius / m, 0.0f, 1.0f) : 0.0f;
}

void drawPanel(Rectangle r, float radius, float d,
               Color bg = PANEL_BG, Color border = PANEL_BORDER) {
    float rn = roundnessFor(r, radius);
    DrawRectangleRounded(r, rn, 12, bg);
    DrawRectangleRoundedLinesEx(r, rn, 12, 1.0f * d, border);
}

void drawButtonBg(Rectangle r, float d, Color bg = BTN_BG,
                  Color border = BTN_BORDER) {
    if (heldIn(r)) bg = BTN_DOWN;
    drawPanel(r, 12.0f * d, d, bg, border);
}

// rlgl culls clockwise triangles, so draw both windings
void drawTri(Vector2 a, Vector2 b, Vector2 c, Color col) {
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, b, col);
}

Vector2 center(Rectangle r) { return {r.x + r.width * 0.5f, r.y + r.height * 0.5f}; }

void iconPlay(Vector2 c, float s, Color col) {
    drawTri({c.x - s * 0.32f, c.y - s * 0.42f},
            {c.x - s * 0.32f, c.y + s * 0.42f},
            {c.x + s * 0.42f, c.y}, col);
}

void iconPause(Vector2 c, float s, Color col) {
    float w = s * 0.22f, h = s * 0.8f;
    DrawRectangleRounded({c.x - s * 0.3f, c.y - h * 0.5f, w, h}, 0.4f, 4, col);
    DrawRectangleRounded({c.x + s * 0.08f, c.y - h * 0.5f, w, h}, 0.4f, 4, col);
}

void iconPlusMinus(Vector2 c, float s, float th, bool plus, Color col) {
    DrawRectangleRec({c.x - s * 0.5f, c.y - th * 0.5f, s, th}, col);
    if (plus) DrawRectangleRec({c.x - th * 0.5f, c.y - s * 0.5f, th, s}, col);
}

void iconTrash(Vector2 c, float s, float th, Color col) {
    float bw = s * 0.62f, bh = s * 0.66f;
    Rectangle body = {c.x - bw * 0.5f, c.y - bh * 0.5f + s * 0.12f, bw, bh};
    DrawRectangleRoundedLinesEx(body, 0.2f, 4, th, col);
    DrawLineEx({c.x - s * 0.45f, body.y - th}, {c.x + s * 0.45f, body.y - th}, th, col);
    DrawLineEx({c.x - s * 0.13f, body.y - th * 2.2f}, {c.x + s * 0.13f, body.y - th * 2.2f}, th, col);
    for (float fx : {-0.14f, 0.14f})
        DrawLineEx({c.x + s * fx, body.y + bh * 0.22f},
                   {c.x + s * fx, body.y + bh * 0.78f}, th * 0.8f, col);
}

void iconCrosshair(Vector2 c, float s, float th, Color col) {
    float r = s * 0.3f;
    DrawRing(c, r - th * 0.5f, r + th * 0.5f, 0.0f, 360.0f, 36, col);
    float a0 = r + th, a1 = s * 0.5f;
    DrawLineEx({c.x - a1, c.y}, {c.x - a0, c.y}, th, col);
    DrawLineEx({c.x + a0, c.y}, {c.x + a1, c.y}, th, col);
    DrawLineEx({c.x, c.y - a1}, {c.x, c.y - a0}, th, col);
    DrawLineEx({c.x, c.y + a0}, {c.x, c.y + a1}, th, col);
    DrawCircleV(c, th * 1.1f, col);
}

void iconReset(Vector2 c, float s, float th, Color col) {
    float r = s * 0.34f;
    DrawRing(c, r - th * 0.5f, r + th * 0.5f, 60.0f, 330.0f, 36, col);
    // Arrow head at the 60° end, pointing back along the circle
    float a = 60.0f * DEG2RAD, aw = th * 1.9f;
    Vector2 base = {c.x + cosf(a) * r, c.y + sinf(a) * r};
    Vector2 rad  = {cosf(a), sinf(a)};
    Vector2 tan  = {sinf(a), -cosf(a)};
    drawTri({base.x + rad.x * aw, base.y + rad.y * aw},
            {base.x - rad.x * aw, base.y - rad.y * aw},
            {base.x + tan.x * aw * 1.4f, base.y + tan.y * aw * 1.4f}, col);
}

void iconClose(Vector2 c, float s, float th, Color col) {
    float h = s * 0.5f;
    DrawLineEx({c.x - h, c.y - h}, {c.x + h, c.y + h}, th, col);
    DrawLineEx({c.x + h, c.y - h}, {c.x - h, c.y + h}, th, col);
}

void iconChevronLeft(Vector2 c, float s, float th, Color col) {
    float h = s * 0.5f;
    DrawLineEx({c.x + h * 0.5f, c.y - h}, {c.x - h * 0.5f, c.y}, th, col);
    DrawLineEx({c.x - h * 0.5f, c.y}, {c.x + h * 0.5f, c.y + h}, th, col);
}

void drawStarPreview(Vector2 c, float mass, float d) {
    float r, l, t;
    stellarModel(mass, 0.0f, r, l, t);
    Color col = temperatureToColor(t);
    float core = (5.0f + 3.5f * log10f(1.0f + mass * 2.0f)) * d;
    DrawCircleGradient((int)c.x, (int)c.y, core * 2.6f, withAlpha(col, 0.45f), withAlpha(col, 0.0f));
    DrawCircleV(c, core, col);
    DrawCircleV(c, core * 0.45f, Color{255, 255, 255, 200});
}

const char *formatSolar(float v, char *buf, size_t n) {
    if      (v < 0.1f)     snprintf(buf, n, "%.3f", v);
    else if (v < 10.0f)    snprintf(buf, n, "%.2f", v);
    else if (v < 1000.0f)  snprintf(buf, n, "%.0f", v);
    else if (v < 10000.0f) snprintf(buf, n, "%.1fk", v / 1000.0f);
    else                   snprintf(buf, n, "%.0fk", v / 1000.0f);
    return buf;
}

float easeOutCubic(float t) { t = 1.0f - t; return 1.0f - t * t * t; }

#ifdef PLATFORM_ANDROID
// Back on the root screen: background the app like other Android apps do,
// instead of finishing it and losing the simulation.
void moveAppToBack() {
    android_app *app = GetAndroidApp();
    if (!app || !app->activity) return;
    JavaVM *vm = app->activity->vm;
    JNIEnv *env = nullptr;
    if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) return;
    jobject activity = app->activity->clazz;
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "moveTaskToBack", "(Z)Z");
    if (mid) env->CallBooleanMethod(activity, mid, JNI_TRUE);
    env->DeleteLocalRef(cls);
    vm->DetachCurrentThread();
}
#endif

} // namespace

// ── Metrics & language ─────────────────────────────────────────

void UI::initMobileMetrics() {
    // Desktop preview (--mobile): pretend the window is a ~411 dp tall phone
    float density = (float)sh / 411.0f;
#ifdef PLATFORM_ANDROID
    android_app *app = GetAndroidApp();
    if (app && app->config) {
        int dpi = AConfiguration_getDensity(app->config);
        if (dpi > 0 && dpi != ACONFIGURATION_DENSITY_NONE &&
            dpi != ACONFIGURATION_DENSITY_ANY)
            density = (float)dpi / 160.0f;
    }
#endif
    // Short screens (or big display-size settings) still need to fit the
    // sheet, which wants roughly 340 dp of height.
    dpScale = fmaxf(1.0f, fminf(density, (float)sh / 340.0f));
}

void UI::detectSystemLanguage() {
#ifdef PLATFORM_ANDROID
    android_app *app = GetAndroidApp();
    if (!app || !app->config) return;
    char lang[3] = {};
    AConfiguration_getLanguage(app->config, lang);
    int fallback = currentLang;
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        if (strncmp(LANGUAGES[i].code, lang, 2) == 0) { currentLang = i; return; }
        if (strcmp(LANGUAGES[i].code, "en") == 0) fallback = i;
    }
    currentLang = fallback;
#else
    // Set by the kiosk hub (de|en|es); unknown or missing keeps the default
    const char *lang = getenv("ASTRO_LANG");
    if (!lang) return;
    for (int i = 0; i < LANGUAGE_COUNT; i++)
        if (strncmp(LANGUAGES[i].code, lang, 2) == 0) { currentLang = i; return; }
#endif
}

float UI::wrappedTextHeight(const char *text, float width, float fontSize) const {
    // Mirrors drawTextWrapped()'s line breaking
    float lineH  = fontSize * 1.4f;
    float spaceW = (float)measureText(" ", fontSize);
    float x = 0.0f;
    int   lines = 1;
    const char *p = text;
    char word[512];
    while (*p) {
        if (*p == '\n') { x = 0.0f; lines++; p++; continue; }
        if (*p == ' ')  { if (x > 0.0f) x += spaceW; p++; continue; }
        int wlen = 0;
        while (p[wlen] && p[wlen] != ' ' && p[wlen] != '\n' && wlen < 510) wlen++;
        memcpy(word, p, wlen);
        word[wlen] = '\0';
        p += wlen;
        float ww = (float)measureText(word, fontSize);
        if (x + ww > width && x > 0.0f) { x = 0.0f; lines++; }
        x += ww;
    }
    return (lines - 1) * lineH + fontSize;
}

void UI::showToast(const char *text) {
    snprintf(toastText, sizeof(toastText), "%s", text);
    toastTimer = 2.2f;
}

float UI::measureSolarValue(const char *value, const char *unit, float size) const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %s", value, unit);
    return (float)measureText(buf, size) + size * 0.5f;
}

// "1.0 M" followed by a hand-drawn ☉ subscript (the UI font has no U+2609)
float UI::drawSolarValue(const char *value, const char *unit, float x, float y,
                         float size, Color col) const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %s", value, unit);
    drawText(buf, x, y, size, col);
    float w  = (float)measureText(buf, size);
    float r  = size * 0.19f;
    Vector2 c = {x + w + r + size * 0.06f, y + size * 0.78f};
    DrawRing(c, r - size * 0.035f, r + size * 0.035f, 0.0f, 360.0f, 24, col);
    DrawCircleV(c, size * 0.05f, col);
    return w + size * 0.5f;
}

// ── Layout ─────────────────────────────────────────────────────

void UI::layoutMobile() {
    const float d    = dpScale;
    const float m    = 12.0f * d;   // screen margin
    const float bm   = 14.0f * d;   // bottom margin, clears the gesture bar
    const float pad  = 8.0f * d;
    const float btn  = 48.0f * d;
    const float cardH = btn + 2.0f * pad;
    static_assert(sizeof(MASS_PRESETS) / sizeof(MASS_PRESETS[0]) == MASS_PRESET_COUNT,
                  "one rectangle per mass preset");

    // Top-left: help + language
    mHelpBtn = {m, m, 44.0f * d, 44.0f * d};
    mLangBtn = {mHelpBtn.x + mHelpBtn.width + pad, m, 60.0f * d, 44.0f * d};
    float itemH = 48.0f * d, menuPad = 6.0f * d, menuW = 210.0f * d;
    mLangMenu = {mLangBtn.x, mLangBtn.y + mLangBtn.height + 6.0f * d,
                 menuW, LANGUAGE_COUNT * itemH + 2.0f * menuPad};
    for (int i = 0; i < LANGUAGE_COUNT; i++)
        mLangItems[i] = {mLangMenu.x + menuPad, mLangMenu.y + menuPad + i * itemH,
                         menuW - 2.0f * menuPad, itemH};

    // Bottom-left: time controls
    float y = (float)sh - bm - cardH;
    float stepW = 44.0f * d, labelW = 68.0f * d;
    mBtnPlay    = {m + pad, y + pad, btn, btn};
    mBtnSlower  = {mBtnPlay.x + btn + pad, y + pad, stepW, btn};
    mSpeedLabel = {mBtnSlower.x + stepW, y + pad, labelW, btn};
    mBtnFaster  = {mSpeedLabel.x + labelW, y + pad, stepW, btn};
    mBtnClear   = {mBtnFaster.x + stepW + pad, y + pad, btn, btn};
    mSimCard    = {m, y, mBtnClear.x + btn + pad - m, cardH};

    // Next to it: camera controls
    mCamCard      = {mSimCard.x + mSimCard.width + 10.0f * d, y,
                     3.0f * pad + 2.0f * btn, cardH};
    mBtnTrack     = {mCamCard.x + pad, y + pad, btn, btn};
    mBtnResetView = {mBtnTrack.x + btn + pad, y + pad, btn, btn};

    // Bottom-right: mass chip that opens the star sheet
    float chipW = 176.0f * d;
    mMassChip = {(float)sw - m - chipW, y, chipW, cardH};

    // Star sheet (right side, full height)
    float sheetW = fminf(340.0f * d, (float)sw * 0.46f);
    float p = 16.0f * d;
    mSheet      = {(float)sw - m - sheetW, m, sheetW, (float)sh - m - bm};
    mSheetClose = {mSheet.x + sheetW - 6.0f * d - 44.0f * d, mSheet.y + 6.0f * d,
                   44.0f * d, 44.0f * d};
    float rowY = mSheet.y + 54.0f * d;                  // value row (30 dp)
    massSlider.bounds = {mSheet.x + p, rowY + 30.0f * d, sheetW - 2.0f * p, 36.0f * d};
    float presetY = massSlider.bounds.y + massSlider.bounds.height + 6.0f * d;
    float gap = 6.0f * d;
    float pw  = (sheetW - 2.0f * p - (MASS_PRESET_COUNT - 1) * gap) / MASS_PRESET_COUNT;
    for (int i = 0; i < MASS_PRESET_COUNT; i++)
        mPresetRects[i] = {mSheet.x + p + i * (pw + gap), presetY, pw, 36.0f * d};
    mStatsRect = {mSheet.x + p, presetY + 36.0f * d + 10.0f * d, sheetW - 2.0f * p, 38.0f * d};
    float hrY = mStatsRect.y + mStatsRect.height + 24.0f * d;   // room for the title
    hrRect = {mSheet.x + 4.0f * d, hrY, sheetW - 10.0f * d,
              mSheet.y + mSheet.height - 6.0f * d - hrY};

    layoutIntroMobile();
}

Rectangle UI::introAnchorMobile() const {
    switch (introPage) {
        case 2: return mSimCard;
        case 3: return mCamCard;
        case 4: return mMassChip;
        default: return {};
    }
}

static const LKey MOBILE_INTRO_KEYS[] = {
    LKey::MobileIntroPage1, LKey::MobileIntroPage2, LKey::MobileIntroPage3,
    LKey::MobileIntroPage4, LKey::MobileIntroPage5,
};

void UI::layoutIntroMobile() {
    const float d = dpScale, m = 12.0f * d, p = 18.0f * d;
    const float navH = 44.0f * d, closeSz = 40.0f * d;
    Rectangle a = introAnchorMobile();

    float w = fminf((a.width > 0 ? 420.0f : 500.0f) * d, (float)sw - 2.0f * m);
    float textW = w - p - closeSz - 10.0f * d;
    float textH = wrappedTextHeight(loc(MOBILE_INTRO_KEYS[introPage]), textW, 15.0f * d);
    float h = p + textH + 18.0f * d + navH + p * 0.8f;

    float x, y;
    if (a.width <= 0) {
        h = fminf(h, (float)sh - 2.0f * m);
        x = ((float)sw - w) * 0.5f;
        y = ((float)sh - h) * 0.5f;
    } else {
        h = fminf(h, a.y - 2.0f * m);
        x = std::clamp(a.x + a.width * 0.5f - w * 0.5f, m, (float)sw - m - w);
        y = a.y - 16.0f * d - h;
    }
    introTooltipRect = {x, y, w, h};
    introBtnClose = {x + w - 6.0f * d - closeSz, y + 6.0f * d, closeSz, closeSz};
    float navW = 100.0f * d;
    introBtnNext = {x + w - p - navW, y + h - p * 0.8f - navH, navW, navH};
    introBtnBack = {introBtnNext.x - navW - 8.0f * d, introBtnNext.y, navW, navH};
}

Rectangle UI::sheetRectAnimated() const {
    float hidden = mSheet.width + 16.0f * dpScale;
    return shifted(mSheet, (1.0f - easeOutCubic(sheetAnim)) * hidden);
}

Rectangle UI::hrPlotRect() const {
    // Same margins as drawHRDiagram()
    float s = scale();
    return {hrRect.x + 15.0f * s, hrRect.y + 6.0f * s,
            hrRect.width - 21.0f * s, hrRect.height - 18.0f * s};
}

// ── Update ─────────────────────────────────────────────────────

void UI::updateMobile(Simulation &sim) {
    float dt = GetFrameTime();
    if (toastTimer > 0.0f) toastTimer = fmaxf(0.0f, toastTimer - dt);

    float target = sheetOpen ? 1.0f : 0.0f;
    float step   = dt / 0.22f;
    sheetAnim = (sheetAnim < target) ? fminf(target, sheetAnim + step)
                                     : fmaxf(target, sheetAnim - step);

    // Android back: close the top-most overlay, else background the app
    if (IsKeyPressed(KEY_BACK)) {
        if      (langMenuOpen)    langMenuOpen = false;
        else if (showIntroDialog) showIntroDialog = false;
        else if (sheetOpen)       sheetOpen = false;
#ifdef PLATFORM_ANDROID
        else moveAppToBack();
#endif
    }

    // An open language menu swallows the next tap
    if (langMenuOpen && Pointer::pressed()) {
        Vector2 pos = Pointer::position();
        for (int i = 0; i < LANGUAGE_COUNT; i++) {
            if (CheckCollisionPointRec(pos, mLangItems[i]) && i != currentLang) {
                currentLang = i;
                layout();
            }
        }
        langMenuOpen       = false;
        introConsumedInput = true;   // keeps the tap from placing a star
        return;
    }

    // Taps the intro dialog already handled are not for the controls below
    bool tapsEnabled = !introConsumedInput;
    Vector2 pos = Pointer::position();
    auto tapped = [&](Rectangle r) {
        return tapsEnabled && Pointer::pressed() && CheckCollisionPointRec(pos, r);
    };

    float &speed  = timeSlider.value;
    bool   paused = sim.timeScale <= 0.001f;

    if (tapped(mHelpBtn)) {
        showIntroDialog = true;
        introPage       = 0;
        sheetOpen       = false;
        layoutIntroMobile();
    } else if (tapped(mLangBtn)) {
        langMenuOpen = true;
    } else if (tapped(mBtnPlay)) {
        sim.timeScale = paused ? speed : 0.0f;
    } else if (tapped(mBtnSlower) || tapped(mBtnFaster)) {
        int idx = 0;
        for (int i = 1; i < SPEED_STEP_COUNT; i++)
            if (fabsf(SPEED_STEPS[i] - speed) < fabsf(SPEED_STEPS[idx] - speed)) idx = i;
        idx += CheckCollisionPointRec(pos, mBtnFaster) ? 1 : -1;
        speed = SPEED_STEPS[std::clamp(idx, 0, SPEED_STEP_COUNT - 1)];
        if (!paused) sim.timeScale = speed;
    } else if (tapped(mBtnClear)) {
        sim.clear();
        resetCameraRequested = true;
        showToast(loc(LKey::StarsCleared));
    } else if (tapped(mBtnTrack)) {
        trackToggleRequested = true;
        showToast(loc(trackingCOM ? LKey::TrackOff : LKey::TrackOn));
    } else if (tapped(mBtnResetView)) {
        resetCameraRequested = true;
        showToast(loc(LKey::ViewReset));
    } else if (!sheetOpen && tapped(mMassChip)) {
        sheetOpen = true;
    } else if (sheetOpen && tapped(mSheetClose)) {
        sheetOpen = false;
    } else if (sheetOpen) {
        for (int i = 0; i < MASS_PRESET_COUNT; i++)
            if (tapped(mPresetRects[i])) massSlider.value = MASS_PRESETS[i];
        if (tapped(hrPlotRect())) hrDragging = true;
    }

    if (!Pointer::down() || !sheetOpen) hrDragging = false;

    if (sheetOpen && sheetAnim >= 1.0f) {
        if (!hrDragging) massSlider.update();

        // Dragging across the H-R diagram picks the main-sequence star
        // with that temperature
        if (hrDragging) {
            Rectangle plot = hrPlotRect();
            float xFrac = (pos.x - plot.x - 12.0f) / fmaxf(plot.width - 24.0f, 1.0f);
            xFrac = std::clamp(xFrac, 0.0f, 1.0f);
            float teff = 2000.0f + (1.0f - xFrac) * (40000.0f - 2000.0f);
            float mass = powf(teff / 5778.0f, 1.0f / MS_TEMP_EXPONENT);
            massSlider.value = std::clamp(mass, massSlider.minVal, massSlider.maxVal);
        }
    }
}

bool UI::isOverUIMobile(Vector2 pos) const {
    if (CheckCollisionPointRec(pos, mHelpBtn) ||
        CheckCollisionPointRec(pos, mLangBtn) ||
        CheckCollisionPointRec(pos, mSimCard) ||
        CheckCollisionPointRec(pos, mCamCard))
        return true;
    if (langMenuOpen && CheckCollisionPointRec(pos, mLangMenu)) return true;
    if (sheetAnim > 0.0f || sheetOpen)
        return CheckCollisionPointRec(pos, sheetRectAnimated()) ||
               CheckCollisionPointRec(pos, mMassChip);
    return CheckCollisionPointRec(pos, mMassChip);
}

// ── Draw ───────────────────────────────────────────────────────

void UI::drawMobile(const Simulation &sim, float mass) {
    const float d = dpScale;
    const float th = 2.5f * d;      // icon stroke
    const float iconSz = 22.0f * d;
    bool paused = sim.timeScale <= 0.001f;
    char buf[96];

    // ── Top-left: help, language, hint ──
    drawButtonBg(mHelpBtn, d);
    {
        float fs = 22.0f * d;
        int tw = measureText("?", fs);
        Vector2 c = center(mHelpBtn);
        drawText("?", c.x - tw * 0.5f, c.y - fs * 0.5f, fs, Color{180, 190, 255, 240});
    }
    drawButtonBg(mLangBtn, d, langMenuOpen ? BTN_DOWN : BTN_BG);
    {
        Vector2 c = center(mLangBtn);
        Rectangle f = {c.x - 17.0f * d, c.y - 11.5f * d, 34.0f * d, 23.0f * d};
        if (flagsLoaded[currentLang])
            DrawTexturePro(flagTextures[currentLang],
                {0, 0, (float)flagTextures[currentLang].width,
                       (float)flagTextures[currentLang].height},
                f, {0, 0}, 0.0f, WHITE);
        else {
            float fs = 15.0f * d;
            int tw = measureText(LANGUAGES[currentLang].code, fs);
            drawText(LANGUAGES[currentLang].code, c.x - tw * 0.5f, c.y - fs * 0.5f, fs, TEXT_MAIN);
        }
    }
    {
        float hx = mLangBtn.x + mLangBtn.width + 14.0f * d;
        snprintf(buf, sizeof(buf), loc(LKey::StarsCount), sim.starCount, MAX_STARS);
        if (sim.starCount == 0) {
            drawText(loc(LKey::TouchDragHint), hx, mHelpBtn.y + 2.0f * d,
                     16.0f * d, Color{200, 205, 225, 230});
            drawText(buf, hx, mHelpBtn.y + 24.0f * d, 13.0f * d, TEXT_DIM);
        } else {
            drawText(buf, hx, mHelpBtn.y + 13.0f * d, 14.0f * d, TEXT_DIM);
        }
    }

    // ── Time controls ──
    drawPanel(mSimCard, 16.0f * d, d);
    drawButtonBg(mBtnPlay, d, paused ? ACCENT_BG : BTN_BG,
                 paused ? Color{80, 110, 200, 220} : BTN_BORDER);
    if (paused) iconPlay(center(mBtnPlay), iconSz, WHITE);
    else        iconPause(center(mBtnPlay), iconSz, WHITE);

    drawButtonBg(mBtnSlower, d);
    iconPlusMinus(center(mBtnSlower), 14.0f * d, th, false, TEXT_MAIN);
    drawButtonBg(mBtnFaster, d);
    iconPlusMinus(center(mBtnFaster), 14.0f * d, th, true, TEXT_MAIN);
    {
        Vector2 c = center(mSpeedLabel);
        float capFs = 11.0f * d, valFs = 18.0f * d;
        const char *cap = loc(paused ? LKey::Paused : LKey::Speed);
        int cw = measureText(cap, capFs);
        drawText(cap, c.x - cw * 0.5f, c.y - 19.0f * d, capFs,
                 paused ? Color{150, 180, 255, 255} : TEXT_DIM);
        snprintf(buf, sizeof(buf), "%g×", timeSlider.value);
        int vw = measureText(buf, valFs);
        drawText(buf, c.x - vw * 0.5f, c.y - 3.0f * d, valFs,
                 paused ? TEXT_DIM : TEXT_MAIN);
    }
    drawButtonBg(mBtnClear, d, DANGER_BG, DANGER_BORDER);
    iconTrash(center(mBtnClear), iconSz, 2.0f * d, Color{235, 190, 190, 255});

    // ── Camera controls ──
    drawPanel(mCamCard, 16.0f * d, d);
    drawButtonBg(mBtnTrack, d, trackingCOM ? ACTIVE_BG : BTN_BG,
                 trackingCOM ? ACTIVE_BORDER : BTN_BORDER);
    iconCrosshair(center(mBtnTrack), 26.0f * d, 2.0f * d,
                  trackingCOM ? Color{140, 255, 170, 255} : TEXT_MAIN);
    drawButtonBg(mBtnResetView, d);
    iconReset(center(mBtnResetView), 26.0f * d, 2.4f * d, TEXT_MAIN);

    // ── Mass chip ──
    if (sheetAnim < 0.5f) {
        float a = 1.0f - sheetAnim * 2.0f;
        Color bg = heldIn(mMassChip) ? BTN_DOWN : PANEL_BG;
        drawPanel(mMassChip, 16.0f * d, d, withAlpha(bg, a), withAlpha(PANEL_BORDER, a));
        drawStarPreview({mMassChip.x + 32.0f * d, mMassChip.y + mMassChip.height * 0.5f}, mass, d);
        float tx = mMassChip.x + 62.0f * d;
        drawText(loc(LKey::Mass), tx, mMassChip.y + 11.0f * d, 12.0f * d, withAlpha(TEXT_DIM, a));
        snprintf(buf, sizeof(buf), "%.1f", mass);
        drawSolarValue(buf, "M", tx, mMassChip.y + 29.0f * d, 21.0f * d, withAlpha(TEXT_MAIN, a));
        iconChevronLeft({mMassChip.x + mMassChip.width - 18.0f * d, mMassChip.y + mMassChip.height * 0.5f},
                        14.0f * d, 2.2f * d, withAlpha(TEXT_DIM, a));
    }

    // ── Star sheet ──
    if (sheetAnim > 0.0f) {
        Rectangle r  = sheetRectAnimated();
        float     dx = r.x - mSheet.x;
        float     p  = 16.0f * d;
        drawPanel(r, 18.0f * d, d, Color{12, 14, 22, 235});

        drawText(loc(LKey::StarProperties), r.x + p, r.y + 17.0f * d, 18.0f * d, TEXT_MAIN);
        Rectangle closeR = shifted(mSheetClose, dx);
        if (heldIn(closeR)) DrawCircleV(center(closeR), 20.0f * d, BTN_DOWN);
        iconClose(center(closeR), 14.0f * d, 2.4f * d, TEXT_DIM);

        // Mass value row
        float rowY = mSheet.y + 54.0f * d;
        drawText(loc(LKey::Mass), r.x + p, rowY + 8.0f * d, 14.0f * d, TEXT_DIM);
        snprintf(buf, sizeof(buf), mass < 10.0f ? "%.2f" : "%.1f", mass);
        float valFs = 22.0f * d;
        float vw = measureSolarValue(buf, "M", valFs);
        drawSolarValue(buf, "M", r.x + r.width - p - vw, rowY + 2.0f * d, valFs, TEXT_MAIN);

        // Mass slider (logarithmic, like UISlider)
        {
            Rectangle b = shifted(massSlider.bounds, dx);
            float knobR = 13.0f * d;
            float frac  = (logf(massSlider.value) - logf(massSlider.minVal)) /
                          (logf(massSlider.maxVal) - logf(massSlider.minVal));
            frac = std::clamp(frac, 0.0f, 1.0f);
            float usable = b.width - 2.0f * knobR;
            float ty = b.y + b.height * 0.5f, trackH = 6.0f * d;
            DrawRectangleRounded({b.x + knobR, ty - trackH * 0.5f, usable, trackH}, 1.0f, 8,
                                 Color{50, 50, 65, 220});
            if (frac > 0.001f)
                DrawRectangleRounded({b.x + knobR, ty - trackH * 0.5f, usable * frac, trackH},
                                     1.0f, 8, Color{90, 160, 255, 220});
            float r_, l_, t_;
            stellarModel(mass, 0.0f, r_, l_, t_);
            Vector2 kc = {b.x + knobR + usable * frac, ty};
            if (massSlider.dragging) DrawCircleV(kc, knobR * 1.6f, Color{90, 160, 255, 60});
            DrawCircleV(kc, knobR + 1.5f * d, Color{20, 22, 32, 255});
            DrawCircleV(kc, knobR, Color{200, 215, 255, 255});
            DrawCircleV(kc, knobR * 0.45f, temperatureToColor(t_));
        }

        // Presets
        for (int i = 0; i < MASS_PRESET_COUNT; i++) {
            Rectangle pr = shifted(mPresetRects[i], dx);
            bool sel = fabsf(massSlider.value - MASS_PRESETS[i]) < MASS_PRESETS[i] * 0.02f;
            drawButtonBg(pr, d, sel ? ACCENT_BG : BTN_BG,
                         sel ? Color{90, 140, 230, 230} : BTN_BORDER);
            snprintf(buf, sizeof(buf), "%g", MASS_PRESETS[i]);
            float fs = 14.0f * d;
            int tw = measureText(buf, fs);
            Vector2 c = center(pr);
            drawText(buf, c.x - tw * 0.5f, c.y - fs * 0.5f, fs, sel ? WHITE : TEXT_MAIN);
        }

        // Derived properties
        {
            float rad, lum, temp;
            stellarModel(mass, 0.0f, rad, lum, temp);
            Rectangle sr = shifted(mStatsRect, dx);
            float colW = sr.width / 3.0f;
            float capFs = 11.0f * d, vFs = 15.0f * d;
            const LKey caps[3] = {LKey::Temperature, LKey::Luminosity, LKey::Radius};
            for (int i = 0; i < 3; i++) {
                float cx = sr.x + colW * i;
                drawText(loc(caps[i]), cx, sr.y, capFs, TEXT_DIM);
                char v[32];
                if (i == 0) {
                    snprintf(v, sizeof(v), "%.0f K", temp);
                    drawText(v, cx, sr.y + 16.0f * d, vFs, temperatureToColor(temp));
                } else {
                    formatSolar(i == 1 ? lum : rad, v, sizeof(v));
                    drawSolarValue(v, i == 1 ? "L" : "R", cx, sr.y + 16.0f * d, vFs, TEXT_MAIN);
                }
            }
        }

        // H-R diagram (drawn at the animated position)
        Rectangle savedHR = hrRect;
        hrRect = shifted(hrRect, dx);
        drawHRDiagram(mass);
        hrRect = savedHR;
    }

    // ── Language menu ──
    if (langMenuOpen) {
        drawPanel(mLangMenu, 14.0f * d, d, Color{18, 20, 32, 245}, BTN_BORDER);
        for (int i = 0; i < LANGUAGE_COUNT; i++) {
            Rectangle it = mLangItems[i];
            if (i == currentLang || heldIn(it))
                DrawRectangleRounded(it, roundnessFor(it, 10.0f * d), 8,
                                     i == currentLang ? ACCENT_BG : BTN_DOWN);
            Rectangle f = {it.x + 12.0f * d, it.y + (it.height - 22.0f * d) * 0.5f,
                           33.0f * d, 22.0f * d};
            if (flagsLoaded[i])
                DrawTexturePro(flagTextures[i],
                    {0, 0, (float)flagTextures[i].width, (float)flagTextures[i].height},
                    f, {0, 0}, 0.0f, WHITE);
            float fs = 16.0f * d;
            drawText(LANGUAGES[i].name, f.x + f.width + 14.0f * d,
                     it.y + (it.height - fs) * 0.5f, fs, TEXT_MAIN);
        }
    }

    // ── Toast ──
    if (toastTimer > 0.0f) {
        float a  = fminf(1.0f, toastTimer / 0.4f);
        float fs = 15.0f * d;
        float tw = (float)measureText(toastText, fs);
        // Below the top row so it never covers the hint text
        float ty = mHelpBtn.y + mHelpBtn.height + 14.0f * d;
        Rectangle tr = {((float)sw - tw) * 0.5f - 18.0f * d, ty, tw + 36.0f * d, 38.0f * d};
        drawPanel(tr, 19.0f * d, d, withAlpha(Color{25, 28, 42, 235}, a), withAlpha(BTN_BORDER, a));
        drawText(toastText, tr.x + 18.0f * d, tr.y + (tr.height - fs) * 0.5f, fs,
                 withAlpha(TEXT_MAIN, a));
    }

    if (debugMode) {
        DrawFPS((int)(sw * 0.5f - 40 * d), (int)(60 * d));
        drawText(TextFormat("dp %.2f  %dx%d  touches %d", dpScale, sw, sh, Pointer::touchCount()),
                 sw * 0.5f - 40 * d, 80 * d, 12 * d, GREEN);
    }

    drawIntroMobile();
}

void UI::drawIntroMobile() {
    if (!showIntroDialog) return;
    const float d = dpScale, p = 18.0f * d;
    Rectangle r = introTooltipRect;
    Rectangle a = introAnchorMobile();
    const Color scrim = {0, 0, 0, 130};

    // Dim everything except the control this page explains
    if (a.width > 0) {
        float e = 6.0f * d;
        Rectangle hole = {a.x - e, a.y - e, a.width + 2 * e, a.height + 2 * e};
        DrawRectangleRec({0, 0, (float)sw, hole.y}, scrim);
        DrawRectangleRec({0, hole.y + hole.height, (float)sw, (float)sh - hole.y - hole.height}, scrim);
        DrawRectangleRec({0, hole.y, hole.x, hole.height}, scrim);
        DrawRectangleRec({hole.x + hole.width, hole.y, (float)sw - hole.x - hole.width, hole.height}, scrim);
        DrawRectangleRoundedLinesEx(hole, roundnessFor(hole, 20.0f * d), 12, 3.0f * d,
                                    Color{100, 170, 255, 230});
        DrawLineEx({a.x + a.width * 0.5f, r.y + r.height},
                   {a.x + a.width * 0.5f, hole.y}, 2.0f * d, Color{100, 170, 255, 170});
    } else {
        DrawRectangle(0, 0, sw, sh, scrim);
    }

    drawPanel(r, 18.0f * d, d, Color{18, 20, 32, 245}, Color{70, 85, 140, 230});

    if (heldIn(introBtnClose)) DrawCircleV(center(introBtnClose), 20.0f * d, BTN_DOWN);
    iconClose(center(introBtnClose), 13.0f * d, 2.4f * d, TEXT_DIM);

    float closeSz = introBtnClose.width;
    Rectangle tb = {r.x + p, r.y + p, r.width - p - closeSz - 10.0f * d,
                    introBtnNext.y - r.y - p - 8.0f * d};
    drawTextWrapped(loc(MOBILE_INTRO_KEYS[introPage]), tb, 15.0f * d, Color{215, 220, 238, 255});

    // Page dots
    float dotR = 4.0f * d, dotSpc = 16.0f * d;
    float dotsY = introBtnNext.y + introBtnNext.height * 0.5f;
    for (int i = 0; i < INTRO_PAGE_COUNT; i++)
        DrawCircleV({r.x + p + dotR + i * dotSpc, dotsY}, dotR,
                    i == introPage ? Color{120, 160, 255, 255} : Color{70, 75, 100, 180});

    bool isLast = introPage >= INTRO_PAGE_COUNT - 1;
    float fs = 15.0f * d;
    drawButtonBg(introBtnNext, d, ACCENT_BG, Color{80, 110, 200, 220});
    {
        const char *txt = loc(isLast ? LKey::IntroDone : LKey::IntroNext);
        int tw = measureText(txt, fs);
        Vector2 c = center(introBtnNext);
        drawText(txt, c.x - tw * 0.5f, c.y - fs * 0.5f, fs, WHITE);
    }
    if (introPage > 0) {
        drawButtonBg(introBtnBack, d);
        const char *txt = loc(LKey::IntroBack);
        int tw = measureText(txt, fs);
        Vector2 c = center(introBtnBack);
        drawText(txt, c.x - tw * 0.5f, c.y - fs * 0.5f, fs, Color{200, 200, 220, 255});
    }
}
