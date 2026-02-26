#include "input_handler.h"
#include "ui.h"
#include "raymath.h"
#include "compat.h"

static Vector3 screenToWorldPlane(Vector2 screenPos, Camera3D cam) {
    Ray ray = GetScreenToWorldRay(screenPos, cam);
    if (fabsf(ray.direction.y) < 1e-6f) return {0, 0, 0};
    float t = -ray.position.y / ray.direction.y;
    if (t < 0) t = 0;
    return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
}

static void panCamera(Renderer &renderer, Vector2 screenDelta) {
    Vector3 camFwd    = Vector3Subtract(renderer.camera.target, renderer.camera.position);
    Vector3 camRight  = Vector3Normalize(Vector3CrossProduct(camFwd, renderer.camera.up));
    Vector3 camFwdXZ  = Vector3Normalize({camFwd.x, 0.0f, camFwd.z});
    Vector3 camRightXZ = Vector3Normalize({camRight.x, 0.0f, camRight.z});

    float camDist = Vector3Length(camFwd);
    float scale   = camDist * 0.002f;

    Vector3 worldDelta = Vector3Add(
        Vector3Scale(camRightXZ, -screenDelta.x * scale),
        Vector3Scale(camFwdXZ,    screenDelta.y * scale)
    );

    renderer.camera.position = Vector3Add(renderer.camera.position, worldDelta);
    renderer.camera.target   = Vector3Add(renderer.camera.target, worldDelta);
}

void InputHandler::handlePinchZoom(Renderer &renderer) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector3 dir = Vector3Subtract(renderer.camera.target, renderer.camera.position);
        dir = Vector3Scale(Vector3Normalize(dir), wheel * 3.0f);
        renderer.camera.position = Vector3Add(renderer.camera.position, dir);
    }

    if (GetTouchPointCount() == 2) {
        Vector2 t0 = GetTouchPosition(0);
        Vector2 t1 = GetTouchPosition(1);
        float dist = Vector2Distance(t0, t1);

        if (prevPinchDist > 0.0f) {
            float delta = (dist - prevPinchDist) * 0.08f;
            Vector3 dir = Vector3Subtract(renderer.camera.target, renderer.camera.position);
            dir = Vector3Scale(Vector3Normalize(dir), delta);
            renderer.camera.position = Vector3Add(renderer.camera.position, dir);
        }
        prevPinchDist = dist;
    } else {
        prevPinchDist = 0.0f;
    }

    // Right/middle drag orbit
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        Vector3 offset = Vector3Subtract(renderer.camera.position, renderer.camera.target);

        Matrix rotY = MatrixRotateY(-delta.x * 0.005f);
        offset = Vector3Transform(offset, rotY);

        Vector3 right = Vector3CrossProduct(Vector3Normalize(offset), renderer.camera.up);
        Matrix rotP   = MatrixRotate(right, -delta.y * 0.005f);
        offset = Vector3Transform(offset, rotP);

        renderer.camera.position = Vector3Add(renderer.camera.target, offset);
    }
}

void InputHandler::handleThreeFingerPan(Renderer &renderer) {
    // Real 3-finger touch
    if (GetTouchPointCount() == 3) {
        Vector2 c = {0, 0};
        for (int i = 0; i < 3; i++) {
            Vector2 tp = GetTouchPosition(i);
            c.x += tp.x;
            c.y += tp.y;
        }
        c.x /= 3.0f;
        c.y /= 3.0f;

        if (threePanning) {
            Vector2 screenDelta = { c.x - threePanPrev.x, c.y - threePanPrev.y };
            panCamera(renderer, screenDelta);
        }

        threePanPrev = c;
        threePanning = true;
        return;
    }

    // Keyboard emulation: Shift + left-drag
    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shiftHeld && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (threePanning) {
            Vector2 screenDelta = GetMouseDelta();
            panCamera(renderer, screenDelta);
        }
        threePanning = true;
        return;
    }

    threePanning = false;
}

void InputHandler::handleStarPlacement(Simulation &sim, const Renderer &renderer,
                                        const UI &ui) {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) return;
    if (GetTouchPointCount() >= 2) return;

    // Block star placement while shift-panning
    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shiftHeld) return;

    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui.isOverUI(mouse)) {
        dragging = true;
        dragScreenStart = mouse;
        dragStart = screenToWorldPlane(mouse, renderer.camera);
        dragEnd   = dragStart;
    }

    if (dragging) {
        dragEnd = screenToWorldPlane(GetMousePosition(), renderer.camera);
    }

    if (dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        dragging = false;
        Vector3 vel = Vector3Scale(Vector3Subtract(dragStart, dragEnd), 3.0f);
        sim.addStar(dragStart, vel, sliderMass, 0.0f);
    }
}

void InputHandler::update(Simulation &sim, Renderer &renderer, const UI &ui, float dt) {
    handleThreeFingerPan(renderer);
    handlePinchZoom(renderer);
    handleStarPlacement(sim, renderer, ui);
}