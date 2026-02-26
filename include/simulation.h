#pragma once
#include "star.h"
#include <array>

class Simulation {
public:
    Star  stars[MAX_STARS];
    int   starCount = 0;
    float timeScale = 1.0f;   // 0 = paused
    float softening = 0.5f;   // avoid singularity

    int  addStar(Vector3 pos, Vector3 vel, float mass, float age);
    void step(float dt);       // advance by dt (already scaled)
    void clear();

private:
    // RK4 helpers — operate on flat state arrays
    struct State {
        Vector3 pos[MAX_STARS];
        Vector3 vel[MAX_STARS];
    };

    void computeAccelerations(const State &s, int n, Vector3 *accel) const;
    void rk4Step(float dt);
};