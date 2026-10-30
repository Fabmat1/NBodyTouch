// src/simulation.cpp

#include "simulation.h"
#include <cmath>
#include <utility>

int Simulation::addStar(Vector3 pos, Vector3 vel, float mass, float age) {
    if (starCount >= MAX_STARS) return -1;
    int i = starCount++;
    // The simulation is planar (y = 0). Touch placement hits the plane only
    // up to float rounding, and chaotic encounters amplify such tiny
    // offsets until stars leave the plane, so snap them onto it exactly.
    pos.y = 0.0f;
    vel.y = 0.0f;
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

void Simulation::removeStar(int i) {
    if (i < 0 || i >= starCount) return;
    int last = starCount - 1;
    if (i != last) stars[i] = std::move(stars[last]);
    stars[last] = Star{};
    starCount--;
}

bool Simulation::isNegligibleEscaper(int i) const {
    constexpr float MIN_DISTANCE = 150.0f;   // never cull anything close by
    constexpr float MAX_TIDAL    = 1e-4f;    // tidal pull / the rest's own gravity

    const Star &s = stars[i];
    if (!s.active) return false;

    // Centre of mass, velocity and size of all the other stars
    float   mRest = 0.0f;
    Vector3 rRest = {0, 0, 0}, vRest = {0, 0, 0};
    for (int j = 0; j < starCount; j++) {
        if (j == i || !stars[j].active) continue;
        mRest += stars[j].mass;
        rRest  = Vector3Add(rRest, Vector3Scale(stars[j].pos, stars[j].mass));
        vRest  = Vector3Add(vRest, Vector3Scale(stars[j].vel, stars[j].mass));
    }
    if (mRest <= 0.0f) return false;
    rRest = Vector3Scale(rRest, 1.0f / mRest);
    vRest = Vector3Scale(vRest, 1.0f / mRest);

    Vector3 r = Vector3Subtract(s.pos, rRest);
    Vector3 v = Vector3Subtract(s.vel, vRest);
    float   d = Vector3Length(r);
    if (d < MIN_DISTANCE || Vector3DotProduct(r, v) <= 0.0f) return false;

    // Two-body energy relative to the rest: >= 0 means it never comes back
    float energy = 0.5f * Vector3DotProduct(v, v) - G_CONST * (mRest + s.mass) / d;
    if (energy <= 0.0f) return false;

    // Its tidal acceleration across the rest (size R) relative to their own
    // gravity: (2 G m R / d^3) / (G M / R^2) = 2 (m / M) (R / d)^3
    float size2 = 0.0f;
    for (int j = 0; j < starCount; j++) {
        if (j == i || !stars[j].active) continue;
        Vector3 dj = Vector3Subtract(stars[j].pos, rRest);
        size2 += stars[j].mass * Vector3DotProduct(dj, dj);
    }
    float R = fmaxf(sqrtf(size2 / mRest), softening);
    float q = R / d;
    return 2.0f * (s.mass / mRest) * q * q * q < MAX_TIDAL;
}

void Simulation::updateBoundCore() {
    // Hysteresis as a fraction of a star's potential well: members leave only
    // when clearly unbound, outsiders join only when clearly bound
    constexpr float HYSTERESIS = 0.1f;

    bool cand[MAX_STARS];
    int  nCand = 0;
    for (int i = 0; i < starCount; i++) {
        cand[i] = stars[i].active;
        nCand  += cand[i];
    }

    while (nCand > 1) {
        float   mTot = 0.0f;
        Vector3 pTot = {0, 0, 0};   // total momentum
        for (int i = 0; i < starCount; i++) {
            if (!cand[i]) continue;
            mTot += stars[i].mass;
            pTot  = Vector3Add(pTot, Vector3Scale(stars[i].vel, stars[i].mass));
        }

        int   worst      = -1;
        float worstScore = 0.0f;
        for (int i = 0; i < starCount; i++) {
            if (!cand[i]) continue;
            const Star &s = stars[i];
            float mRest = mTot - s.mass;
            if (mRest <= 0.0f) continue;

            // Potential of the other candidates at this star
            float phi = 0.0f;
            for (int j = 0; j < starCount; j++) {
                if (j == i || !cand[j]) continue;
                Vector3 d = Vector3Subtract(stars[j].pos, s.pos);
                phi -= G_CONST * stars[j].mass /
                       sqrtf(Vector3DotProduct(d, d) + softening * softening);
            }
            // Energy of its orbit relative to the rest (per reduced mass);
            // for two stars this is exactly the two-body binding criterion
            Vector3 vRest = Vector3Scale(Vector3Subtract(pTot, Vector3Scale(s.vel, s.mass)),
                                         1.0f / mRest);
            Vector3 v     = Vector3Subtract(s.vel, vRest);
            float   well  = phi * (mRest + s.mass) / mRest;
            float   e     = 0.5f * Vector3DotProduct(v, v) + well;
            float   excess = s.bound ? e + HYSTERESIS * well : e - HYSTERESIS * well;
            if (excess <= 0.0f) continue;

            // Of the unbound ones drop the star carrying most of the energy,
            // i.e. the faster / lighter one, so the slow massive core stays
            float score = excess * mRest / mTot;
            if (score > worstScore) { worstScore = score; worst = i; }
        }
        if (worst < 0) break;
        cand[worst] = false;
        nCand--;
    }

    for (int i = 0; i < starCount; i++) stars[i].bound = cand[i];
}

void Simulation::computeAccelerations(const State &s, int n, Vector3 *accel) const {
    for (int i = 0; i < n; i++) accel[i] = {0, 0, 0};

    for (int i = 0; i < n; i++) {
        if (!stars[i].active) continue;
        for (int j = i + 1; j < n; j++) {
            if (!stars[j].active) continue;

            Vector3 diff = Vector3Subtract(s.pos[j], s.pos[i]);
            diff.y = 0.0f;   // in-plane forces only: y stays exactly 0
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