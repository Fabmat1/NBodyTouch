#include "input_handler.h"
#include "ui.h"
#include "pointer.h"
#include "raymath.h"
#include "compat.h"
#include <cmath>
#include <algorithm>

static bool screenToWorldPlane(Vector2 screenPos, Camera3D cam, Vector3 &outWorld) {
    Ray ray = GetScreenToWorldRay(screenPos, cam);
    if (fabsf(ray.direction.y) < 1e-6f) return false;
    float t = -ray.position.y / ray.direction.y;
    if (t < 0.0f) return false;

    outWorld = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    if (fabsf(outWorld.x) > 100000.0f || fabsf(outWorld.z) > 100000.0f) return false;
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

    float dot = fabsf(Vector3DotProduct(Vector3Normalize(newOffset), renderer.camera.up));
    if (dot < 0.98f) offset = newOffset;

    renderer.camera.position = Vector3Add(renderer.camera.target, offset);
}

void InputHandler::handlePinchZoomAndRotate(Renderer &renderer) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        renderer.zoomCamera(wheel * 3.0f);
    }

    if (Pointer::touchCount() == 2) {
        Vector2 t0 = Pointer::touchPosition(0);
        Vector2 t1 = Pointer::touchPosition(1);
        float dist = Vector2Distance(t0, t1);

        Vector2 center = { (t0.x + t1.x) * 0.5f, (t0.y + t1.y) * 0.5f };
        float angle = atan2f(t1.y - t0.y, t1.x - t0.x);

        // Only compute deltas when fingers actually moved.
        // Stale/cached frames produce identical positions → freshData false
        // → no phantom zoom/rotate from contamination or float noise.
        bool live = Pointer::freshData();

        if (prevPinchDist > 0.0f && live) {
            // ── pinch zoom ──
            float zoomDelta = (dist - prevPinchDist) * 0.08f;
            zoomDelta = std::clamp(zoomDelta, -5.0f, 5.0f);
            renderer.zoomCamera(zoomDelta);

            // ── two-finger rotation ──
            if (twoFingerRotating) {
                float angleDelta = angle - prevTwoFingerAngle;
                while (angleDelta >  PI) angleDelta -= 2.0f * PI;
                while (angleDelta < -PI) angleDelta += 2.0f * PI;

                if (fabsf(angleDelta) < 0.5f) {
                    Vector3 offset = Vector3Subtract(renderer.camera.position,
                                                     renderer.camera.target);
                    // +angleDelta: CW fingers → CCW camera orbit → scene rotates CW
                    Matrix rotYM = MatrixRotateY(angleDelta);
                    offset = Vector3Transform(offset, rotYM);
                    renderer.camera.position = Vector3Add(renderer.camera.target, offset);
                }
            }

            // ── two-finger vertical drag → pitch ──
            Vector2 centerDelta = { center.x - prevTwoFingerCenter.x,
                                    center.y - prevTwoFingerCenter.y };
            float cdLen = Vector2Length(centerDelta);
            if (cdLen > 1.0f && cdLen < 100.0f) {
                orbitCamera(renderer, {0.0f, centerDelta.y});
            }
        }

        // Only store reference values from real movement —
        // prevents stale frames from corrupting the baseline.
        if (live) {
            prevPinchDist       = dist;
            prevTwoFingerAngle  = angle;
            prevTwoFingerCenter = center;
        }
        twoFingerRotating = true;
    } else {
        prevPinchDist     = 0.0f;
        twoFingerRotating = false;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        orbitCamera(renderer, delta);
    }
}

void InputHandler::handleMiddleMousePan(Renderer &renderer) {
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        panCamera(renderer, delta);
        manualPanThisFrame = true;
    }
}

void InputHandler::handleThreeFingerPan(Renderer &renderer) {
    if (Pointer::touchCount() == 3) {
        Vector2 c = {0, 0};
        for (int i = 0; i < 3; i++) {
            Vector2 tp = Pointer::touchPosition(i);
            c.x += tp.x;
            c.y += tp.y;
        }
        c.x /= 3.0f;
        c.y /= 3.0f;

        if (threePanning && Pointer::freshData()) {
            Vector2 screenDelta = { c.x - threePanPrev.x, c.y - threePanPrev.y };
            if (Vector2Length(screenDelta) < 100.0f) {
                panCamera(renderer, screenDelta);
                manualPanThisFrame = true;
            }
        }

        if (Pointer::freshData()) {
            threePanPrev = c;
        }
        threePanning = true;
        return;
    }

    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shiftHeld && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (threePanning) {
            Vector2 screenDelta = GetMouseDelta();
            panCamera(renderer, screenDelta);
            manualPanThisFrame = true;
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

    if (Pointer::touchCount() >= 2) {
        dragging = false;
        return;
    }

    if (multiTouchCooldown > 0.0f) return;

    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shiftHeld) return;

    Vector2 pointer = Pointer::position();

    if (Pointer::pressed() && !ui.isOverUI(pointer)) {
        Vector3 worldPos;
        bool hitPlane = screenToWorldPlane(pointer, renderer.camera, worldPos);

        dragging        = true;
        dragScreenStart = pointer;
        dragValid       = hitPlane;

        if (hitPlane) {
            dragStart = worldPos;
            dragEnd   = dragStart;
        }
    }

    if (dragging && Pointer::released()) {
        dragging = false;
        if (dragValid) {
            Vector3 vel = Vector3Scale(Vector3Subtract(dragStart, dragEnd), 3.0f);
            sim.addStar(dragStart, vel, sliderMass, 0.0f);
        } else {
            invalidSpawnTimer = invalidSpawnDuration;
            invalidScreenPos  = dragScreenStart;
        }
        return;
    }

    if (dragging && dragValid && Pointer::down()) {
        Vector3 worldPos;
        if (screenToWorldPlane(Pointer::position(), renderer.camera, worldPos)) {
            dragEnd = worldPos;
        }
    }

    if (dragging && !Pointer::down()) {
        dragging = false;
    }
}

void InputHandler::update(Simulation &sim, Renderer &renderer, const UI &ui, float dt) {
    manualPanThisFrame = false;   // ← reset at top of frame

    if (invalidSpawnTimer > 0.0f) {
        invalidSpawnTimer -= dt;
        if (invalidSpawnTimer < 0.0f) invalidSpawnTimer = 0.0f;
    }

    if (Pointer::touchCount() >= 2)
        multiTouchCooldown = 0.3f;
    if (multiTouchCooldown > 0.0f) {
        multiTouchCooldown -= dt;
        if (multiTouchCooldown < 0.0f) multiTouchCooldown = 0.0f;
    }

    handleThreeFingerPan(renderer);
    handlePinchZoomAndRotate(renderer);
    handleMiddleMousePan(renderer);
    handleStarPlacement(sim, renderer, ui);

    if (ui.zoomRequest != 0.0f) {
        renderer.zoomCamera(ui.zoomRequest * dt * 30.0f);
    }
}