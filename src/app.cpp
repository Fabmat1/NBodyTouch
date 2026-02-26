#include "app.h"
#include "raylib.h"

void App::run() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenW, screenH, "N-Body Dynamics — Astrophysics Explorer");
    SetTargetFPS(60);

    renderer.init(screenW, screenH);
    ui.init(screenW, screenH);
    ui.debugMode = debugMode;

    while (!WindowShouldClose() && !ui.quitRequested) {
        float dt = GetFrameTime();

        ui.update(sim, input.sliderMass);
        input.update(sim, renderer, ui, dt);
        sim.step(dt);

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