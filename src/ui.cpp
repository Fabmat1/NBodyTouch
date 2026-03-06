#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>

#include "ui.h"
#include "star.h"
#include "assets.h"
#include "locale.h"
#include "compat.h"

// ── Helpers ────────────────────────────────────────────────────

float UI::scale() const {
    return fminf((float)sw / 1920.0f, (float)sh / 1080.0f);
}

const char *UI::loc(LKey key) const {
    return LANGUAGES[currentLang].strings[(int)key];
}

void UI::drawText(const char *text, float x, float y, float size, Color col) const {
    if (fontLoaded) {
        DrawTextEx(uiFont, text, {x, y}, size, 1.0f, col);
    } else {
        DrawText(text, (int)x, (int)y, (int)size, col);
    }
}

int UI::measureText(const char *text, float size) const {
    if (fontLoaded) {
        Vector2 v = MeasureTextEx(uiFont, text, size, 1.0f);
        return (int)v.x;
    }
    return MeasureText(text, (int)size);
}

// ── UISlider ───────────────────────────────────────────────────

void UISlider::draw(Font font, float fontSize) const {
    float knobR    = (bounds.height - 2.0f) * 0.5f;
    float usableW  = bounds.width - 2.0f * knobR;
    float trackH   = fmaxf(2.0f, bounds.height * 0.22f);
    float trackY   = bounds.y + bounds.height * 0.5f - trackH * 0.5f;

    float range = maxVal / fmaxf(minVal, 1e-6f);
    float frac;
    if (range > 10.0f && minVal > 0.0f) {
        frac = (logf(value) - logf(minVal)) / (logf(maxVal) - logf(minVal));
    } else {
        frac = (value - minVal) / (maxVal - minVal);
    }
    frac = std::clamp(frac, 0.0f, 1.0f);

    Rectangle trackBg = { bounds.x + knobR, trackY, usableW, trackH };
    DrawRectangleRounded(trackBg, 1.0f, 8, Color{50, 50, 65, 220});

    if (frac > 0.001f) {
        Rectangle trackFill = { trackBg.x, trackY, usableW * frac, trackH };
        DrawRectangleRounded(trackFill, 1.0f, 8, Color{90, 160, 255, 200});
    }

    float knobX = bounds.x + knobR + usableW * frac;
    float knobY = bounds.y + bounds.height * 0.5f;
    DrawCircle((int)knobX, (int)knobY, knobR + 1, Color{30, 30, 40, 255});
    DrawCircle((int)knobX, (int)knobY, knobR, Color{180, 200, 255, 255});

    float labelGap = fontSize * 0.3f;
    float labelY   = bounds.y - fontSize - labelGap;
    if (font.texture.id > 0) {
        DrawTextEx(font, label, {bounds.x, labelY}, fontSize, 1.0f, Color{180,180,200,255});
        char buf[64];
        snprintf(buf, sizeof(buf), fmt, value);
        Vector2 vs = MeasureTextEx(font, buf, fontSize, 1.0f);
        DrawTextEx(font, buf, {bounds.x + bounds.width - vs.x, labelY}, fontSize, 1.0f, WHITE);
    } else {
        DrawText(label, (int)bounds.x, (int)labelY, (int)fontSize, Color{180,180,200,255});
        char buf[64];
        snprintf(buf, sizeof(buf), fmt, value);
        int tw = MeasureText(buf, (int)fontSize);
        DrawText(buf, (int)(bounds.x + bounds.width - tw), (int)labelY, (int)fontSize, WHITE);
    }
}

bool UISlider::update() {
    Vector2 mouse = GetMousePosition();
    bool changed  = false;

    float padX = bounds.height * 0.3f;
    float padY = bounds.height * 0.45f;
    Rectangle hitArea = { bounds.x - padX, bounds.y - padY,
                          bounds.width + 2 * padX, bounds.height + 2 * padY };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mouse, hitArea)) {
        dragging = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        dragging = false;
    }

    float range = maxVal / fmaxf(minVal, 1e-6f);
    bool useLog = (range > 10.0f && minVal > 0.0f);

    auto applyPos = [&](Vector2 p) {
        float knobR   = (bounds.height - 2.0f) * 0.5f;
        float usableW = bounds.width - 2.0f * knobR;
        float frac    = (p.x - bounds.x - knobR) / usableW;
        frac = std::clamp(frac, 0.0f, 1.0f);

        float newVal;
        if (useLog) {
            newVal = expf(logf(minVal) + frac * (logf(maxVal) - logf(minVal)));
        } else {
            newVal = minVal + frac * (maxVal - minVal);
        }

        if (newVal != value) { value = newVal; changed = true; }
    };

    if (dragging) applyPos(mouse);

    for (int t = 0; t < GetTouchPointCount(); t++) {
        Vector2 tp = GetTouchPosition(t);
        if (CheckCollisionPointRec(tp, hitArea)) applyPos(tp);
    }

    return changed;
}

// ── HR Data loading ────────────────────────────────────────────

void UI::loadHRData(const char *path) {
    hrData.clear();
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "HR data file not found: %s", path);
        return;
    }

    char *text = LoadFileText(path);
    if (!text) return;

    char *line = strtok(text, "\n\r");
    while (line) {
        if (line[0] == '#' || line[0] == '\0') {
            line = strtok(nullptr, "\n\r");
            continue;
        }
        float teff = 0, lum = 0;
        if (sscanf(line, "%f,%f", &teff, &lum) == 2) {
            hrData.push_back({teff, lum});
        }
        line = strtok(nullptr, "\n\r");
    }

    UnloadFileText(text);
    TraceLog(LOG_INFO, "Loaded %d HR diagram points from %s", (int)hrData.size(), path);
}

// ── Flag loading ───────────────────────────────────────────────

void UI::loadFlags() {
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        char rel[256];
        snprintf(rel, sizeof(rel), "assets/locale_flags/%s", LANGUAGES[i].flagFile);
        const char *fullPath = AssetPath(rel);

        if (FileExists(fullPath)) {
            flagTextures[i] = LoadTexture(fullPath);
            SetTextureFilter(flagTextures[i], TEXTURE_FILTER_BILINEAR);
            flagsLoaded[i] = (flagTextures[i].id != 0);
            if (flagsLoaded[i])
                TraceLog(LOG_INFO, "Flag loaded: %s", fullPath);
        } else {
            flagsLoaded[i] = false;
            TraceLog(LOG_WARNING, "Flag texture not found: %s", fullPath);
        }
    }
}

// ── Layout ─────────────────────────────────────────────────────

void UI::layout() {
    float s = scale();

    // ── Right panel ───────────────────────────────────
    float panelW = 250.0f * s;
    float panelX = (float)sw - panelW;
    rightPanel = { panelX, 0, panelW, (float)sh };

    // Language flags
    float flagW   = 36.0f * s;
    float flagH   = 24.0f * s;
    float flagGap = 8.0f * s;
    float totalFW = LANGUAGE_COUNT * flagW + (LANGUAGE_COUNT - 1) * flagGap;
    float flagX0  = panelX + (panelW - totalFW) * 0.5f;
    float flagY   = 10.0f * s;
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        flagRects[i] = { flagX0 + i * (flagW + flagGap), flagY, flagW, flagH };
    }

    float pad     = 20.0f * s;
    float sliderW = panelW - 2.0f * pad;
    float sliderH = 28.0f * s;

    massSlider.bounds = { panelX + pad, 98.0f * s, sliderW, sliderH };
    massSlider.label  = loc(LKey::MassSun);

    hrRect = { panelX + 10.0f * s, 155.0f * s, panelW - 20.0f * s, 260.0f * s };

    // Quit button at bottom of right panel
    btnQuit = { panelX + 5.0f * s, (float)sh - 46.0f * s, panelW - 10.0f * s, 36.0f * s };

    // ── Bottom-left panel ─────────────────────────────
    float boxW = 240.0f * s;
    float boxH = 120.0f * s;
    float boxX = 10.0f * s;
    float boxY = (float)sh - boxH - 10.0f * s;
    bottomLeftPanel = { boxX, boxY, boxW, boxH };

    float bPad = 14.0f * s;
    float btnW = (boxW - 3 * bPad) * 0.5f;
    float btnH = 36.0f * s;

    timeSlider.bounds = { boxX + bPad, boxY + 38.0f * s, boxW - 2 * bPad, 24.0f * s };
    timeSlider.label  = loc(LKey::SimSpeed);

    btnPause = { boxX + bPad,            boxY + boxH - btnH - bPad, btnW, btnH };
    btnReset = { boxX + 2 * bPad + btnW, boxY + boxH - btnH - bPad, btnW, btnH };

    // ── Zoom panel — bottom-center ────────────────────
    float zpW = 200.0f * s;
    float zpH = 80.0f * s;
    float zpX = (float)sw * 0.5f - zpW * 0.5f;
    float zpY = (float)sh - zpH - 10.0f * s;
    zoomPanel = { zpX, zpY, zpW, zpH };

    float zbW = 60.0f * s;
    float zbH = 32.0f * s;
    float zbGap = 20.0f * s;
    float zbTotalW = 2 * zbW + zbGap;
    float zbX = zpX + (zpW - zbTotalW) * 0.5f;
    float zbY = zpY + zpH - zbH - 8.0f * s;
    btnZoomIn  = { zbX, zbY, zbW, zbH };
    btnZoomOut = { zbX + zbW + zbGap, zbY, zbW, zbH };
}

// ── UI init ────────────────────────────────────────────────────

void UI::init(int screenW, int screenH) {
    sw = screenW;
    sh = screenH;

    // Build codepoint set: ASCII + Latin Extended + German/European + common symbols
    int codepoints[1024];
    int cpCount = 0;
    for (int i = 32;   i <= 126;  i++) codepoints[cpCount++] = i;   // ASCII
    for (int i = 160;  i <= 591;  i++) codepoints[cpCount++] = i;   // Latin-1 Supp + Latin Extended-A/B
    for (int i = 8192; i <= 8303; i++) codepoints[cpCount++] = i;   // General Punctuation (—, –, …)
    for (int i = 8320; i <= 8399; i++) codepoints[cpCount++] = i;   // Superscripts/Subscripts
    codepoints[cpCount++] = 0x00D7;  // ×
    codepoints[cpCount++] = 0x2609;  // ☉ (sun symbol)

    const char *fontRelPaths[] = {
        "assets/fonts/Inter_18pt-Medium.ttf",
        "assets/fonts/Inter_18pt-Regular.ttf",
        "assets/fonts/Inter_24pt-Bold.ttf",
        "assets/fonts/ui_font.ttf",
    };
    fontLoaded = false;
    for (auto &rel : fontRelPaths) {
        const char *path = AssetPath(rel);
        if (FileExists(path)) {
            uiFont = LoadFontEx(path, 48, codepoints, cpCount);
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
            TraceLog(LOG_INFO, "UI font loaded: %s (%d codepoints)", path, cpCount);
            break;
        }
    }
    if (!fontLoaded) {
        uiFont = GetFontDefault();
    }

    massSlider.minVal = 0.1f;
    massSlider.maxVal = 50.0f;
    massSlider.value  = 1.0f;
    massSlider.fmt    = "%.1f";

    timeSlider.minVal = 0.0f;
    timeSlider.maxVal = 5.0f;
    timeSlider.value  = 1.0f;
    timeSlider.fmt    = "%.2fx";

    loadFlags();
    layout();

    loadHRData(AssetPath("assets/hr_data.csv"));
}

void UI::shutdown() {
    if (fontLoaded) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        if (flagsLoaded[i]) {
            UnloadTexture(flagTextures[i]);
            flagsLoaded[i] = false;
        }
    }
}

// ── Language selector ──────────────────────────────────────────

void UI::updateLanguageSelector() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < LANGUAGE_COUNT; i++) {
            if (CheckCollisionPointRec(mouse, flagRects[i])) {
                if (i != currentLang) {
                    currentLang = i;
                    layout();
                }
                return;
            }
        }
    }
    if (GetTouchPointCount() > 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int t = 0; t < GetTouchPointCount(); t++) {
            Vector2 tp = GetTouchPosition(t);
            for (int i = 0; i < LANGUAGE_COUNT; i++) {
                if (CheckCollisionPointRec(tp, flagRects[i])) {
                    if (i != currentLang) {
                        currentLang = i;
                        layout();
                    }
                    return;
                }
            }
        }
    }
}

void UI::drawLanguageSelector() const {
    float s = scale();
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        Rectangle r = flagRects[i];

        DrawRectangleRounded(r, 0.15f, 4, Color{30, 30, 45, 200});

        Color tint = (i == currentLang) ? WHITE : Color{160, 160, 160, 180};
        if (flagsLoaded[i]) {
            DrawTexturePro(flagTextures[i],
                {0, 0, (float)flagTextures[i].width, (float)flagTextures[i].height},
                r, {0, 0}, 0.0f, tint);
        } else {
            float fs = r.height * 0.55f;
            const char *code = LANGUAGES[i].code;
            int tw = measureText(code, fs);
            drawText(code,
                     r.x + (r.width - tw) * 0.5f,
                     r.y + (r.height - fs) * 0.5f,
                     fs, tint);
        }

        if (i == currentLang) {
            DrawRectangleRoundedLinesEx(r, 0.15f, 4, 2.0f * s, Color{100, 180, 255, 255});
        } else {
            DrawRectangleRoundedLinesEx(r, 0.15f, 4, 1.0f * s, Color{70, 70, 90, 150});
        }
    }
}

// ── Panel backgrounds ──────────────────────────────────────────

void UI::drawPanelBackgrounds() const {
    float s = scale();

    DrawRectangleRounded(rightPanel, 0.02f, 8, Color{12, 14, 22, 210});
    DrawRectangleRoundedLinesEx(rightPanel, 0.02f, 8, 1.0f * s, Color{50, 55, 75, 200});

    DrawRectangleRounded(bottomLeftPanel, 0.08f, 8, Color{12, 14, 22, 210});
    DrawRectangleRoundedLinesEx(bottomLeftPanel, 0.08f, 8, 1.0f * s, Color{50, 55, 75, 200});

    DrawRectangleRounded(zoomPanel, 0.08f, 8, Color{12, 14, 22, 210});
    DrawRectangleRoundedLinesEx(zoomPanel, 0.08f, 8, 1.0f * s, Color{50, 55, 75, 200});
}

// ── H-R Diagram ────────────────────────────────────────────────

static Vector2 hrToPixel(const Rectangle &r, float teff, float lum) {
    float tMin = 2000.0f, tMax = 40000.0f;
    float lMin = -4.5f,   lMax = 6.5f;

    float xFrac = 1.0f - (teff - tMin) / (tMax - tMin);
    float yFrac = 1.0f - (log10f(fmaxf(lum, 1e-6f)) - lMin) / (lMax - lMin);

    xFrac = std::clamp(xFrac, 0.0f, 1.0f);
    yFrac = std::clamp(yFrac, 0.0f, 1.0f);

    float mx = r.width  * 0.07f;
    float my = r.height * 0.058f;

    float px = r.x + mx + xFrac * (r.width  - 2.0f * mx);
    float py = r.y + my + yFrac * (r.height - 2.0f * my);
    return {px, py};
}

void UI::drawHRDiagram(float mass) const {
    float s = scale();

    DrawRectangleRounded(hrRect, 0.06f, 8, Color{8, 8, 16, 230});
    DrawRectangleRoundedLinesEx(hrRect, 0.06f, 8, 1.0f * s, Color{60, 65, 85, 200});

    float fs = 13.0f * s;
    const char *title = loc(LKey::HRDiagram);
    drawText(title, hrRect.x + hrRect.width * 0.5f -
             measureText(title, fs) * 0.5f, hrRect.y - fs - 4 * s, fs,
             Color{140,145,170,255});

    float labelFs = 11.0f * s;
    drawText(loc(LKey::Hot), hrRect.x + 4 * s,
             hrRect.y + hrRect.height + 4 * s, labelFs, Color{130,140,255,255});
    drawText(loc(LKey::Cool),
             hrRect.x + hrRect.width - measureText(loc(LKey::Cool), labelFs) - 4 * s,
             hrRect.y + hrRect.height + 4 * s, labelFs, Color{255,140,80,255});
    drawText("L", hrRect.x - 14 * s,
             hrRect.y + hrRect.height * 0.5f - 5 * s, labelFs, Color{140,145,170,200});

    float dotR = 3.0f * s;
    for (auto &pt : hrData) {
        Vector2 p = hrToPixel(hrRect, pt.teff, pt.luminosity);
        Color col = temperatureToColor(pt.teff);
        col.a = 120;
        DrawCircle((int)p.x, (int)p.y, dotR, col);
    }

    float r, l, t;
    stellarModel(mass, 0.0f, r, l, t);

    Vector2 starPos = hrToPixel(hrRect, t, l);
    Color starCol = temperatureToColor(t);

    DrawCircle((int)starPos.x, (int)starPos.y, 12 * s,
               Color{starCol.r, starCol.g, starCol.b, 40});
    DrawCircle((int)starPos.x, (int)starPos.y, 7 * s, starCol);
    DrawCircleLines((int)starPos.x, (int)starPos.y, (int)(9 * s),
                    Color{255,255,255,200});
}

// ── Zoom controls — bottom-center panel ────────────────────────

void UI::drawZoomControls() const {
    float s  = scale();
    Vector2 mouse = GetMousePosition();

    // Title and zoom level on top row
    float headerFs = 14.0f * s;
    drawText(loc(LKey::Zoom), zoomPanel.x + 12 * s, zoomPanel.y + 8 * s,
             headerFs, Color{180, 180, 200, 255});

    char zoomBuf[32];
    snprintf(zoomBuf, sizeof(zoomBuf), "%.1fx", cameraZoom);
    int ztw = measureText(zoomBuf, headerFs);
    drawText(zoomBuf, zoomPanel.x + zoomPanel.width - ztw - 12 * s,
             zoomPanel.y + 8 * s, headerFs, WHITE);

    // Buttons
    float btnFs = 20.0f * s;

    {
        bool over = CheckCollisionPointRec(mouse, btnZoomIn);
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnZoomIn, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnZoomIn, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = loc(LKey::ZoomIn);
        int tw = measureText(txt, btnFs);
        drawText(txt, btnZoomIn.x + btnZoomIn.width * 0.5f - tw * 0.5f,
                 btnZoomIn.y + btnZoomIn.height * 0.5f - btnFs * 0.5f, btnFs, WHITE);
    }

    {
        bool over = CheckCollisionPointRec(mouse, btnZoomOut);
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnZoomOut, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnZoomOut, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = loc(LKey::ZoomOut);
        int tw = measureText(txt, btnFs);
        drawText(txt, btnZoomOut.x + btnZoomOut.width * 0.5f - tw * 0.5f,
                 btnZoomOut.y + btnZoomOut.height * 0.5f - btnFs * 0.5f, btnFs, WHITE);
    }
}

// ── Time controls (draw only) ──────────────────────────────────

void UI::drawTimeControls(const Simulation &sim) const {
    float s = scale();
    Vector2 mouse = GetMousePosition();

    {
        bool over = CheckCollisionPointRec(mouse, btnPause);
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnPause, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnPause, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = (sim.timeScale <= 0.001f) ? loc(LKey::Play) : loc(LKey::Pause);
        float fs = 15.0f * s;
        int tw = measureText(txt, fs);
        drawText(txt, btnPause.x + btnPause.width * 0.5f - tw * 0.5f,
                 btnPause.y + btnPause.height * 0.5f - fs * 0.5f, fs, WHITE);
    }

    {
        bool over = CheckCollisionPointRec(mouse, btnReset);
        Color bg = over ? Color{100, 45, 45, 255} : Color{55, 30, 30, 240};
        DrawRectangleRounded(btnReset, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnReset, 0.3f, 8, 1.0f * s, Color{130, 80, 80, 200});
        const char *txt = loc(LKey::Clear);
        float fs = 15.0f * s;
        int tw = measureText(txt, fs);
        drawText(txt, btnReset.x + btnReset.width * 0.5f - tw * 0.5f,
                 btnReset.y + btnReset.height * 0.5f - fs * 0.5f, fs, WHITE);
    }
}

// ── Main update ────────────────────────────────────────────────

void UI::update(Simulation &sim, float &outMass) {
    int newW = GetScreenWidth();
    int newH = GetScreenHeight();
    if (newW != sw || newH != sh) {
        sw = newW;
        sh = newH;
        layout();
    }

    zoomRequest = 0.0f;

    updateLanguageSelector();

    massSlider.update();

    if (timeSlider.update()) {
        if (sim.timeScale > 0.001f)
            sim.timeScale = timeSlider.value;
    }

    // Zoom buttons (continuous while held)
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btnZoomIn))  zoomRequest =  1.0f;
        if (CheckCollisionPointRec(mouse, btnZoomOut)) zoomRequest = -1.0f;
    }
    for (int t = 0; t < GetTouchPointCount(); t++) {
        Vector2 tp = GetTouchPosition(t);
        if (CheckCollisionPointRec(tp, btnZoomIn))  zoomRequest =  1.0f;
        if (CheckCollisionPointRec(tp, btnZoomOut)) zoomRequest = -1.0f;
    }

    // Pause / Clear
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btnPause)) {
            if (sim.timeScale > 0.001f) sim.timeScale = 0.0f;
            else sim.timeScale = timeSlider.value > 0.01f ? timeSlider.value : 1.0f;
        }
        if (CheckCollisionPointRec(mouse, btnReset))
            sim.clear();
    }

    // Quit
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btnQuit))
            quitRequested = true;
    }
    for (int t = 0; t < GetTouchPointCount(); t++) {
        if (CheckCollisionPointRec(GetTouchPosition(t), btnQuit) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            quitRequested = true;
        }
    }

    outMass = massSlider.value;
}

// ── Main draw ──────────────────────────────────────────────────

void UI::draw(const Simulation &sim, float mass) {
    float s = scale();

    drawPanelBackgrounds();
    drawLanguageSelector();

    float titleFs = 20.0f * s;
    drawText(loc(LKey::StarProperties),
             rightPanel.x + 20 * s, 48.0f * s, titleFs, Color{200,205,230,255});

    massSlider.draw(uiFont, 14.0f * s);
    drawHRDiagram(mass);

    // Star count — below HR diagram inside right panel
    {
        char buf[64];
        snprintf(buf, sizeof(buf), loc(LKey::StarsCount), sim.starCount, MAX_STARS);
        float counterY = hrRect.y + hrRect.height + 12.0f * s;
        drawText(buf, rightPanel.x + 15 * s, counterY, 13 * s, Color{100,105,130,255});
    }

    drawZoomControls();

    timeSlider.draw(uiFont, 14.0f * s);
    drawTimeControls(sim);

    // Help text
    drawText(loc(LKey::TouchDragHint), 12 * s, 12 * s, 16 * s, Color{160,165,185,200});
    drawText(loc(LKey::ControlsHint),  12 * s, 32 * s, 13 * s, Color{100,105,130,180});

    // Quit button (visual)
    {
        Vector2 mouse = GetMousePosition();
        bool over = CheckCollisionPointRec(mouse, btnQuit);
        Color bg = over ? Color{70, 55, 55, 240} : Color{35, 30, 38, 220};
        DrawRectangleRounded(btnQuit, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnQuit, 0.3f, 8, 1.0f * s, Color{100, 80, 90, 200});

        const char *txt = loc(LKey::BackToApp);
        float fs = 14.0f * s;
        int tw = measureText(txt, fs);
        drawText(txt, btnQuit.x + btnQuit.width * 0.5f - tw * 0.5f,
                 btnQuit.y + btnQuit.height * 0.5f - fs * 0.5f, fs, Color{220,200,200,255});
    }

    if (debugMode) {
        float cx = sw * 0.5f - 40 * s;
        DrawFPS((int)cx, (int)(10 * s));
        drawText(TextFormat("dt: %.3f ms", GetFrameTime() * 1000.0f),
                 cx, 30 * s, 13 * s, GREEN);
        drawText(TextFormat("Time scale: %.3f", sim.timeScale),
                 cx, 46 * s, 13 * s, GREEN);
    }
}

bool UI::isOverUI(Vector2 pos) const {
    if (CheckCollisionPointRec(pos, rightPanel))      return true;
    if (CheckCollisionPointRec(pos, bottomLeftPanel))  return true;
    if (CheckCollisionPointRec(pos, zoomPanel))        return true;
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        if (CheckCollisionPointRec(pos, flagRects[i])) return true;
    }
    return false;
}