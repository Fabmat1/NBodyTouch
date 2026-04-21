#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>

#include "ui.h"
#include "star.h"
#include "assets.h"
#include "localization.h"
#include "compat.h"
#include "pointer.h"

// ── Helpers ────────────────────────────────────────────────────

float UI::scale() const {
    float s = fminf((float)sw / 1920.0f, (float)sh / 1080.0f);
#ifdef PLATFORM_ANDROID
    s *= 2.2f;
#endif
    return s;
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

void UI::drawTextWrapped(const char *text, Rectangle bounds,
                         float fontSize, Color color) const {
    float lineH  = fontSize * 1.4f;
    float spaceW = (float)measureText(" ", fontSize);
    float x = bounds.x;
    float y = bounds.y;

    const char *p = text;
    char word[512];

    while (*p) {
        if (y + fontSize > bounds.y + bounds.height) break;

        if (*p == '\n') {
            x = bounds.x;
            y += lineH;
            p++;
            continue;
        }
        if (*p == ' ') {
            if (x > bounds.x) x += spaceW;
            p++;
            continue;
        }

        int wlen = 0;
        while (p[wlen] && p[wlen] != ' ' && p[wlen] != '\n' && wlen < 510)
            wlen++;
        memcpy(word, p, wlen);
        word[wlen] = '\0';
        p += wlen;

        float ww = (float)measureText(word, fontSize);

        if (x + ww > bounds.x + bounds.width && x > bounds.x) {
            x = bounds.x;
            y += lineH;
            if (y + fontSize > bounds.y + bounds.height) break;
        }

        drawText(word, x, y, fontSize, color);
        x += ww;
    }
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
    Vector2 pointer = Pointer::position();
    bool changed    = false;

    float padX = bounds.height * 0.3f;
    float padY = bounds.height * 0.45f;
    Rectangle hitArea = { bounds.x - padX, bounds.y - padY,
                          bounds.width + 2 * padX, bounds.height + 2 * padY };

    if (Pointer::pressed() && CheckCollisionPointRec(pointer, hitArea)) {
        dragging = true;
    }
    if (Pointer::released()) {
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

        if (!useLog && snapStep > 0.0f) {
            newVal = roundf(newVal / snapStep) * snapStep;
        }

        newVal = std::clamp(newVal, minVal, maxVal);
        if (newVal != value) { value = newVal; changed = true; }
    };

    if (dragging) {
        applyPos(pointer);
    } else {
        for (int t = 0; t < Pointer::touchCount(); t++) {
            Vector2 tp = Pointer::touchPosition(t);
            if (CheckCollisionPointRec(tp, hitArea)) applyPos(tp);
        }
    }

    return changed;
}

// ── HR Data loading ────────────────────────────────────────────

void UI::loadHRData(const char *path) {
    hrData.clear();
#ifndef PLATFORM_ANDROID
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "HR data file not found: %s", path);
        return;
    }
#endif
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

#ifndef PLATFORM_ANDROID
        if (!FileExists(fullPath)) {
            flagsLoaded[i] = false;
            TraceLog(LOG_WARNING, "Flag texture not found: %s", fullPath);
            continue;
        }
#endif
        flagTextures[i] = LoadTexture(fullPath);
        SetTextureFilter(flagTextures[i], TEXTURE_FILTER_BILINEAR);
        flagsLoaded[i] = (flagTextures[i].id != 0);
    }
}

// ── Intro image loading ────────────────────────────────────────

void UI::loadIntroImages() {
    static const char *paths[INTRO_IMAGE_COUNT] = {
        "assets/intro/page1_touch_drag.png",
        "assets/intro/page2_zoom.png",
        "assets/intro/page2_pan.png",
        "assets/intro/page3_speed.png",
        "assets/intro/page4_zoom.png",
        "assets/intro/page5_back.png",
    };
    for (int i = 0; i < INTRO_IMAGE_COUNT; i++) {
        const char *fullPath = AssetPath(paths[i]);
#ifndef PLATFORM_ANDROID
        if (!FileExists(fullPath)) {
            introImagesLoaded[i] = false;
            continue;
        }
#endif
        introImages[i] = LoadTexture(fullPath);
        if (introImages[i].id != 0) {
            SetTextureFilter(introImages[i], TEXTURE_FILTER_BILINEAR);
            introImagesLoaded[i] = true;
        } else {
            introImagesLoaded[i] = false;
        }
    }
}

void UI::loadScenarioImages() {
    static const char *paths[SCENARIO_COUNT] = {
        "assets/scenarios/triple.png",
        "assets/scenarios/fantastic_four.png",
        "assets/scenarios/freefall.png",
        "assets/scenarios/infinity.png",
        "assets/scenarios/recursion.png",
        "assets/scenarios/ping_pong.png",
    };
    for (int i = 0; i < SCENARIO_COUNT; i++) {
        const char *fullPath = AssetPath(paths[i]);
#ifndef PLATFORM_ANDROID
        if (!FileExists(fullPath)) {
            scenarioImagesLoaded[i] = false;
            continue;
        }
#endif
        scenarioImages[i] = LoadTexture(fullPath);
        if (scenarioImages[i].id != 0) {
            SetTextureFilter(scenarioImages[i], TEXTURE_FILTER_BILINEAR);
            scenarioImagesLoaded[i] = true;
        } else {
            scenarioImagesLoaded[i] = false;
        }
    }
}

// ── Layout ─────────────────────────────────────────────────────

void UI::layout() {
    float s = scale();

    float panelW = 250.0f * s;
    float panelX = (float)sw - panelW;
    rightPanel = { panelX, 0, panelW, (float)sh };

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
    
    float scPad    = 6.0f * s;
    float scGap    = 5.0f * s;
    float scBtnW   = (panelW - 2.0f * scPad - scGap) * 0.5f;
    float scBtnH   = 85.0f * s;
    float scHeadH  = 22.0f * s;
    float gridY = hrRect.y + hrRect.height + 38.0f * s + scHeadH;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 2; col++) {
            int idx = row * 2 + col;
            scenarioBtns[idx] = {
                panelX + scPad + col * (scBtnW + scGap),
                gridY + row * (scBtnH + scGap),
                scBtnW, scBtnH
            };
        }
    }

    btnQuit = { panelX + 5.0f * s, (float)sh - 46.0f * s, panelW - 10.0f * s, 36.0f * s };

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

    // ── Zoom panel (taller to fit reset-camera button) ────────────────
    float zpW = 200.0f * s;
    float zpH = 100.0f * s;
    float zpX = (float)sw * 0.5f - zpW * 0.5f;
    float zpY = (float)sh - zpH - 10.0f * s;
    zoomPanel = { zpX, zpY, zpW, zpH };

    float zbW = 48.0f * s;
    float zbH = 28.0f * s;
    float zbGap = 8.0f * s;
    float zbTotalW = 3 * zbW + 2 * zbGap;
    float zbX = zpX + (zpW - zbTotalW) * 0.5f;
    float zbY = zpY + 28.0f * s;
    btnZoomIn   = { zbX,                       zbY, zbW, zbH };
    btnTrackCOM = { zbX +     (zbW + zbGap),   zbY, zbW, zbH };
    btnZoomOut  = { zbX + 2 * (zbW + zbGap),   zbY, zbW, zbH };

    float rcY = zbY + zbH + 6.0f * s;
    float rcH = 26.0f * s;
    btnResetCamera = { zbX, rcY, zbTotalW, rcH };

    // ── Info button ───────────────────────────────────────────────────
    float infoSz = 32.0f * s;
    btnInfo = { 12.0f * s, 52.0f * s, infoSz, infoSz };

    layoutIntroPage();
}

void UI::layoutIntroPage() {
    float s = scale();
    float pad = 16.0f * s;

    if (introPage == 0) {
        float dlgW = fminf(520.0f * s, (float)sw * 0.50f);
        float dlgH = fminf(380.0f * s, (float)sh * 0.55f);
        float dlgX = ((float)sw - dlgW) * 0.5f;
        float dlgY = ((float)sh - dlgH) * 0.5f;
        introTooltipRect = { dlgX, dlgY, dlgW, dlgH };
    } else if (introPage == 1) {
        float tw = 400.0f * s;
        float th = 280.0f * s;
        introTooltipRect = { 12.0f * s, 60.0f * s, tw, th };
    } else {
        Rectangle anchor = {};
        bool anchorAbove = false;
        bool anchorLeft  = false;

        switch (introPage) {
            case 2: anchor = bottomLeftPanel; anchorAbove = true; break;
            case 3: anchor = zoomPanel;       anchorAbove = true; break;
            case 4: anchor = btnQuit;         anchorAbove = true; anchorLeft = true; break;
        }

        float tw = 360.0f * s;
        float th = 220.0f * s;
        float tx, ty;

        if (anchorAbove)
            ty = anchor.y - th - 10.0f * s;
        else
            ty = anchor.y + anchor.height + 10.0f * s;

        if (anchorLeft)
            tx = anchor.x;
        else
            tx = anchor.x + anchor.width * 0.5f - tw * 0.5f;

        tx = std::clamp(tx, 8.0f * s, (float)sw - tw - 8.0f * s);
        ty = std::clamp(ty, 8.0f * s, (float)sh - th - 8.0f * s);
        introTooltipRect = { tx, ty, tw, th };
    }

    float closeSz = 28.0f * s;
    introBtnClose = {
        introTooltipRect.x + introTooltipRect.width - closeSz - pad * 0.3f,
        introTooltipRect.y + pad * 0.3f,
        closeSz, closeSz
    };

    float navBtnW = 80.0f * s;
    float navBtnH = 32.0f * s;
    float navY = introTooltipRect.y + introTooltipRect.height - pad - navBtnH;
    introBtnNext = {
        introTooltipRect.x + introTooltipRect.width - pad - navBtnW,
        navY, navBtnW, navBtnH
    };
    introBtnBack = {
        introBtnNext.x - navBtnW - 8.0f * s,
        navY, navBtnW, navBtnH
    };
}

// ── UI init ────────────────────────────────────────────────────

void UI::init(int screenW, int screenH) {
    sw = screenW;
    sh = screenH;

    int codepoints[1024];
    int cpCount = 0;
    for (int i = 32;   i <= 126;  i++) codepoints[cpCount++] = i;
    for (int i = 160;  i <= 591;  i++) codepoints[cpCount++] = i;
    for (int i = 8192; i <= 8303; i++) codepoints[cpCount++] = i;
    for (int i = 8320; i <= 8399; i++) codepoints[cpCount++] = i;
    codepoints[cpCount++] = 0x00D7;
    codepoints[cpCount++] = 0x2609;
    codepoints[cpCount++] = 0x00BF;
    codepoints[cpCount++] = 0x00A1;

    const char *fontRelPaths[] = {
        "assets/fonts/Inter_18pt-Medium.ttf",
        "assets/fonts/Inter_18pt-Regular.ttf",
        "assets/fonts/Inter_24pt-Bold.ttf",
        "assets/fonts/ui_font.ttf",
    };
    fontLoaded = false;
    for (auto &rel : fontRelPaths) {
        const char *path = AssetPath(rel);
#ifndef PLATFORM_ANDROID
        if (!FileExists(path)) continue;
#endif
        uiFont = LoadFontEx(path, 48, codepoints, cpCount);
        if (uiFont.texture.id != 0) {
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

    timeSlider.minVal   = 0.0f;
    timeSlider.maxVal   = 5.0f;
    timeSlider.value    = 1.0f;
    timeSlider.fmt      = "%.2fx";
    timeSlider.snapStep = 0.25f;

    loadFlags();
    loadIntroImages();
    loadScenarioImages();
    layout();

    loadHRData(AssetPath("assets/hr_data.csv"));

    showIntroDialog = true;
    introPage       = 0;
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
    for (int i = 0; i < INTRO_IMAGE_COUNT; i++) {
        if (introImagesLoaded[i]) {
            UnloadTexture(introImages[i]);
            introImagesLoaded[i] = false;
        }
    }
    for (int i = 0; i < SCENARIO_COUNT; i++) {
        if (scenarioImagesLoaded[i]) {
            UnloadTexture(scenarioImages[i]);
            scenarioImagesLoaded[i] = false;
        }
    }
}

// ── Language selector ──────────────────────────────────────────

void UI::updateLanguageSelector() {
    if (Pointer::pressed()) {
        Vector2 pos = Pointer::position();
        for (int i = 0; i < LANGUAGE_COUNT; i++) {
            if (CheckCollisionPointRec(pos, flagRects[i])) {
                if (i != currentLang) {
                    currentLang = i;
                    layout();
                }
                return;
            }
        }
        for (int t = 0; t < Pointer::touchCount(); t++) {
            Vector2 tp = Pointer::touchPosition(t);
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

    float tClamped = std::clamp(teff, tMin, tMax);
    float xFrac = 1.0f - (log10f(tClamped) - log10f(tMin)) /
                         (log10f(tMax)     - log10f(tMin));
    float yFrac = 1.0f - (log10f(fmaxf(lum, 1e-6f)) - lMin) / (lMax - lMin);

    xFrac = std::clamp(xFrac, 0.0f, 1.0f);
    yFrac = std::clamp(yFrac, 0.0f, 1.0f);

    float px = 12.0f;
    float py = 12.0f;

    return { r.x + px + xFrac * (r.width - 2.0f * px),
             r.y + py + yFrac * (r.height - 2.0f * py) };
}

void UI::drawHRDiagram(float mass) const {
    float s = scale();

    float marginL = 15.0f * s;
    float marginB = 12.0f * s;
    float marginT = 6.0f * s;
    float marginR = 6.0f * s;

    Rectangle plotRect = {
        hrRect.x + marginL,
        hrRect.y + marginT,
        hrRect.width - marginL - marginR,
        hrRect.height - marginT - marginB
    };

    DrawRectangleRounded(plotRect, 0.06f, 8, Color{8, 8, 16, 230});
    DrawRectangleRoundedLinesEx(plotRect, 0.06f, 8, 1.0f * s, Color{60, 65, 85, 200});

    float fs = 13.0f * s;
    const char *title = loc(LKey::HRDiagram);
    drawText(title, plotRect.x + plotRect.width * 0.5f -
             measureText(title, fs) * 0.5f, plotRect.y - fs - 4 * s, fs,
             Color{140,145,170,255});

    auto drawRotated = [&](const char *text, float cx, float cy, float fontSize, Color col) {
        if (fontLoaded) {
            Vector2 sz = MeasureTextEx(uiFont, text, fontSize, 1.0f);
            Vector2 origin = { sz.x * 0.5f, fontSize * 0.5f };
            DrawTextPro(uiFont, text, {cx, cy}, origin, -90.0f, fontSize, 1.0f, col);
        } else {
            int tw = MeasureText(text, (int)fontSize);
            DrawText(text, (int)(cx - tw * 0.5f), (int)(cy - fontSize * 0.5f),
                     (int)fontSize, col);
        }
    };

    float lumFs = 11.0f * s;
    float axisCX = hrRect.x + marginL * 0.35f;

    drawRotated(loc(LKey::Bright), axisCX,
                plotRect.y + plotRect.height * 0.10f,
                lumFs, Color{200, 200, 140, 210});

    drawRotated(loc(LKey::Luminosity), axisCX,
                plotRect.y + plotRect.height * 0.50f,
                lumFs, Color{140, 145, 170, 220});

    drawRotated(loc(LKey::Dim), axisCX,
                plotRect.y + plotRect.height * 0.90f,
                lumFs, Color{140, 145, 170, 190});

    float axisFs = 11.0f * s;
    float bottomY = plotRect.y + plotRect.height + 4 * s;

    drawText(loc(LKey::Hot), plotRect.x + 1 * s, bottomY,
             axisFs, Color{130, 140, 255, 255});

    const char *coolTxt = loc(LKey::Cool);
    drawText(coolTxt,
             plotRect.x + plotRect.width - measureText(coolTxt, axisFs) - 1 * s,
             bottomY, axisFs, Color{255, 140, 80, 255});

    const char *tempLabel = loc(LKey::Temperature);
    int tempW = measureText(tempLabel, axisFs);
    drawText(tempLabel,
             plotRect.x + plotRect.width * 0.5f - tempW * 0.5f,
             bottomY, axisFs, Color{140, 145, 170, 210});

    float dotR = 3.0f * s;
    for (auto &pt : hrData) {
        Vector2 p = hrToPixel(plotRect, pt.teff, pt.luminosity);
        Color col = temperatureToColor(pt.teff);
        col.a = 120;
        DrawCircle((int)p.x, (int)p.y, dotR, col);
    }

    float r, l, t;
    stellarModel(mass, 0.0f, r, l, t);

    Vector2 starPos = hrToPixel(plotRect, t, l);
    Color starCol = temperatureToColor(t);

    DrawCircle((int)starPos.x, (int)starPos.y, 12 * s,
               Color{starCol.r, starCol.g, starCol.b, 40});
    DrawCircle((int)starPos.x, (int)starPos.y, 7 * s, starCol);
    DrawCircleLines((int)starPos.x, (int)starPos.y, (int)(9 * s),
                    Color{255, 255, 255, 200});
}

// ── Zoom controls ──────────────────────────────────────────────

void UI::drawZoomControls() const {
    float s = scale();
    Vector2 pointer = Pointer::position();

    float headerFs = 13.0f * s;
    drawText(loc(LKey::Zoom), zoomPanel.x + 8 * s, zoomPanel.y + 5 * s,
             headerFs, Color{180, 180, 200, 255});

    char zoomBuf[32];
    snprintf(zoomBuf, sizeof(zoomBuf), "%.1fx", cameraZoom);
    int ztw = measureText(zoomBuf, headerFs);
    drawText(zoomBuf, zoomPanel.x + zoomPanel.width - ztw - 8 * s,
             zoomPanel.y + 5 * s, headerFs, WHITE);

    float btnFs = 20.0f * s;

    // Zoom In
    {
        bool over = CheckCollisionPointRec(pointer, btnZoomIn) && Pointer::down();
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnZoomIn, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnZoomIn, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = loc(LKey::ZoomIn);
        int tw = measureText(txt, btnFs);
        drawText(txt, btnZoomIn.x + btnZoomIn.width * 0.5f - tw * 0.5f,
                 btnZoomIn.y + btnZoomIn.height * 0.5f - btnFs * 0.5f, btnFs, WHITE);
    }

    // Track Centre of Mass
    {
        bool active = trackingCOM;
        bool over   = CheckCollisionPointRec(pointer, btnTrackCOM) && Pointer::down();
        Color bg    = active ? Color{40, 90, 60, 255}
                             : (over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240});
        Color border = active ? Color{80, 200, 120, 220} : Color{90, 95, 130, 200};
        DrawRectangleRounded(btnTrackCOM, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnTrackCOM, 0.3f, 8, 1.5f * s, border);

        float cx = btnTrackCOM.x + btnTrackCOM.width  * 0.5f;
        float cy = btnTrackCOM.y + btnTrackCOM.height * 0.5f;
        float r  = btnTrackCOM.height * 0.28f;
        float arm = r * 0.55f;
        Color ic = active ? Color{120, 255, 160, 255} : Color{200, 200, 220, 220};
        DrawCircleLines((int)cx, (int)cy, r, ic);
        DrawLineEx({cx - r - arm, cy}, {cx + r + arm, cy}, 1.5f * s, ic);
        DrawLineEx({cx, cy - r - arm}, {cx, cy + r + arm}, 1.5f * s, ic);
    }

    // Zoom Out
    {
        bool over = CheckCollisionPointRec(pointer, btnZoomOut) && Pointer::down();
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnZoomOut, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnZoomOut, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = loc(LKey::ZoomOut);
        int tw = measureText(txt, btnFs);
        drawText(txt, btnZoomOut.x + btnZoomOut.width * 0.5f - tw * 0.5f,
                 btnZoomOut.y + btnZoomOut.height * 0.5f - btnFs * 0.5f, btnFs, WHITE);
    }

    // Reset Camera
    {
        bool over = CheckCollisionPointRec(pointer, btnResetCamera) && Pointer::down();
        Color bg = over ? Color{60, 65, 100, 255} : Color{35, 38, 55, 240};
        DrawRectangleRounded(btnResetCamera, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(btnResetCamera, 0.3f, 8, 1.0f * s, Color{90, 95, 130, 200});
        const char *txt = loc(LKey::ResetCamera);
        float rcFs = 12.0f * s;
        int tw = measureText(txt, rcFs);
        drawText(txt, btnResetCamera.x + btnResetCamera.width * 0.5f - tw * 0.5f,
                 btnResetCamera.y + btnResetCamera.height * 0.5f - rcFs * 0.5f,
                 rcFs, Color{200, 200, 220, 220});
    }
}

// ── Time controls ──────────────────────────────────────────────

void UI::drawTimeControls(const Simulation &sim) const {
    float s = scale();
    Vector2 pointer = Pointer::position();

    {
        bool over = CheckCollisionPointRec(pointer, btnPause) && Pointer::down();
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
        bool over = CheckCollisionPointRec(pointer, btnReset) && Pointer::down();
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

// ── Intro dialog update ────────────────────────────────────────

void UI::updateIntroDialog() {
    if (!showIntroDialog) return;

    layoutIntroPage();

    if (Pointer::pressed()) {
        Vector2 pos = Pointer::position();

        if (CheckCollisionPointRec(pos, introBtnClose)) {
            showIntroDialog    = false;
            introConsumedInput = true;
            return;
        }
        if (CheckCollisionPointRec(pos, introBtnNext)) {
            if (introPage < INTRO_PAGE_COUNT - 1) introPage++;
            else showIntroDialog = false;
            introConsumedInput = true;
            return;
        }
        if (introPage > 0 && CheckCollisionPointRec(pos, introBtnBack)) {
            introPage--;
            introConsumedInput = true;
            return;
        }
        if (CheckCollisionPointRec(pos, introTooltipRect)) {
            introConsumedInput = true;
            return;
        }

        // Click outside dialog — check if it's on normal UI
        bool onUI = CheckCollisionPointRec(pos, rightPanel) ||
                    CheckCollisionPointRec(pos, bottomLeftPanel) ||
                    CheckCollisionPointRec(pos, zoomPanel) ||
                    CheckCollisionPointRec(pos, btnTrackCOM) ||
                    CheckCollisionPointRec(pos, btnInfo);
        for (int i = 0; i < LANGUAGE_COUNT && !onUI; i++)
            if (CheckCollisionPointRec(pos, flagRects[i])) onUI = true;

        if (!onUI) {
            showIntroDialog    = false;
            introConsumedInput = true;
        }
    }
}

void UI::drawScenarios() const {
    float s = scale();
    Vector2 pointer = Pointer::position();

    static const LKey titleKeys[SCENARIO_COUNT] = {
        LKey::ScenarioTriple,   LKey::ScenarioFour,
        LKey::ScenarioFreefall, LKey::ScenarioInfinity,
        LKey::ScenarioRecursion, LKey::ScenarioPingPong,
    };

    // Header
    float headerY = scenarioBtns[0].y - 22.0f * s;
    float headerFs = 14.0f * s;
    drawText(loc(LKey::Scenarios),
             scenarioBtns[0].x, headerY, headerFs,
             Color{180, 185, 215, 255});

    for (int i = 0; i < SCENARIO_COUNT; i++) {
        Rectangle r = scenarioBtns[i];
        bool over   = CheckCollisionPointRec(pointer, r) && Pointer::down();

        Color bg     = over ? Color{55, 60, 90, 245} : Color{25, 28, 42, 235};
        Color border = over ? Color{110, 140, 210, 230} : Color{60, 65, 90, 200};
        DrawRectangleRounded(r, 0.1f, 8, bg);
        DrawRectangleRoundedLinesEx(r, 0.1f, 8, 1.0f * s, border);

        // Preview image
        float titleH = 20.0f * s;
        float imgPad = 4.0f * s;
        Rectangle imgR = {
            r.x + imgPad, r.y + imgPad,
            r.width - 2.0f * imgPad,
            r.height - titleH - imgPad
        };

        if (scenarioImagesLoaded[i]) {
            float ia = (float)scenarioImages[i].width /
                       (float)scenarioImages[i].height;
            float ba = imgR.width / imgR.height;
            Rectangle dst;
            if (ia > ba)
                dst = { imgR.x, imgR.y + (imgR.height - imgR.width / ia) * 0.5f,
                        imgR.width, imgR.width / ia };
            else
                dst = { imgR.x + (imgR.width - imgR.height * ia) * 0.5f, imgR.y,
                        imgR.height * ia, imgR.height };
            DrawTexturePro(scenarioImages[i],
                {0, 0, (float)scenarioImages[i].width,
                       (float)scenarioImages[i].height},
                dst, {0, 0}, 0.0f, WHITE);
        } else {
            DrawRectangleRounded(imgR, 0.06f, 8, Color{18, 20, 30, 220});
            DrawRectangleRoundedLinesEx(imgR, 0.06f, 8, 1.0f * s,
                                        Color{45, 50, 70, 180});
            const char *ph = "?";
            float pfs = imgR.height * 0.4f;
            int pw = measureText(ph, pfs);
            drawText(ph, imgR.x + (imgR.width - pw) * 0.5f,
                     imgR.y + (imgR.height - pfs) * 0.5f, pfs,
                     Color{70, 75, 100, 200});
        }

        // Title
        const char *title = loc(titleKeys[i]);
        float tfs = 10.0f * s;
        int tw = measureText(title, tfs);
        float maxTW = r.width - 6.0f * s;
        if (tw > maxTW) tfs = tfs * maxTW / (float)tw;
        tw = measureText(title, tfs);
        drawText(title,
                 r.x + (r.width - tw) * 0.5f,
                 r.y + r.height - titleH + (titleH - tfs) * 0.5f,
                 tfs, Color{215, 220, 240, 255});
    }
}

// ── Intro dialog draw ──────────────────────────────────────────

void UI::drawIntroDialog() {
    if (!showIntroDialog) return;

    float s = scale();
    float pad = 16.0f * s;
    Rectangle r = introTooltipRect;

    // Anchor highlight + connector line for pages 2-5
    if (introPage >= 2) {
        Rectangle anchor = {};
        switch (introPage) {
            case 2: anchor = bottomLeftPanel; break;
            case 3: anchor = zoomPanel;       break;
            case 4: anchor = btnQuit;         break;
        }

        DrawRectangleRoundedLinesEx(anchor, 0.08f, 8, 3.0f * s,
                                    Color{100, 160, 255, 200});

        Vector2 from = { r.x + r.width * 0.5f, r.y + r.height };
        Vector2 to   = { anchor.x + anchor.width * 0.5f, anchor.y };
        if (r.y + r.height > anchor.y) {
            from.y = r.y;
            to.y   = anchor.y + anchor.height;
        }
        DrawLineEx(from, to, 2.0f * s, Color{100, 160, 255, 150});
    }

    // Background
    DrawRectangleRounded(r, 0.06f, 8, Color{18, 20, 32, 240});
    DrawRectangleRoundedLinesEx(r, 0.06f, 8, 2.0f * s, Color{60, 70, 110, 220});

    // Close (X)
    {
        DrawRectangleRounded(introBtnClose, 0.3f, 8, Color{60, 30, 30, 220});
        float cx  = introBtnClose.x + introBtnClose.width * 0.5f;
        float cy  = introBtnClose.y + introBtnClose.height * 0.5f;
        float xsz = introBtnClose.width * 0.22f;
        DrawLineEx({cx - xsz, cy - xsz}, {cx + xsz, cy + xsz}, 2.0f * s,
                   Color{220, 150, 150, 255});
        DrawLineEx({cx + xsz, cy - xsz}, {cx - xsz, cy + xsz}, 2.0f * s,
                   Color{220, 150, 150, 255});
    }

    // Content
    static const LKey pageKeys[] = {
        LKey::IntroPage1, LKey::IntroPage2, LKey::IntroPage3,
        LKey::IntroPage4, LKey::IntroPage5,
    };
    // Image indices per page: {first, second} (-1 = none)
    static const int pageImgs[][2] = {
        { 0, -1}, { 1,  2}, { 3, -1}, { 4, -1}, { 5, -1},
    };

    float navH   = 32.0f * s + pad + 8.0f * s;
    float topPad = pad + (introPage == 0 ? 8.0f * s : 2.0f * s);

    auto drawImgFit = [&](int idx, Rectangle b) {
        if (idx < 0 || idx >= INTRO_IMAGE_COUNT) return;
        if (introImagesLoaded[idx]) {
            float ia = (float)introImages[idx].width / (float)introImages[idx].height;
            float ba = b.width / b.height;
            Rectangle dst;
            if (ia > ba)
                dst = { b.x, b.y + (b.height - b.width / ia) * 0.5f,
                        b.width, b.width / ia };
            else
                dst = { b.x + (b.width - b.height * ia) * 0.5f, b.y,
                        b.height * ia, b.height };
            DrawTexturePro(introImages[idx],
                {0, 0, (float)introImages[idx].width, (float)introImages[idx].height},
                dst, {0, 0}, 0.0f, WHITE);
        } else {
            DrawRectangleRounded(b, 0.05f, 8, Color{30, 32, 48, 220});
            DrawRectangleRoundedLinesEx(b, 0.05f, 8, 1.5f * s,
                                        Color{55, 60, 85, 180});
            static const char *imgDescs[] = {
                "[ Touch & Drag ]", "[ Zoom / Rotate ]", "[ Pan ]",
                "[ Speed Controls ]", "[ Zoom Panel ]", "[ Back Button ]",
            };
            const char *desc = (idx >= 0 && idx < INTRO_IMAGE_COUNT)
                               ? imgDescs[idx] : "[ ? ]";
            float pfs = 11.0f * s;
            int dtw = measureText(desc, pfs);
            drawText(desc, b.x + (b.width - dtw) * 0.5f,
                     b.y + (b.height - pfs) * 0.5f, pfs,
                     Color{100, 105, 130, 180});
        }
    };

    if (introPage == 0) {
        // Page 1: text left, image right
        float imgW  = r.width * 0.36f;
        float textW = r.width - 2.0f * pad - imgW - 8.0f * s;
        float textH = r.height - topPad - navH;
        Rectangle textB = { r.x + pad, r.y + topPad, textW, textH };
        drawTextWrapped(loc(pageKeys[0]), textB, 14.0f * s,
                        Color{210, 215, 235, 255});
        Rectangle imgB = { r.x + r.width - pad - imgW, r.y + topPad, imgW, textH };
        drawImgFit(pageImgs[0][0], imgB);
    } else {
        // Pages 2-5: text top ~50%, images bottom ~50%
        float contentH = r.height - topPad - navH;
        float textH    = contentH * 0.48f;
        float gap      = 6.0f * s;
        float imgH     = contentH - textH - gap;
        float contentW = r.width - 2.0f * pad;

        Rectangle textB = { r.x + pad, r.y + topPad, contentW, textH };
        drawTextWrapped(loc(pageKeys[introPage]), textB, 13.0f * s,
                        Color{210, 215, 235, 255});

        float imgY = r.y + topPad + textH + gap;
        if (pageImgs[introPage][1] >= 0) {
            float halfW = (contentW - gap) * 0.5f;
            Rectangle leftB  = { r.x + pad, imgY, halfW, imgH };
            Rectangle rightB = { r.x + pad + halfW + gap, imgY, halfW, imgH };
            drawImgFit(pageImgs[introPage][0], leftB);
            drawImgFit(pageImgs[introPage][1], rightB);
        } else {
            Rectangle imgB = { r.x + pad + contentW * 0.15f, imgY,
                               contentW * 0.7f, imgH };
            drawImgFit(pageImgs[introPage][0], imgB);
        }
    }

    // Page dots
    {
        float dotR   = 4.0f * s;
        float dotSpc = dotR * 2.0f + 6.0f * s;
        float totalW = dotSpc * (INTRO_PAGE_COUNT - 1);
        float startX = r.x + r.width * 0.5f - totalW * 0.5f;
        float dotsY  = introBtnNext.y + introBtnNext.height * 0.5f;

        for (int i = 0; i < INTRO_PAGE_COUNT; i++) {
            float cx = startX + i * dotSpc;
            Color c = (i == introPage) ? Color{120, 160, 255, 255}
                                       : Color{70, 75, 100, 180};
            DrawCircle((int)cx, (int)dotsY, dotR, c);
        }
    }

    // Next / Done
    {
        bool isLast = (introPage >= INTRO_PAGE_COUNT - 1);
        const char *txt = loc(isLast ? LKey::IntroDone : LKey::IntroNext);
        Color bg = Color{40, 60, 120, 240};
        DrawRectangleRounded(introBtnNext, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(introBtnNext, 0.3f, 8, 1.5f * s,
                                    Color{80, 110, 200, 220});
        float fs = 14.0f * s;
        int tw = measureText(txt, fs);
        drawText(txt, introBtnNext.x + introBtnNext.width * 0.5f - tw * 0.5f,
                 introBtnNext.y + introBtnNext.height * 0.5f - fs * 0.5f, fs, WHITE);
    }

    // Back
    if (introPage > 0) {
        const char *txt = loc(LKey::IntroBack);
        Color bg = Color{45, 45, 60, 240};
        DrawRectangleRounded(introBtnBack, 0.3f, 8, bg);
        DrawRectangleRoundedLinesEx(introBtnBack, 0.3f, 8, 1.5f * s,
                                    Color{80, 85, 120, 200});
        float fs = 14.0f * s;
        int tw = measureText(txt, fs);
        drawText(txt, introBtnBack.x + introBtnBack.width * 0.5f - tw * 0.5f,
                 introBtnBack.y + introBtnBack.height * 0.5f - fs * 0.5f, fs,
                 Color{200, 200, 220, 255});
    }
}

void UI::loadScenario(int idx, Simulation &sim) {
    sim.clear();
    resetCameraRequested = true;
    trackingCOM = false;
    trackBlend  = 0.0f;

    switch (idx) {
        case 0: {
            // Triple Problem — equilateral triangle, one vertex perturbed
            const float L = 10.0f;
            const float R = L / sqrtf(3.0f);
            const float v = sqrtf(G_CONST * 1.0f / L);

            sim.addStar({R + 0.01f, 0, 0},        {0, 0, v},              1.0f, 0.0f);
            sim.addStar({-R * 0.5f, 0,  R * 0.866025f},
                        {-v * 0.866025f, 0, -v * 0.5f},                  1.0f, 0.0f);
            sim.addStar({-R * 0.5f, 0, -R * 0.866025f},
                        { v * 0.866025f, 0, -v * 0.5f},                  1.0f, 0.0f);
            break;
        }
        case 1: {
            // Fantastic Four — hierarchical quadruple
            // Inner binary: 2×3 M☉ at ±1.5 (sep 3, tight)
            // Outer binary: 2×0.3 M☉ at 12.75 and 11.25 (local COM at 12, sep 1.5)
            sim.addStar({-0.2571f, +0.0000f, +0.0000f}, {+0.0000f, +0.0000f, +8.0470f}, 3.0f, 0.0f);
            sim.addStar({-1.4571f, +0.0000f, +0.0000f}, {+0.0000f, +0.0000f, -6.0952f}, 3.0f, 0.0f);
            sim.addStar({+5.4429f, +0.0000f, +0.0000f}, {+0.0000f, +0.0000f, -1.7729f}, 0.5f, 0.0f);
            sim.addStar({+4.8429f, +0.0000f, +0.0000f}, {+0.0000f, +0.0000f, -9.9379f}, 0.5f, 0.0f);
            break;
        }
        case 2: {
            // Freefall — 5×0.1 M☉ in eccentric pentagonal orbits around 10 M☉
            sim.addStar({0, 0, 0}, {0, 0, 0}, 10.0f, 0.0f);
            const float r = 20.0f;
            const float v = 2.45f;   // e ≈ 0.7
            sim.addStar({  r,      0,   0.0f   }, { 0.0f,     0,  v      }, 0.1f, 0.0f);
            sim.addStar({  6.18f,  0,  19.02f  }, {-2.330f,   0,  0.757f }, 0.1f, 0.0f);
            sim.addStar({-16.18f,  0,  11.76f  }, {-1.440f,   0, -1.982f }, 0.1f, 0.0f);
            sim.addStar({-16.18f,  0, -11.76f  }, { 1.440f,   0, -1.982f }, 0.1f, 0.0f);
            sim.addStar({  6.18f,  0, -19.02f  }, { 2.330f,   0,  0.757f }, 0.1f, 0.0f);
            break;
        }
        case 3: {
            // Infinity — Chenciner-Montgomery figure-eight

            sim.addStar({+5.8200f, +0.0000f, -1.4585f}, {+1.7023f, +0.0000f, +1.5788f}, 2.0f, 0.0f);
            sim.addStar({-5.8200f, +0.0000f, +1.4585f}, {+1.7023f, +0.0000f, +1.5788f}, 2.0f, 0.0f);
            sim.addStar({+0.0000f, +0.0000f, +0.0000f}, {-3.4047f, +0.0000f, -3.1576f}, 2.0f, 0.0f);
            break;
        }
        case 4: {
            // Recursion — nested hierarchy 25 / 5 / 1 / 0.5 / 0.1 M☉
            sim.addStar({  0, 0,   0}, {0, 0, -2.89f}, 25.0f, 0.0f);
            sim.addStar({  4, 0,   0}, {0, 0, 14.43f},  5.0f, 0.0f);
            sim.addStar({  0, 0,  10}, {-11.14f, 0, 0}, 1.0f, 0.0f);
            sim.addStar({-18, 0,   0}, {0, 0, -8.37f},  0.5f, 0.0f);
            sim.addStar({  0, 0, -25}, { 7.11f, 0, 0},  0.1f, 0.0f);
            break;
        }
        case 5: {
            // Wider binary + test particle offset perpendicular with
            // retrograde x-velocity → long-lived bouncing before escape
            sim.addStar({ 10.0f, 0, 0}, {0, 0,  4.999f}, 25.0f, 0.0f);
            sim.addStar({-10.0f, 0, 0}, {0, 0, -4.999f}, 25.0f, 0.0f);
            sim.addStar({  0.0f, 0, 1.5f}, {-1.2f, 0, 0}, 0.1f, 0.0f);
            break;
        }
    }
}

// ── Main update ────────────────────────────────────────────────

void UI::update(Simulation &sim, float &outMass) {
    introConsumedInput = false;

    int newW = GetScreenWidth();
    int newH = GetScreenHeight();
    if (newW != sw || newH != sh) {
        sw = newW;
        sh = newH;
        layout();
    }

    updateIntroDialog();

    zoomRequest = 0.0f;

    updateLanguageSelector();

    massSlider.update();

    if (timeSlider.update()) {
        sim.timeScale = timeSlider.value;
    }

    if (Pointer::down()) {
        Vector2 pos = Pointer::position();
        if (CheckCollisionPointRec(pos, btnZoomIn))  zoomRequest =  1.0f;
        if (CheckCollisionPointRec(pos, btnZoomOut)) zoomRequest = -1.0f;
    }
    for (int t = 0; t < Pointer::touchCount(); t++) {
        Vector2 tp = Pointer::touchPosition(t);
        if (CheckCollisionPointRec(tp, btnZoomIn))  zoomRequest =  1.0f;
        if (CheckCollisionPointRec(tp, btnZoomOut)) zoomRequest = -1.0f;
    }

    if (Pointer::pressed()) {
        Vector2 pos = Pointer::position();
        if (CheckCollisionPointRec(pos, btnPause)) {
            if (sim.timeScale > 0.001f) sim.timeScale = 0.0f;
            else sim.timeScale = timeSlider.value > 0.01f ? timeSlider.value : 1.0f;
        }
        if (CheckCollisionPointRec(pos, btnReset)) {
            sim.clear();
            resetCameraRequested = true;
        }
        if (CheckCollisionPointRec(pos, btnResetCamera)) {
            resetCameraRequested = true;
        }
        if (CheckCollisionPointRec(pos, btnInfo)) {
            showIntroDialog = true;
            introPage = 0;
        }
        for (int i = 0; i < SCENARIO_COUNT; i++) {
            if (CheckCollisionPointRec(pos, scenarioBtns[i])) {
                loadScenario(i, sim);
                break;
            }
        }
    }

    if (Pointer::pressed()) {
        Vector2 pos = Pointer::position();
        if (CheckCollisionPointRec(pos, btnQuit))
            quitRequested = true;
        for (int t = 0; t < Pointer::touchCount(); t++) {
            if (CheckCollisionPointRec(Pointer::touchPosition(t), btnQuit))
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

    {
        char buf[64];
        snprintf(buf, sizeof(buf), loc(LKey::StarsCount), sim.starCount, MAX_STARS);
        float counterY = hrRect.y + hrRect.height + 22.0f * s;
        drawText(buf, rightPanel.x + 15 * s, counterY, 13 * s, Color{100,105,130,255});
    }
    
    drawScenarios();

    drawZoomControls();

    timeSlider.draw(uiFont, 14.0f * s);
    drawTimeControls(sim);

    drawText(loc(LKey::TouchDragHint), 12 * s, 12 * s, 16 * s, Color{160,165,185,200});
    drawText(loc(LKey::ControlsHint),  12 * s, 32 * s, 13 * s, Color{100,105,130,180});

    // Info button
    {
        Vector2 pointer = Pointer::position();
        bool over = CheckCollisionPointRec(pointer, btnInfo) && Pointer::down();
        Color bg = over ? Color{50, 55, 85, 240} : Color{35, 38, 55, 220};
        DrawRectangleRounded(btnInfo, 1.0f, 8, bg);
        DrawRectangleRoundedLinesEx(btnInfo, 1.0f, 8, 1.5f * s,
                                    Color{80, 90, 150, 200});
        float ifs = 18.0f * s;
        const char *iText = "i";
        int itw = measureText(iText, ifs);
        drawText(iText, btnInfo.x + (btnInfo.width - itw) * 0.5f,
                 btnInfo.y + (btnInfo.height - ifs) * 0.5f + 1.0f * s,
                 ifs, Color{180, 190, 255, 240});
    }

    // Quit button
    {
        Vector2 pointer = Pointer::position();
        bool over = CheckCollisionPointRec(pointer, btnQuit) && Pointer::down();
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
        drawText(TextFormat("RawTC:%d  RawTP:%.0f,%.0f  Mouse:%.0f,%.0f",
                 GetTouchPointCount(),
                 GetTouchPosition(0).x, GetTouchPosition(0).y,
                 GetMousePosition().x, GetMousePosition().y),
                 cx, 62 * s, 11 * s, GREEN);
        drawText(TextFormat("CachedTC:%d  Ptr:%.0f,%.0f  Touch:%s  Down:%s",
                 Pointer::touchCount(),
                 Pointer::position().x, Pointer::position().y,
                 Pointer::_usingTouch ? "Y" : "N",
                 Pointer::down() ? "Y" : "N"),
                 cx, 76 * s, 11 * s, GREEN);
    }

    // Intro dialog drawn last (modal overlay)
    drawIntroDialog();
}

bool UI::isOverUI(Vector2 pos) const {
    if (introConsumedInput) return true;
    if (showIntroDialog && CheckCollisionPointRec(pos, introTooltipRect)) return true;
    if (CheckCollisionPointRec(pos, rightPanel))      return true;
    if (CheckCollisionPointRec(pos, bottomLeftPanel))  return true;
    if (CheckCollisionPointRec(pos, zoomPanel))        return true;
    if (CheckCollisionPointRec(pos, btnTrackCOM))      return true;
    if (CheckCollisionPointRec(pos, btnInfo))          return true;
    if (CheckCollisionPointRec(pos, btnResetCamera))   return true;
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        if (CheckCollisionPointRec(pos, flagRects[i])) return true;
    }
    return false;
}