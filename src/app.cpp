#include "app.h"
#include "assets.h"
#include "pointer.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef PLATFORM_ANDROID
#include <sys/system_properties.h>

static bool IsAndroidEmulator() {
    char val[PROP_VALUE_MAX] = {};
    if (__system_property_get("ro.boot.qemu", val) > 0 && val[0] == '1') return true;
    if (__system_property_get("ro.kernel.qemu", val) > 0 && val[0] == '1') return true;
    if (__system_property_get("ro.hardware", val) > 0 &&
        (strcmp(val, "ranchu") == 0 || strcmp(val, "goldfish") == 0)) return true;
    return false;
}
#else
#include <csignal>
#include <unistd.h>

// raylib 5.5 ignores a failed platform init (no display, no GL context) and
// then crashes in rlglInit. Turn that into a stderr message and exit code 1.
static void OnInitWindowCrash(int) {
    static const char msg[] = "nbody_dynamics: failed to initialise the window/graphics\n";
    (void)!write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(1);
}
#endif

int App::run() {
    if (quietMode) SetTraceLogLevel(LOG_NONE);

#ifdef PLATFORM_ANDROID
    // The emulator's EGL has no 4x MSAA config; requesting one leaves the
    // surface uncreated and the screen black.
    if (!IsAndroidEmulator()) SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "N-Body Dynamics");
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    signal(SIGSEGV, OnInitWindowCrash);
    InitWindow(screenW, screenH, "N-Body Dynamics — Astrophysics Explorer");
    signal(SIGSEGV, SIG_DFL);
    if (!IsWindowReady()) {
        // No dialogs: the kiosk hub logs stderr and returns to its menu
        fprintf(stderr, "nbody_dynamics: failed to create a %dx%d window\n", screenW, screenH);
        return 1;
    }
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();

    // Taskbar/title-bar icon (X11 and SDL; Wayland takes it from the .desktop entry)
    if (FileExists(AssetPath("assets/icon.png"))) {
        Image icon = LoadImage(AssetPath("assets/icon.png"));
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
#endif

    if (targetFPS > 0) SetTargetFPS(targetFPS);

    renderer.init(screenW, screenH);
    ui.mobile = mobileUI;
    ui.init(screenW, screenH);
    ui.debugMode = debugMode;

    Vector3 trackStartOffset = Vector3Subtract(renderer.camera.position,
                                               renderer.camera.target);

    float inactivityTimer = 0.0f;

    while (!WindowShouldClose() && !ui.quitRequested) {
        float dt = GetFrameTime();
        Pointer::beginFrame();

        // ── Inactivity detection ──────────────────────────────────────────
        {
            bool anyActivity = Pointer::down() ||
                               Pointer::pressed() ||
                               Pointer::released() ||
                               Pointer::touchCount() > 0 ||
                               IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
                               IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                               GetKeyPressed() != 0 ||
                               GetMouseWheelMove() != 0.0f;
            Vector2 md = GetMouseDelta();
            if (fabsf(md.x) > 0.5f || fabsf(md.y) > 0.5f)
                anyActivity = true;

            if (anyActivity) inactivityTimer = 0.0f;
            else             inactivityTimer += dt;

            if (inactivityTimeout > 0.0f && inactivityTimer >= inactivityTimeout)
                break;
        }

        // ── Compute centre of mass (for input handler) ───────────────────
        Vector3 com       = {0, 0, 0};
        float   totalMass = 0.0f;
        for (int i = 0; i < sim.starCount; i++) {
            if (!sim.stars[i].active) continue;
            com = Vector3Add(com, Vector3Scale(sim.stars[i].pos, sim.stars[i].mass));
            totalMass += sim.stars[i].mass;
        }
        if (totalMass > 0.0f)
            com = Vector3Scale(com, 1.0f / totalMass);

        input.currentCOM        = com;
        input.comTrackingActive = ui.trackingCOM;

        // ── Update ────────────────────────────────────────────────────────
        ui.update(sim, input.sliderMass);
        input.update(sim, renderer, ui, dt);
        sim.step(dt);

        if (IsKeyPressed(KEY_SPACE))
            sim.timeScale = (sim.timeScale > 0.001f) ? 0.0f : 1.0f;
        if (IsKeyPressed(KEY_C)) {
            sim.clear();
            ui.resetCameraRequested = true;
        }

        // ── Handle camera reset request ───────────────────────────────────
        if (ui.resetCameraRequested) {
            renderer.resetCamera();
            ui.trackingCOM          = false;
            ui.trackBlend           = 0.0f;
            ui.resetCameraRequested = false;
        }

        // ── Centre-of-mass tracking toggle ────────────────────────────────
        {
            if (ui.trackToggleRequested) {
                ui.trackToggleRequested = false;
                ui.trackingCOM = !ui.trackingCOM;
                if (ui.trackingCOM) {
                    trackStartOffset = Vector3Subtract(renderer.camera.position,
                                                       renderer.camera.target);
                    ui.trackBlend = 0.0f;
                }
            }
        }

        if (ui.trackingCOM && input.didManualPan()) {
            ui.trackingCOM = false;
            ui.trackBlend  = 0.0f;
        }

        // ── Recompute COM after sim step for camera tracking ──────────────
        com       = {0, 0, 0};
        totalMass = 0.0f;
        for (int i = 0; i < sim.starCount; i++) {
            if (!sim.stars[i].active) continue;
            com = Vector3Add(com, Vector3Scale(sim.stars[i].pos, sim.stars[i].mass));
            totalMass += sim.stars[i].mass;
        }
        if (totalMass > 0.0f)
            com = Vector3Scale(com, 1.0f / totalMass);

        if (ui.trackingCOM && sim.starCount > 0) {
            ui.trackBlend += dt / 0.5f;
            if (ui.trackBlend > 1.0f) ui.trackBlend = 1.0f;

            float t = ui.trackBlend;
            t = t * t * (3.0f - 2.0f * t);

            // Preserve current camera offset (allows orbit/zoom to still work)
            Vector3 offset = Vector3Subtract(renderer.camera.position,
                                             renderer.camera.target);

            // Exponential catch-up with shrinking half-life:
            //   t=0 → halfLife=0.3s (gentle), t=1 → near-instant tracking
            float halfLife = 0.3f * (1.0f - t) + 0.001f;
            float alpha = 1.0f - powf(0.5f, dt / halfLife);

            renderer.camera.target   = Vector3Lerp(renderer.camera.target, com, alpha);
            renderer.camera.position = Vector3Add(renderer.camera.target, offset);
        } else if (!ui.trackingCOM) {
            ui.trackBlend = 0.0f;
        }

        // ── Camera zoom readout ───────────────────────────────────────────
        Vector3 camDiff = Vector3Subtract(renderer.camera.target, renderer.camera.position);
        ui.cameraZoom = 44.72f / fmaxf(Vector3Length(camDiff), 0.1f);

        // ── Draw ──────────────────────────────────────────────────────────
        BeginDrawing();
            ClearBackground(BLACK);

            renderer.drawScene(sim);

            if (input.isDragging()) {
                Vector3 from = input.getDragStart();
                Vector3 to   = input.getDragEnd();
                Vector3 vel  = Vector3Scale(Vector3Subtract(from, to), 3.0f);
                float   len  = Vector3Length(vel);

                if (len > 0.1f) {
                    BeginMode3D(renderer.camera);
                        Vector3 tip = Vector3Add(from, Vector3Scale(
                            Vector3Normalize(vel), fminf(len, 15.0f)));
                        DrawLine3D(from, tip, YELLOW);
                        DrawSphere(tip, 0.15f, YELLOW);
                        DrawSphere(from, 0.3f, WHITE);
                    EndMode3D();
                }
            }

            if (input.showInvalidSpawn()) {
                float alpha = input.invalidSpawnAlpha();
                unsigned char a = (unsigned char)(alpha * 220);
                Color col = { 255, 60, 60, a };
                Vector2 p = input.invalidSpawnPos();
                float sz = 20.0f * ui.uiScale();

                DrawLineEx({p.x - sz, p.y - sz}, {p.x + sz, p.y + sz}, 5.0f, col);
                DrawLineEx({p.x + sz, p.y - sz}, {p.x - sz, p.y + sz}, 5.0f, col);

                Color textCol = { 255, 180, 180, a };
                const char *msg = ui.loc(LKey::OutsideSpawnArea);
                float fs = 16.0f * ui.uiScale();
                int tw = ui.measureText(msg, fs);
                ui.drawText(msg, p.x - tw * 0.5f, p.y + sz * 1.5f, fs, textCol);
            }

            ui.draw(sim, input.sliderMass);

        EndDrawing();
    }

    ui.shutdown();
    renderer.shutdown();
    CloseWindow();
    return 0;
}