// include/simulation.h
#pragma once
#include "star.h"
#include <array>

class Simulation {
public:
    Star  stars[MAX_STARS];
    int   starCount = 0;
    float timeScale = 1.0f;
    float softening = 1.5f;

    int  addStar(Vector3 pos, Vector3 vel, float mass, float age);
    void step(float dt);
    void clear();

private:
    struct State {
        Vector3 pos[MAX_STARS];
        Vector3 vel[MAX_STARS];
    };

    void  computeAccelerations(const State &s, int n, Vector3 *accel) const;
    float maxAcceleration(const State &s, int n) const;
    void  rk4Step(float dt);
};