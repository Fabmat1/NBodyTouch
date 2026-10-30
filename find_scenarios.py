#!/usr/bin/env python3
"""
Find good initial conditions for N-Body scenarios that survive the
softened gravity used in the sim (G=40, eps=1.5).

Usage:  python3 find_scenarios.py [fantastic_four|infinity|ping_pong|all]
"""

import numpy as np
import sys

G     = 40.0
SOFT2 = 1.5 ** 2

# ────────────────────────────────────────────────────────────────
#  Physics: mirrors src/simulation.cpp exactly (RK4 + Plummer)
# ────────────────────────────────────────────────────────────────
def accel(pos, mass):
    n = pos.shape[0]
    a = np.zeros_like(pos)
    for i in range(n):
        for j in range(n):
            if i == j: continue
            d = pos[j] - pos[i]
            r2 = d @ d + SOFT2
            a[i] += G * mass[j] * d / (r2 * np.sqrt(r2))
    return a

def rk4(pos, vel, mass, dt):
    k1p, k1v = vel,               accel(pos,              mass)
    k2p, k2v = vel + 0.5*dt*k1v,  accel(pos + 0.5*dt*k1p, mass)
    k3p, k3v = vel + 0.5*dt*k2v,  accel(pos + 0.5*dt*k2p, mass)
    k4p, k4v = vel +     dt*k3v,  accel(pos +     dt*k3p, mass)
    return (pos + dt/6*(k1p + 2*k2p + 2*k3p + k4p),
            vel + dt/6*(k1v + 2*k2v + 2*k3v + k4v))

def simulate(pos, vel, mass, t_end, dt, callback=None):
    pos, vel, mass = np.asarray(pos,float), np.asarray(vel,float), np.asarray(mass,float)
    t = 0.0
    while t < t_end:
        if callback and not callback(t, pos, vel): return t
        pos, vel = rk4(pos, vel, mass, dt)
        t += dt
    return t

def cpp_snippet(label, stars):
    print(f"\n// {label}")
    for p, v, m in stars:
        print(f"sim.addStar({{{p[0]:+.4f}f, {p[1]:+.4f}f, {p[2]:+.4f}f}}, "
              f"{{{v[0]:+.4f}f, {v[1]:+.4f}f, {v[2]:+.4f}f}}, "
              f"{m}f, 0.0f);")

# ────────────────────────────────────────────────────────────────
#  SCENARIO 1 — Fantastic Four (analytical)
# ────────────────────────────────────────────────────────────────
def fantastic_four():
    print("\n═══ Fantastic Four ═══")
    m_in, m_out = 3.0, 0.5
    a_in, a_out = 0.6, 0.3    # half-separations (tight binaries)
    R           = 6.0         # distance between inner- and outer-binary COMs

    M_in_tot, M_out_tot = 2*m_in, 2*m_out
    M_tot = M_in_tot + M_out_tot

    # Each binary-COM orbits system-COM on circular Kepler orbit
    v_rel = np.sqrt(G * M_tot / R)
    v_in_com  = v_rel * M_out_tot / M_tot
    v_out_com = v_rel * M_in_tot  / M_tot
    c_in  = R * M_out_tot / M_tot   # inner-COM distance from origin
    c_out = R * M_in_tot  / M_tot

    # Within-binary circular velocities
    v_in_orb  = np.sqrt(G * m_in  / (4 * a_in))
    v_out_orb = np.sqrt(G * m_out / (4 * a_out))

    stars = [
        ((-c_in + a_in,  0, 0), (0, 0,  v_in_orb  + v_in_com ), m_in),
        ((-c_in - a_in,  0, 0), (0, 0, -v_in_orb  + v_in_com ), m_in),
        (( c_out + a_out, 0, 0), (0, 0,  v_out_orb - v_out_com), m_out),
        (( c_out - a_out, 0, 0), (0, 0, -v_out_orb - v_out_com), m_out),
    ]

    T_in_bin  = 2*np.pi*np.sqrt((2*a_in )**3/(G*M_in_tot ))
    T_out_bin = 2*np.pi*np.sqrt((2*a_out)**3/(G*M_out_tot))
    T_sys     = 2*np.pi*np.sqrt(R**3       /(G*M_tot     ))
    print(f"  T_inner_bin={T_in_bin:.3f}  T_outer_bin={T_out_bin:.3f}  T_sys={T_sys:.3f}")
    print(f"  Stability ratios T_sys/T_in_bin={T_sys/T_in_bin:.1f}  "
          f"T_sys/T_out_bin={T_sys/T_out_bin:.1f}")

    # Verify: run 50 time units, ensure no star flies off
    pos = np.array([s[0] for s in stars])
    vel = np.array([s[1] for s in stars])
    m   = np.array([s[2] for s in stars])
    escaped = [False]
    def cb(t,p,v):
        if np.linalg.norm(p,axis=1).max() > 30:
            escaped[0] = True; return False
        return True
    simulate(pos, vel, m, 50.0, 0.005, cb)
    print(f"  Survived 50 time units: {not escaped[0]}")
    cpp_snippet("Fantastic Four", stars)

# ────────────────────────────────────────────────────────────────
#  SCENARIO 2 — Infinity (sweep scale to beat softening)
# ────────────────────────────────────────────────────────────────
def infinity():
    print("\n═══ Infinity ═══")
    px_c, py_c = 0.97000436, -0.24308753
    vx_c, vy_c = 0.46620368,  0.43236573
    M = 2.0

    best = None
    for L in np.arange(6, 40, 2.0):
        V = np.sqrt(G * M / L)
        pos0 = np.array([[ px_c*L, 0,  py_c*L],
                         [-px_c*L, 0, -py_c*L],
                         [      0, 0,       0]])
        vel0 = np.array([[   vx_c*V, 0,   vy_c*V],
                         [   vx_c*V, 0,   vy_c*V],
                         [-2*vx_c*V, 0,-2*vy_c*V]])
        T = 6.3259 * np.sqrt(L**3 / (G*M))       # canonical period scaled

        # drift: after 5 periods, how far is center-of-configuration from start?
        good = [True]
        def cb(t,p,v):
            if np.linalg.norm(p,axis=1).max() > 4*L:
                good[0] = False; return False
            return True
        t_end = simulate(pos0, vel0, [M]*3, 5*T, T/400, cb)
        survived = good[0]
        print(f"  L={L:4.1f}  T={T:5.2f}  survived_5T={survived}")
        if survived and (best is None or L < best):
            best = L

    L = best if best else 20.0
    print(f"  → chosen L = {L}")
    V = np.sqrt(G * M / L)
    stars = [
        (( px_c*L, 0,  py_c*L), (   vx_c*V, 0,   vy_c*V), M),
        ((-px_c*L, 0, -py_c*L), (   vx_c*V, 0,   vy_c*V), M),
        ((      0, 0,       0), (-2*vx_c*V, 0,-2*vy_c*V), M),
    ]
    cpp_snippet("Infinity — figure-eight", stars)

# ────────────────────────────────────────────────────────────────
#  SCENARIO 3 — Ping Pong (grid-search restricted-3-body)
# ────────────────────────────────────────────────────────────────
def ping_pong():
    print("\n═══ Ping Pong ═══")
    m_star, m_test, r_bin = 25.0, 0.1, 8.0
    v_bin = np.sqrt(G * m_star / (4*r_bin))
    T_bin = 2*np.pi*np.sqrt((2*r_bin)**3/(G*2*m_star))
    print(f"  Binary v={v_bin:.3f} T={T_bin:.3f}")

    def score(x0,z0,vx,vz, n_periods=15):
        pos = np.array([[ r_bin,0,0], [-r_bin,0,0], [ x0,0,z0]], float)
        vel = np.array([[0,0, v_bin], [0,0,-v_bin], [vx,0,vz ]], float)
        mass = np.array([m_star, m_star, m_test])

        last_side   = None     # which star the test particle was closer to
        bounces     = 0
        time_bound  = 0.0
        dt = T_bin / 400
        t  = 0.0
        while t < n_periods*T_bin:
            d0 = np.linalg.norm(pos[2]-pos[0])
            d1 = np.linalg.norm(pos[2]-pos[1])
            r  = np.linalg.norm(pos[2])
            if r > 4*r_bin: break            # escaped
            if min(d0,d1) < 0.2: break       # merged / collided
            side = 0 if d0 < d1 else 1
            if last_side is not None and side != last_side:
                # only count as "bounce" if we came reasonably close
                if min(d0,d1) < 1.5*r_bin:
                    bounces += 1
            last_side = side
            pos, vel = rk4(pos, vel, mass, dt)
            t += dt
            time_bound = t
        return bounces, time_bound

    best = (0, 0, None)
    # grid search
    for x0 in np.linspace(-5, 5, 11):
        for z0 in np.linspace(-3, 3, 7):
            for vx in np.linspace(-3, 3, 7):
                for vz in np.linspace(-4, 4, 9):
                    b, tb = score(x0,z0,vx,vz)
                    if b > best[0] or (b == best[0] and tb > (best[1] if best[2] else 0)):
                        if b > 2:
                            best = (b, tb, (x0,z0,vx,vz))

    if best[2] is None:
        print("  no multi-bounce config found; using safe fallback")
        best = (0, 0, (0.0, 0.5, 2.0, 0.0))
    b, tb, (x0,z0,vx,vz) = best
    print(f"  best: bounces={b}  t_bound={tb:.2f}")
    print(f"  IC:   x={x0} z={z0} vx={vx} vz={vz}")

    # refine around the winner
    from itertools import product
    refined = best
    for dx,dz,dvx,dvz in product(*[np.linspace(-0.3,0.3,5)]*4):
        b, tb = score(x0+dx, z0+dz, vx+dvx, vz+dvz, n_periods=20)
        if b > refined[0] or (b == refined[0] and tb > refined[1]):
            refined = (b, tb, (x0+dx, z0+dz, vx+dvx, vz+dvz))
    b, tb, (x0,z0,vx,vz) = refined
    print(f"  refined: bounces={b}  t_bound={tb:.2f}")
    print(f"  IC: x={x0:.3f} z={z0:.3f} vx={vx:.3f} vz={vz:.3f}")

    stars = [
        (( r_bin,0,0), (0,0, v_bin), m_star),
        ((-r_bin,0,0), (0,0,-v_bin), m_star),
        (( x0,0,z0  ), (vx,0,vz   ), m_test),
    ]
    cpp_snippet(f"Ping Pong — {b} bounces", stars)

# ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    if which in ("all","fantastic_four"): fantastic_four()
    if which in ("all","infinity"):       infinity()
    if which in ("all","ping_pong"):      ping_pong()