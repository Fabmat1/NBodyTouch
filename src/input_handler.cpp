// src/input_handler.cpp

#include "input_handler.h"
#include "ui.h"
#include "raymath.h"
#include "compat.h"
#include <cmath>

// Returns true if the ray hits the y=0 plane at a reasonable distance
static bool screenToWorldPlane(Vector2 screenPos, Camera3D cam, Vector3 &outWorld) {
    Ray ray = GetScreenToWorldRay(screenPos, cam);
    if (fabsf(ray.direction.y) < 1e-6f) return false;
    float t = -ray.position.y / ray.direction.y;
    if (t < 0.0f) return false;

    outWorld = Vector3Add(ray.position, Vector3Scale(ray.direction, t));

    // Reject if the hit point is unreasonably far (> 500 units from origin)
    if (fabsf(outWorld.x) > 500.0f || fabsf(outWorld.z) > 500.0f) return false;

    return true;
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

static void orbitCamera(Renderer &renderer, Vector2 delta) {
    Vector3 offset = Vector3Subtract(renderer.camera.position, renderer.camera.target);

    Matrix rotY = MatrixRotateY(-delta.x * 0.005f);
    offset = Vector3Transform(offset, rotY);

    Vector3 right = Vector3CrossProduct(Vector3Normalize(offset), renderer.camera.up);
    Matrix rotP   = MatrixRotate(right, -delta.y * 0.005f);
    Vector3 newOffset = Vector3Transform(offset, rotP);

    // Prevent flipping past poles: reject if the new offset is nearly
    // parallel to the up vector
    float dot = fabsf(Vector3DotProduct(Vector3Normalize(newOffset), renderer.camera.up));
    if (dot < 0.98f) {
        offset = newOffset;
    }

    renderer.camera.position = Vector3Add(renderer.camera.target, offset);
}

void InputHandler::handlePinchZoomAndRotate(Renderer &renderer) {
    // Mouse wheel zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        renderer.zoomCamera(wheel * 3.0f);
    }

    if (GetTouchPointCount() == 2) {
        Vector2 t0 = GetTouchPosition(0);
        Vector2 t1 = GetTouchPosition(1);
        float dist = Vector2Distance(t0, t1);

        Vector2 center = { (t0.x + t1.x) * 0.5f, (t0.y + t1.y) * 0.5f };
        float angle = atan2f(t1.y - t0.y, t1.x - t0.x);

        if (prevPinchDist > 0.0f) {
            // Pinch zoom
            float zoomDelta = (dist - prevPinchDist) * 0.08f;
            renderer.zoomCamera(zoomDelta);

            // Two-finger rotation
            if (twoFingerRotating) {
                float angleDelta = angle - prevTwoFingerAngle;
                // Normalize angle delta to [-PI, PI]
                while (angleDelta > PI) angleDelta -= 2.0f * PI;
                while (angleDelta < -PI) angleDelta += 2.0f * PI;

                // Rotate around the target's vertical axis
                Vector3 offset = Vector3Subtract(renderer.camera.position, renderer.camera.target);
                Matrix rotY = MatrixRotateY(-angleDelta);
                offset = Vector3Transform(offset, rotY);
                renderer.camera.position = Vector3Add(renderer.camera.target, offset);
            }

            // Two-finger drag for pitch (vertical component of center movement)
            Vector2 centerDelta = { center.x - prevTwoFingerCenter.x,
                                    center.y - prevTwoFingerCenter.y };
            if (fabsf(centerDelta.y) > 1.0f) {
                orbitCamera(renderer, {0.0f, centerDelta.y});
            }
        }

        prevPinchDist = dist;
        prevTwoFingerAngle = angle;
        prevTwoFingerCenter = center;
        twoFingerRotating = true;
    } else {
        prevPinchDist = 0.0f;
        twoFingerRotating = false;
    }

    // Right-click drag: orbit
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        orbitCamera(renderer, delta);
    }
}

void InputHandler::handleMiddleMousePan(Renderer &renderer) {
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        panCamera(renderer, delta);
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

    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shiftHeld) return;

    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui.isOverUI(mouse)) {
        Vector3 worldPos;
        bool hitPlane = screenToWorldPlane(mouse, renderer.camera, worldPos);

        dragging = true;
        dragScreenStart = mouse;
        dragValid = hitPlane;

        if (hitPlane) {
            dragStart = worldPos;
            dragEnd   = dragStart;
        }
    }

    if (dragging && dragValid) {
        Vector3 worldPos;
        if (screenToWorldPlane(GetMousePosition(), renderer.camera, worldPos)) {
            dragEnd = worldPos;
        }
    }

    if (dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        dragging = false;
        if (dragValid) {
            Vector3 vel = Vector3Scale(Vector3Subtract(dragStart, dragEnd), 3.0f);
            sim.addStar(dragStart, vel, sliderMass, 0.0f);
        } else {
            // Show invalid spawn indicator
            invalidSpawnTimer = invalidSpawnDuration;
            invalidScreenPos  = dragScreenStart;
        }
    }
}

void InputHandler::update(Simulation &sim, Renderer &renderer, const UI &ui, float dt) {
    // Tick down invalid spawn indicator
    if (invalidSpawnTimer > 0.0f) {
        invalidSpawnTimer -= dt;
        if (invalidSpawnTimer < 0.0f) invalidSpawnTimer = 0.0f;
    }

    handleThreeFingerPan(renderer);
    handlePinchZoomAndRotate(renderer);
    handleMiddleMousePan(renderer);
    handleStarPlacement(sim, renderer, ui);

    // UI zoom buttons
    if (ui.zoomRequest != 0.0f) {
        renderer.zoomCamera(ui.zoomRequest * dt * 30.0f);
    }
}