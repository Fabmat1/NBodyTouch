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

private:
    bool    dragging        = false;
    Vector3 dragStart       = {0};
    Vector3 dragEnd         = {0};
    Vector2 dragScreenStart = {0};
    float   prevPinchDist   = 0.0f;

    // Three-finger pan state
    bool    threePanning     = false;
    Vector2 threePanPrev     = {0};

    void handlePinchZoom(Renderer &renderer);
    void handleThreeFingerPan(Renderer &renderer);
    void handleStarPlacement(Simulation &sim, const Renderer &renderer, const UI &ui);
};