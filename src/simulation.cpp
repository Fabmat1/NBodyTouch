// src/simulation.cpp

#include "simulation.h"
#include <cmath>

int Simulation::addStar(Vector3 pos, Vector3 vel, float mass, float age) {
    if (starCount >= MAX_STARS) return -1;
    int i = starCount++;
    stars[i] = Star{};
    stars[i].pos    = pos;
    stars[i].vel    = vel;
    stars[i].mass   = mass;
    stars[i].age    = age;
    stars[i].active = true;
    stars[i].updateStellarProperties();
    return i;
}

void Simulation::clear() {
    for (int i = 0; i < starCount; i++) {
        stars[i].active = false;
        stars[i].trail.clear();
    }
    starCount = 0;
}

void Simulation::computeAccelerations(const State &s, int n, Vector3 *accel) const {
    for (int i = 0; i < n; i++) accel[i] = {0, 0, 0};

    for (int i = 0; i < n; i++) {
        if (!stars[i].active) continue;
        for (int j = i + 1; j < n; j++) {
            if (!stars[j].active) continue;

            Vector3 diff = Vector3Subtract(s.pos[j], s.pos[i]);
            float dist2  = Vector3DotProduct(diff, diff) + softening * softening;
            float dist   = sqrtf(dist2);
            float inv_dist3 = 1.0f / (dist2 * dist);

            Vector3 fi = Vector3Scale(diff, G_CONST * stars[j].mass * inv_dist3);
            Vector3 fj = Vector3Scale(diff, G_CONST * stars[i].mass * inv_dist3);

            accel[i] = Vector3Add(accel[i], fi);
            accel[j] = Vector3Subtract(accel[j], fj);
        }
    }
}

// Returns the maximum acceleration magnitude across all active stars
float Simulation::maxAcceleration(const State &s, int n) const {
    Vector3 accel[MAX_STARS];
    computeAccelerations(s, n, accel);
    float maxA2 = 0.0f;
    for (int i = 0; i < n; i++) {
        if (!stars[i].active) continue;
        float a2 = Vector3DotProduct(accel[i], accel[i]);
        if (a2 > maxA2) maxA2 = a2;
    }
    return sqrtf(maxA2);
}

void Simulation::rk4Step(float dt) {
    int n = starCount;
    if (n == 0) return;

    State s0, k1p, k1v, k2p, k2v, k3p, k3v, k4p, k4v;
    State tmp;
    Vector3 accel[MAX_STARS];

    // Current state
    for (int i = 0; i < n; i++) {
        s0.pos[i] = stars[i].pos;
        s0.vel[i] = stars[i].vel;
    }

    // k1
    computeAccelerations(s0, n, accel);
    for (int i = 0; i < n; i++) {
        k1p.pos[i] = stars[i].vel;
        k1v.vel[i] = accel[i];
    }

    // k2
    for (int i = 0; i < n; i++) {
        tmp.pos[i] = Vector3Add(s0.pos[i], Vector3Scale(k1p.pos[i], dt * 0.5f));
        tmp.vel[i] = Vector3Add(s0.vel[i], Vector3Scale(k1v.vel[i], dt * 0.5f));
    }
    computeAccelerations(tmp, n, accel);
    for (int i = 0; i < n; i++) {
        k2p.pos[i] = tmp.vel[i];
        k2v.vel[i] = accel[i];
    }

    // k3
    for (int i = 0; i < n; i++) {
        tmp.pos[i] = Vector3Add(s0.pos[i], Vector3Scale(k2p.pos[i], dt * 0.5f));
        tmp.vel[i] = Vector3Add(s0.vel[i], Vector3Scale(k2v.vel[i], dt * 0.5f));
    }
    computeAccelerations(tmp, n, accel);
    for (int i = 0; i < n; i++) {
        k3p.pos[i] = tmp.vel[i];
        k3v.vel[i] = accel[i];
    }

    // k4
    for (int i = 0; i < n; i++) {
        tmp.pos[i] = Vector3Add(s0.pos[i], Vector3Scale(k3p.pos[i], dt));
        tmp.vel[i] = Vector3Add(s0.vel[i], Vector3Scale(k3v.vel[i], dt));
    }
    computeAccelerations(tmp, n, accel);
    for (int i = 0; i < n; i++) {
        k4p.pos[i] = tmp.vel[i];
        k4v.vel[i] = accel[i];
    }

    // Combine
    for (int i = 0; i < n; i++) {
        if (!stars[i].active) continue;
        Vector3 dp = Vector3Scale(
            Vector3Add(Vector3Add(k1p.pos[i], Vector3Scale(k2p.pos[i], 2.0f)),
                       Vector3Add(Vector3Scale(k3p.pos[i], 2.0f), k4p.pos[i])),
            dt / 6.0f);
        Vector3 dv = Vector3Scale(
            Vector3Add(Vector3Add(k1v.vel[i], Vector3Scale(k2v.vel[i], 2.0f)),
                       Vector3Add(Vector3Scale(k3v.vel[i], 2.0f), k4v.vel[i])),
            dt / 6.0f);

        stars[i].pos = Vector3Add(stars[i].pos, dp);
        stars[i].vel = Vector3Add(stars[i].vel, dv);
    }
}

void Simulation::step(float dt) {
    float simDt = dt * timeScale;
    if (simDt <= 0.0f) return;

    int n = starCount;

    // Compute adaptive timestep from current accelerations.
    // We want: dt <= eta * sqrt(softening / a_max)
    // This ensures that no star moves an unphysical distance in one step.
    constexpr float MAX_STEP  = 0.01f;   // absolute upper bound
    constexpr float MIN_STEP  = 1e-5f;   // prevent infinite loops
    constexpr float ETA       = 0.2f;    // Courant-like safety factor

    float h = MAX_STEP;
    if (n > 0) {
        State s0;
        for (int i = 0; i < n; i++) {
            s0.pos[i] = stars[i].pos;
            s0.vel[i] = stars[i].vel;
        }
        float aMax = maxAcceleration(s0, n);
        if (aMax > 1e-6f) {
            // Dynamical timescale criterion: h ~ eta * sqrt(eps / a_max)
            float hDyn = ETA * sqrtf(softening / aMax);
            h = fminf(h, hDyn);
        }
        h = fmaxf(h, MIN_STEP);
    }

    float remaining = simDt;
    while (remaining > 0.0f) {
        float step = fminf(remaining, h);
        rk4Step(step);
        remaining -= step;

        // Re-evaluate adaptive step after each sub-step since
        // stars may have moved closer or farther apart.
        if (remaining > 0.0f && n > 0) {
            State s0;
            for (int i = 0; i < n; i++) {
                s0.pos[i] = stars[i].pos;
                s0.vel[i] = stars[i].vel;
            }
            float aMax = maxAcceleration(s0, n);
            if (aMax > 1e-6f) {
                float hDyn = ETA * sqrtf(softening / aMax);
                h = fmaxf(fminf(hDyn, MAX_STEP), MIN_STEP);
            }
        }
    }

    // Push trails (once per frame, not per sub-step)
    for (int i = 0; i < starCount; i++) {
        if (stars[i].active) stars[i].pushTrail();
    }
}