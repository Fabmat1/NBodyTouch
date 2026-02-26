#pragma once
#include "simulation.h"
#include "raylib.h"

class Renderer {
public:
    Camera3D camera;

    void init(int screenW, int screenH);
    void drawScene(const Simulation &sim);
    void shutdown();

    bool skyboxLoaded = false;

private:
    Model   skybox;
    Shader  skyboxShader;
    unsigned int cubemapTexture;

    void loadSkybox();
    void drawSkybox() const;
    void drawStar(const Star &s) const;
    void drawTrail(const Star &s) const;
    void drawVelocityArrow(Vector3 from, Vector3 to) const;

    int screenW, screenH;

    friend class InputHandler;
};