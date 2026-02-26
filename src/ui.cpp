#include "ui.h"
#include "star.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "compat.h"

// ── Helpers ────────────────────────────────────────────────────

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
    float trackH   = 6.0f;
    float trackY   = bounds.y + bounds.height * 0.5f - trackH * 0.5f;

    // Convert value to 0..1 fraction (log-space if range spans > 10x)
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

    float labelY = bounds.y - fontSize - 4.0f;
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

    Rectangle hitArea = { bounds.x - 8, bounds.y - 12, bounds.width + 16, bounds.height + 24 };

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
        // Skip comments and empty lines
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

// ── UI init ────────────────────────────────────────────────────

void UI::init(int screenW, int screenH) {
    sw = screenW;
    sh = screenH;

    const char *fontPaths[] = {
        "assets/fonts/Inter_18pt-Medium.ttf",
        "assets/fonts/Inter_18pt-Regular.ttf",
        "assets/fonts/Inter_24pt-Bold.ttf",
        "assets/fonts/ui_font.ttf",
    };
    fontLoaded = false;
    for (auto &path : fontPaths) {
        if (FileExists(path)) {
            uiFont = LoadFontEx(path, 32, nullptr, 0);
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
            TraceLog(LOG_INFO, "UI font loaded: %s", path);
            break;
        }
    }
    if (!fontLoaded) {
        uiFont = GetFontDefault();
    }

    // ── Right panel ───────────────────────────────────
    float panelX = sw - 240.0f;
    float sliderW = 200.0f;
    float sliderH = 28.0f;

    massSlider = {
        { panelX + 20, 80, sliderW, sliderH },
        0.1f, 50.0f, 1.0f, "Mass (Msun)", "%.1f"
    };

    // HR diagram gets more space now (no age slider)
    hrRect = { panelX + 10, 140, 220, 260 };

    // ── Bottom-left ───────────────────────────────────
    float boxW = 240.0f;
    float boxH = 120.0f;
    float boxX = 10.0f;
    float boxY = sh - boxH - 10.0f;
    float pad  = 14.0f;
    float btnW = (boxW - 3 * pad) * 0.5f;
    float btnH = 36.0f;

    timeSlider = {
        { boxX + pad, boxY + 38, boxW - 2 * pad, 24 },
        0.0f, 5.0f, 1.0f, "Sim Speed", "%.2fx"
    };

    btnPause = { boxX + pad,            boxY + boxH - btnH - pad, btnW, btnH };
    btnReset = { boxX + 2 * pad + btnW, boxY + boxH - btnH - pad, btnW, btnH };
    // Quit button — top-left
    btnQuit = { (float)sw - 245, (float)sh - 56, 240, 36 };

    // Load HR background data
    loadHRData("assets/hr_data.csv");
}

void UI::shutdown() {
    if (fontLoaded) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
}

// ── Panel backgrounds ──────────────────────────────────────────

void UI::drawPanelBackgrounds() const {
    Rectangle rp = { (float)sw - 250.0f, 0, 250, (float)sh };
    DrawRectangleRounded(rp, 0.02f, 8, Color{12, 14, 22, 210});
    DrawRectangleRoundedLinesEx(rp, 0.02f, 8, 1.0f, Color{50, 55, 75, 200});

    float boxW = 240.0f, boxH = 120.0f, boxX = 10.0f, boxY = sh - boxH - 10.0f;
    Rectangle bl = { boxX, boxY, boxW, boxH };
    DrawRectangleRounded(bl, 0.08f, 8, Color{12, 14, 22, 210});
    DrawRectangleRoundedLinesEx(bl, 0.08f, 8, 1.0f, Color{50, 55, 75, 200});
}

// ── H-R Diagram ────────────────────────────────────────────────

// Map (teff, luminosity) → pixel position in HR rect
static Vector2 hrToPixel(const Rectangle &r, float teff, float lum) {
    float tMin = 2000.0f, tMax = 40000.0f;
    float lMin = -4.5f,   lMax = 6.5f;

    float xFrac = 1.0f - (teff - tMin) / (tMax - tMin);   // hot = left
    float yFrac = 1.0f - (log10f(fmaxf(lum, 1e-6f)) - lMin) / (lMax - lMin);

    xFrac = std::clamp(xFrac, 0.0f, 1.0f);
    yFrac = std::clamp(yFrac, 0.0f, 1.0f);

    float px = r.x + 15 + xFrac * (r.width - 30);
    float py = r.y + 15 + yFrac * (r.height - 30);
    return {px, py};
}

void UI::drawHRDiagram(float mass) const {
    DrawRectangleRounded(hrRect, 0.06f, 8, Color{8, 8, 16, 230});
    DrawRectangleRoundedLinesEx(hrRect, 0.06f, 8, 1.0f, Color{60, 65, 85, 200});

    float fs = 13.0f;
    const char *title = "H-R Diagram";
    drawText(title, hrRect.x + hrRect.width * 0.5f -
             measureText(title, fs) * 0.5f, hrRect.y - fs - 4, fs,
             Color{140,145,170,255});

    drawText("Hot",  hrRect.x + 4, hrRect.y + hrRect.height + 4, 11, Color{130,140,255,255});
    drawText("Cool", hrRect.x + hrRect.width - measureText("Cool", 11) - 4,
             hrRect.y + hrRect.height + 4, 11, Color{255,140,80,255});
    drawText("L", hrRect.x - 14, hrRect.y + hrRect.height * 0.5f - 5, 11, Color{140,145,170,200});

    // Background data points from CSV
    for (auto &pt : hrData) {
        Vector2 p = hrToPixel(hrRect, pt.teff, pt.luminosity);
        Color col = temperatureToColor(pt.teff);
        col.a = 120;
        DrawCircle((int)p.x, (int)p.y, 3.0f, col);
    }

    // Faint main sequence band
    for (int i = 0; i < 3; i++) {
        float off = (float)(i - 1) * 6.0f;
        DrawLine(
            (int)(hrRect.x + 15 + off), (int)(hrRect.y + hrRect.height - 15),
            (int)(hrRect.x + hrRect.width - 15 + off), (int)(hrRect.y + 15),
            Color{35, 40, 55, 80});
    }

    // Current slider star at ZAMS (age=0)
    float r, l, t;
    stellarModel(mass, 0.0f, r, l, t);

    Vector2 starPos = hrToPixel(hrRect, t, l);
    Color starCol = temperatureToColor(t);

    // Glow around indicator
    DrawCircle((int)starPos.x, (int)starPos.y, 12, Color{starCol.r, starCol.g, starCol.b, 40});
    DrawCircle((int)starPos.x, (int)starPos.y, 7, starCol);
    DrawCircleLines((int)starPos.x, (int)starPos.y, 9, Color{255,255,255,200});
}

// ── Time controls ──────────────────────────────────────────────

void UI::drawTimeControls(Simulation &sim) {
    Vector2 mouse = GetMousePosition();
    bool overPause = CheckCollisionPointRec(mouse, btnPause);
    bool overReset = CheckCollisionPointRec(mouse, btnReset);

    {
        Color bg = overPause ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnPause, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnPause, 0.3f, 8, 1.0f, Color{90, 95, 130, 200});
        const char *txt = (sim.timeScale <= 0.001f) ? "Play" : "Pause";
        float fs = 15.0f;
        int tw = measureText(txt, fs);
        drawText(txt, btnPause.x + btnPause.width * 0.5f - tw * 0.5f,
                 btnPause.y + btnPause.height * 0.5f - fs * 0.5f, fs, WHITE);
    }

    {
        Color bg = overReset ? Color{100, 45, 45, 255} : Color{55, 30, 30, 240};
        DrawRectangleRounded(btnReset, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnReset, 0.3f, 8, 1.0f, Color{130, 80, 80, 200});
        const char *txt = "Clear";
        float fs = 15.0f;
        int tw = measureText(txt, fs);
        drawText(txt, btnReset.x + btnReset.width * 0.5f - tw * 0.5f,
                 btnReset.y + btnReset.height * 0.5f - fs * 0.5f, fs, WHITE);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (overPause) {
            if (sim.timeScale > 0.001f) sim.timeScale = 0.0f;
            else sim.timeScale = timeSlider.value > 0.01f ? timeSlider.value : 1.0f;
        }
        if (overReset) sim.clear();
    }
}

// ── Main update / draw ─────────────────────────────────────────

void UI::update(Simulation &sim, float &outMass) {
    massSlider.update();

    if (timeSlider.update()) {
        if (sim.timeScale > 0.001f)
            sim.timeScale = timeSlider.value;
    }

    outMass = massSlider.value;
}

void UI::draw(const Simulation &sim, float mass) {
    drawPanelBackgrounds();

    drawText("Star Properties", sw - 240.0f + 20, 20, 20, Color{200,205,230,255});

    massSlider.draw(uiFont, 14.0f);
    drawHRDiagram(mass);

    timeSlider.draw(uiFont, 14.0f);
    drawTimeControls(const_cast<Simulation &>(sim));

    char buf[64];
    snprintf(buf, sizeof(buf), "Stars: %d / %d", sim.starCount, MAX_STARS);
    drawText(buf, sw - 235.0f, sh - 30.0f, 13, Color{100,105,130,255});

    drawText("Touch & drag to place a star",                12, 12, 16, Color{160,165,185,200});
    drawText("Scroll/pinch zoom - Right-drag orbit - 3-finger pan", 12, 32, 13, Color{100,105,130,180});

    // Quit / back button
    {
        Vector2 mouse = GetMousePosition();
        bool over = CheckCollisionPointRec(mouse, btnQuit);
        Color bg = over ? Color{70, 55, 55, 240} : Color{35, 30, 38, 220};
        DrawRectangleRounded(btnQuit, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnQuit, 0.3f, 8, 1.0f, Color{100, 80, 90, 200});

        const char *txt = "Zuruck zur App-Ubersicht";
        float fs = 14.0f;
        int tw = measureText(txt, fs);
        drawText(txt, btnQuit.x + btnQuit.width * 0.5f - tw * 0.5f,
                 btnQuit.y + btnQuit.height * 0.5f - fs * 0.5f, fs, Color{220,200,200,255});

        if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            quitRequested = true;
        }
        for (int t = 0; t < GetTouchPointCount(); t++) {
            if (CheckCollisionPointRec(GetTouchPosition(t), btnQuit) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                quitRequested = true;
            }
        }
    }

    if (debugMode) {
        DrawFPS(sw / 2 - 40, 10);
        drawText(TextFormat("dt: %.3f ms", GetFrameTime() * 1000.0f), sw / 2 - 40, 30, 13, GREEN);
        drawText(TextFormat("Time scale: %.3f", sim.timeScale), sw / 2 - 40, 46, 13, GREEN);
    }
}

bool UI::isOverUI(Vector2 pos) const {
    if (CheckCollisionPointRec(pos, btnQuit)) return true;
    if (pos.x > sw - 250) return true;
    float boxW = 240.0f, boxH = 120.0f, boxX = 10.0f, boxY = sh - boxH - 10.0f;
    if (pos.x >= boxX && pos.x <= boxX + boxW && pos.y >= boxY && pos.y <= boxY + boxH)
        return true;
    return false;
}