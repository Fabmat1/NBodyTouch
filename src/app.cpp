#include "app.h"
#include "raylib.h"


void App::run() {
#ifdef PLATFORM_ANDROID
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "N-Body Dynamics");
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenW, screenH, "N-Body Dynamics — Astrophysics Explorer");
    // Pick up actual size (matters for HiDPI/4K)
    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
#endif

    SetTargetFPS(60);

    renderer.init(screenW, screenH);
    ui.init(screenW, screenH);
    ui.debugMode = debugMode;

    while (!WindowShouldClose() && !ui.quitRequested) {
        float dt = GetFrameTime();

        ui.update(sim, input.sliderMass);
        input.update(sim, renderer, ui, dt);
        sim.step(dt);

        Vector3 camDiff = Vector3Subtract(renderer.camera.target, renderer.camera.position);
        ui.cameraZoom = 44.72f / fmaxf(Vector3Length(camDiff), 0.1f);

        BeginDrawing();
            ClearBackground(BLACK);

            renderer.drawScene(sim);

            if (input.isDragging()) {
                BeginMode3D(renderer.camera);
                    Vector3 from = input.getDragStart();
                    Vector3 to   = input.getDragEnd();
                    Vector3 vel  = Vector3Scale(Vector3Subtract(from, to), 3.0f);
                    float   len  = Vector3Length(vel);
                    if (len > 0.1f) {
                        Vector3 tip = Vector3Add(from, Vector3Scale(
                            Vector3Normalize(vel), fminf(len, 15.0f)));
                        DrawLine3D(from, tip, YELLOW);
                        DrawSphere(tip, 0.15f, YELLOW);
                    }
                    DrawSphere(from, 0.3f, WHITE);
                EndMode3D();
            }

            // Invalid spawn indicator: red X that fades out
            if (input.showInvalidSpawn()) {
                float alpha = input.invalidSpawnAlpha();
                unsigned char a = (unsigned char)(alpha * 220);
                Color col = { 255, 60, 60, a };
                Vector2 p = input.invalidSpawnPos();
                float sz = 20.0f;

                DrawLineEx({p.x - sz, p.y - sz}, {p.x + sz, p.y + sz}, 5.0f, col);
                DrawLineEx({p.x + sz, p.y - sz}, {p.x - sz, p.y + sz}, 5.0f, col);

                // Tooltip text using UI font and locale
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