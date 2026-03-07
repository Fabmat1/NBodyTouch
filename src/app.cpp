#include "app.h"
#include "pointer.h"
#include "raylib.h"
#include "raymath.h"

void App::run() {
#ifdef PLATFORM_ANDROID
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "N-Body Dynamics");
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenW, screenH, "N-Body Dynamics — Astrophysics Explorer");
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
#endif

    SetTargetFPS(60);

    renderer.init(screenW, screenH);
    ui.init(screenW, screenH);
    ui.debugMode = debugMode;

    // Offset from target when tracking was activated (for smooth blend-in)
    Vector3 trackStartOffset = Vector3Subtract(renderer.camera.position,
                                               renderer.camera.target);

    while (!WindowShouldClose() && !ui.quitRequested) {
        float dt = GetFrameTime();
        Pointer::beginFrame();

        ui.update(sim, input.sliderMass);
        input.update(sim, renderer, ui, dt);
        sim.step(dt);

        // ── Centre-of-mass tracking ───────────────────────────────────────

        // Toggle on button press
        {
            bool btnHit = false;
            if (Pointer::pressed()) {
                Vector2 pos = Pointer::position();
                if (CheckCollisionPointRec(pos, ui.btnTrackCOM)) btnHit = true;
            }
            if (!btnHit) {
                for (int t = 0; t < Pointer::touchCount(); t++) {
                    if (Pointer::pressed() &&
                        CheckCollisionPointRec(Pointer::touchPosition(t), ui.btnTrackCOM))
                        btnHit = true;
                }
            }
            if (btnHit) {
                ui.trackingCOM = !ui.trackingCOM;
                if (ui.trackingCOM) {
                    trackStartOffset = Vector3Subtract(renderer.camera.position,
                                                       renderer.camera.target);
                    ui.trackBlend = 0.0f;
                }
            }
        }

        // Auto-deactivate if the user manually panned
        if (ui.trackingCOM && input.didManualPan()) {
            ui.trackingCOM = false;
            ui.trackBlend  = 0.0f;
        }

        // Compute centre of mass
        Vector3 com       = {0, 0, 0};
        float   totalMass = 0.0f;
        for (int i = 0; i < sim.starCount; i++) {
            if (!sim.stars[i].active) continue;
            com = Vector3Add(com, Vector3Scale(sim.stars[i].pos, sim.stars[i].mass));
            totalMass += sim.stars[i].mass;
        }
        if (totalMass > 0.0f)
            com = Vector3Scale(com, 1.0f / totalMass);

        if (ui.trackingCOM && sim.starCount > 0) {
            // Advance blend 0→1 over ~0.5 s
            ui.trackBlend += dt / 0.5f;
            if (ui.trackBlend > 1.0f) ui.trackBlend = 1.0f;

            // Ease-in-out cubic
            float blend = ui.trackBlend;
            blend = blend * blend * (3.0f - 2.0f * blend);

            if (ui.trackBlend >= 1.0f) {
                // Fully tracking: keep camera offset, move target to COM
                Vector3 offset = Vector3Subtract(renderer.camera.position,
                                                 renderer.camera.target);
                renderer.camera.target   = com;
                renderer.camera.position = Vector3Add(com, offset);
            } else {
                // Smoothly interpolate target toward COM
                // and position toward (COM + startOffset)
                Vector3 desiredTarget   = com;
                Vector3 desiredPosition = Vector3Add(com, trackStartOffset);
                float   lerpSpeed       = blend * 6.0f * dt;
                renderer.camera.target   = Vector3Lerp(renderer.camera.target,
                                                        desiredTarget,   lerpSpeed);
                renderer.camera.position = Vector3Lerp(renderer.camera.position,
                                                        desiredPosition, lerpSpeed);
            }
        } else if (!ui.trackingCOM) {
            ui.trackBlend = 0.0f;
        }

        // ── Camera zoom readout ───────────────────────────────────────────
        Vector3 camDiff = Vector3Subtract(renderer.camera.target, renderer.camera.position);
        ui.cameraZoom = 44.72f / fmaxf(Vector3Length(camDiff), 0.1f);

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
                float sz = 20.0f;

                DrawLineEx({p.x - sz, p.y - sz}, {p.x + sz, p.y + sz}, 5.0f, col);
                DrawLineEx({p.x + sz, p.y - sz}, {p.x - sz, p.y + sz}, 5.0f, col);

                Color textCol = { 255, 180, 180, a };
                const char *msg = ui.loc(LKey::OutsideSpawnArea);
                float fs = 16.0f;
                int tw = ui.measureText(msg, fs);
                ui.drawText(msg, p.x - tw * 0.5f, p.y + sz * 1.5f, fs, textCol);
            }

            ui.draw(sim, input.sliderMass);

            if (IsKeyPressed(KEY_SPACE))
                sim.timeScale = (sim.timeScale > 0.001f) ? 0.0f : 1.0f;
            if (IsKeyPressed(KEY_C)) sim.clear();

        EndDrawing();
    }

    ui.shutdown();
    renderer.shutdown();
    CloseWindow();
}