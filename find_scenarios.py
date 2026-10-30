#!/usr/bin/env python3
"""
Initial conditions for the N-body scenarios in UI::loadScenario.

The simulation uses softened gravity (G = 40, Plummer softening eps = 1.5), so
textbook point-mass setups drift or fall apart; everything here is solved for
the softened force instead.

  infinity   Figure-eight choreography, found by shooting: after T/3 the three
             stars must have taken each other's places (positions and
             velocities). Prints x, z, vx, vz of star 1 and the period.
  ping_pong  Test star around a circular equal-mass binary (2 x 25 Msun at
             +-10). Symmetric periodic orbit in the binary's rotating frame:
             start on the binary axis at x0 moving perpendicular, find the
             speed w0 for which it crosses the axis on the other star's side
             perpendicular again. Also reports the monodromy eigenvalues
             (|lambda| ~ 1: linearly stable).
  fantastic_four
             Two circular binaries on an elliptical mutual orbit. Prints the
             construction; the parameters (a = 12, e = 0.3, separations 3 and
             3.5, phase 2.356, counter-rotating light pair) were picked by
             scanning in the real C++ integrator for 6-7 regular outer orbits
             that hold at 30/60/144 fps and 2x speed - the breakup itself is
             chaotic, so check any change in the app.

Usage:  python3 find_scenarios.py [infinity|ping_pong|fantastic_four|all]
"""

import itertools
import sys

import numpy as np
from scipy.integrate import solve_ivp

G, EPS = 40.0, 1.5
EPS2 = EPS * EPS


def circular_speed(M, d):
    """Relative speed of a circular orbit at separation d (softened)."""
    return np.sqrt(G * M * d * d / (d * d + EPS2) ** 1.5)


# ── Infinity ─────────────────────────────────────────────────────────────────

def infinity(L=12.0, M=8.0):
    X1, Z1 = 0.97000436, -0.24308753          # Chenciner & Montgomery, G = m = 1
    V3 = np.array([-0.93240737, -0.86473146])
    m = np.full(3, M)

    def rhs(t, y):
        p, v = y[:6].reshape(3, 2), y[6:].reshape(3, 2)
        d = p[None, :, :] - p[:, None, :]
        r2 = (d ** 2).sum(-1) + EPS2
        np.fill_diagonal(r2, np.inf)
        a = G * (m[None, :, None] * d / r2[..., None] ** 1.5).sum(1)
        return np.concatenate([v.ravel(), a.ravel()])

    def state(z, vx, vz):
        p = np.array([[x, z], [-x, -z], [0.0, 0.0]])
        v = np.array([[vx, vz], [vx, vz], [-2 * vx, -2 * vz]])
        return p, v

    def flow(u):
        p0, v0 = state(*u[:3])
        y = solve_ivp(rhs, (0, u[3]), np.concatenate([p0.ravel(), v0.ravel()]),
                      method="DOP853", rtol=1e-11, atol=1e-11).y[:, -1]
        return p0, v0, y[:6].reshape(3, 2), y[6:].reshape(3, 2)

    x = X1 * L
    vs = np.sqrt(G * M / L)
    u = np.array([Z1 * L, -V3[0] / 2 * vs, -V3[1] / 2 * vs, 6.32591398 / 3 * np.sqrt(L ** 3 / (G * M))])
    p0, v0, p1, v1 = flow(u)
    perm = list(min(itertools.permutations(range(3)),
                    key=lambda P: np.abs(p1 - p0[list(P)]).sum() + np.abs(v1 - v0[list(P)]).sum()))

    def resid(u):
        p0, v0, p1, v1 = flow(u)
        return np.concatenate([(p1 - p0[perm]).ravel(), (v1 - v0[perm]).ravel()])

    for _ in range(20):
        r = resid(u)
        if np.abs(r).max() < 1e-8:
            break
        J = np.empty((r.size, 4))
        for k in range(4):
            du = np.zeros(4)
            du[k] = 1e-6 * max(1.0, abs(u[k]))
            J[:, k] = (resid(u + du) - r) / du[k]
        u = u - np.linalg.lstsq(J, r, rcond=None)[0]
    z, vx, vz, T3 = u
    print(f"Infinity (3 x {M} Msun, size {L}): residual {np.abs(resid(u)).max():.1e}")
    print(f"  x = {x:.6f}f, z = {z:.6f}f, vx = {vx:.6f}f, vz = {vz:.6f}f, period {3 * T3:.3f} s")


# ── Ping Pong ────────────────────────────────────────────────────────────────

def ping_pong(x0=6.5, bracket=(-20.8, -20.65), M=25.0, R=10.0):
    # x0 = 6.5 has a second, unstable orbit at w0 ~ -20.94; the bracket picks
    # the stable one
    om = 0.5 * circular_speed(2 * M, 2 * R) / R

    def rhs(t, s):
        x, y, vx, vy = s
        ax, ay = om * om * x, om * om * y
        for sx in (R, -R):
            r3 = ((x - sx) ** 2 + y * y + EPS2) ** 1.5
            ax -= G * M * (x - sx) / r3
            ay -= G * M * y / r3
        return [vx, vy, ax + 2 * om * vy, ay - 2 * om * vx]

    def axis(t, s):
        return s[1]

    def shoot(w0):
        sol = solve_ivp(rhs, (0, 30), [x0, 0, 0, w0], method="DOP853", rtol=1e-11, atol=1e-11, events=axis)
        for t, s in zip(sol.t_events[0], sol.y_events[0]):
            if t > 1e-6 and s[0] < -0.5:            # first crossing on the other star's side
                return s[2], t
        return None, None

    a, b = bracket
    fa, _ = shoot(a)
    for _ in range(60):
        mid = 0.5 * (a + b)
        fm, _ = shoot(mid)
        if np.sign(fm) == np.sign(fa):
            a, fa = mid, fm
        else:
            b = mid
    w0 = 0.5 * (a + b)
    _, th = shoot(w0)
    T = 2 * th

    def flow(s0):
        return solve_ivp(rhs, (0, T), s0, method="DOP853", rtol=1e-11, atol=1e-11).y[:, -1]

    s0 = np.array([x0, 0, 0, w0])
    mono = np.column_stack([(flow(s0 + d) - flow(s0 - d)) / 2e-6 for d in np.eye(4) * 1e-6])
    lam = np.abs(np.linalg.eigvals(mono)).max()
    print(f"Ping Pong (binary 2 x {M} Msun at +-{R}, omega {om:.5f}):")
    print(f"  x0 = {x0}f, w0 = {w0:.6f}f (rotating frame), period {T:.3f} s, "
          f"max |eigenvalue| {lam:.3f}")


# ── Fantastic Four ───────────────────────────────────────────────────────────

def fantastic_four(mA=3.0, mB=1.0, dA=3.0, dB=3.5, a=12.0, e=0.3):
    MA, MB = 2 * mA, 2 * mB
    M = MA + MB
    R = a * (1 + e)
    v = np.sqrt(G * M * (1 - e) / R)
    T = 2 * np.pi * np.sqrt(a ** 3 / (G * M))
    print(f"Fantastic Four: outer period {T:.1f} s, apoapsis {R:.1f}, relative speed {v:.3f}")
    print(f"  heavy pair 2 x {mA} at separation {dA} (orbital speed {circular_speed(MA, dA):.3f}), "
          f"light pair 2 x {mB} at {dB} (speed {circular_speed(MB, dB):.3f}, counter-rotating)")


if __name__ == "__main__":
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    if which in ("all", "infinity"):
        infinity()
    if which in ("all", "ping_pong"):
        ping_pong()
    if which in ("all", "fantastic_four"):
        fantastic_four()
