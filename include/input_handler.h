#pragma once
#include "simulation.h"
#include "renderer.h"

class UI;

class InputHandler {
public:
    float sliderMass = 1.0f;

    void update(Simulation &sim, Renderer &renderer, const UI &ui, float dt);

    bool    isDragging()   const { return dragging && multiTouchCooldown <= 0.0f; }
    Vector3 getDragStart() const { return dragStart; }
    Vector3 getDragEnd()   const { return dragEnd; }

    bool    showInvalidSpawn() const { return invalidSpawnTimer > 0.0f; }
    Vector2 invalidSpawnPos()  const { return invalidScreenPos; }
    float   invalidSpawnAlpha() const { return invalidSpawnTimer / invalidSpawnDuration; }
    
    bool didManualPan() const { return manualPanThisFrame; }

private:
    bool    dragging        = false;
    bool    dragValid       = false;
    Vector3 dragStart       = {0};
    Vector3 dragEnd         = {0};
    Vector2 dragScreenStart = {0};
    float   prevPinchDist   = 0.0f;

    float   invalidSpawnTimer    = 0.0f;
    float   invalidSpawnDuration = 2.0f;
    Vector2 invalidScreenPos     = {0};

    bool    threePanning     = false;
    Vector2 threePanPrev     = {0};

    float   prevTwoFingerAngle  = 0.0f;
    bool    twoFingerRotating   = false;
    Vector2 prevTwoFingerCenter = {0};

    float   multiTouchCooldown  = 0.0f;
    bool manualPanThisFrame = false;

    void handlePinchZoomAndRotate(Renderer &renderer);
    void handleThreeFingerPan(Renderer &renderer);
    void handleMiddleMousePan(Renderer &renderer);
    void handleStarPlacement(Simulation &sim, const Renderer &renderer, const UI &ui);
};