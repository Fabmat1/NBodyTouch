// include/input_handler.h

#pragma once
#include "simulation.h"
#include "renderer.h"

class UI;

class InputHandler {
public:
    float sliderMass = 1.0f;

    void update(Simulation &sim, Renderer &renderer, const UI &ui, float dt);

    bool    isDragging()   const { return dragging; }
    Vector3 getDragStart() const { return dragStart; }
    Vector3 getDragEnd()   const { return dragEnd; }

    // Visual feedback for invalid spawn
    bool    showInvalidSpawn() const { return invalidSpawnTimer > 0.0f; }
    Vector2 invalidSpawnPos()  const { return invalidScreenPos; }
    float   invalidSpawnAlpha() const { return invalidSpawnTimer / invalidSpawnDuration; }

private:
    bool    dragging        = false;
    bool    dragValid       = false;   // whether drag start hit the plane
    Vector3 dragStart       = {0};
    Vector3 dragEnd         = {0};
    Vector2 dragScreenStart = {0};
    float   prevPinchDist   = 0.0f;

    // Invalid spawn indicator
    float   invalidSpawnTimer    = 0.0f;
    float   invalidSpawnDuration = 2.0f;
    Vector2 invalidScreenPos     = {0};

    // Three-finger pan state
    bool    threePanning     = false;
    Vector2 threePanPrev     = {0};

    // Two-finger rotation state
    float   prevTwoFingerAngle = 0.0f;
    bool    twoFingerRotating  = false;
    Vector2 prevTwoFingerCenter = {0};

    void handlePinchZoomAndRotate(Renderer &renderer);
    void handleThreeFingerPan(Renderer &renderer);
    void handleMiddleMousePan(Renderer &renderer);
    void handleStarPlacement(Simulation &sim, const Renderer &renderer, const UI &ui);
};