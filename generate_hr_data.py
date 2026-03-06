#!/usr/bin/env python3
"""
Generate assets/hr_data.csv from theoretical stellar models.
Realistic scatter from metallicity, age spread, binaries, rotation.
"""

import numpy as np
import os

TARGET_TOTAL = 500
SEED = 42


def ms_properties(mass, feh=0.0):
    """Main sequence properties with metallicity dependence."""
    # Metallicity shifts: metal-poor → hotter, less luminous
    t_shift = 1.0 + 0.06 * feh   # feh=-2 → 0.88, feh=+0.3 → 1.02
    l_shift = 1.0 + 0.15 * feh

    if mass < 0.43:
        L = 0.23 * mass ** 2.3
    elif mass < 2.0:
        L = mass ** 4.0
    elif mass < 55.0:
        L = 1.4 * mass ** 3.5
    else:
        L = 32000.0 * mass / 55.0

    if mass < 0.7:
        R = mass ** 0.8
    elif mass < 2.0:
        R = mass ** 0.57
    else:
        R = 1.33 * mass ** 0.555

    Tsun = 5778.0
    T = Tsun * (L / (R * R)) ** 0.25
    return T * t_shift, L * l_shift


def evolved_fraction(mass, age_frac):
    """How far through MS lifetime. age_frac=0 ZAMS, 1=TAMS."""
    # Stars brighten and cool as they age on MS
    L_boost = 1.0 + 1.5 * age_frac ** 2
    T_shift = 1.0 - 0.08 * age_frac
    return T_shift, L_boost


def generate():
    rng = np.random.default_rng(SEED)
    stars = []

    def add(T, L):
        if 2000 < T < 60000 and 1e-5 < L < 5e6:
            stars.append((T, L))

    # ── Main sequence ─────────────────────────────────────────
    n_ms = int(TARGET_TOTAL * 0.48)
    log_masses = rng.uniform(np.log10(0.08), np.log10(120.0), n_ms)
    masses = 10.0 ** log_masses

    for m in masses:
        feh = rng.normal(-0.2, 0.5)            # metallicity spread
        feh = np.clip(feh, -2.0, 0.5)
        age_frac = rng.beta(2, 3)               # most stars mid-MS
        T, L = ms_properties(m, feh)
        t_ev, l_ev = evolved_fraction(m, age_frac)
        T *= t_ev
        L *= l_ev

        # Large scatter: rotation, activity, spots, calibration
        T *= rng.normal(1.0, 0.05)
        L *= 10.0 ** rng.normal(0.0, 0.12)
        add(T, L)

    # ── Unresolved binaries (shifted ~0.75 mag up) ───────────
    n_bin = int(TARGET_TOTAL * 0.10)
    log_masses = rng.uniform(np.log10(0.1), np.log10(50.0), n_bin)
    for lm in log_masses:
        m = 10.0 ** lm
        feh = rng.normal(-0.2, 0.4)
        feh = np.clip(feh, -1.5, 0.4)
        T, L = ms_properties(m, feh)
        q = rng.uniform(0.3, 1.0)              # mass ratio
        T2, L2 = ms_properties(m * q, feh)
        # Combined light
        L_tot = L + L2
        # T biased toward primary but shifted slightly
        T_comb = T * (L / L_tot) + T2 * (L2 / L_tot)
        T_comb *= rng.normal(1.0, 0.04)
        L_tot *= 10.0 ** rng.normal(0.0, 0.08)
        add(T_comb, L_tot)

    # ── Subgiant branch ──────────────────────────────────────
    n_sg = int(TARGET_TOTAL * 0.05)
    for _ in range(n_sg):
        m = 10.0 ** rng.uniform(np.log10(0.9), np.log10(5.0))
        feh = rng.normal(-0.3, 0.5)
        feh = np.clip(feh, -2.0, 0.4)
        T_ms, L_ms = ms_properties(m, feh)
        frac = rng.random()
        # Subgiants: T drops, L rises mildly
        T = T_ms * (1.0 - 0.25 * frac) + rng.normal(0, 150)
        L = L_ms * (1.5 + 4.0 * frac) * 10.0 ** rng.normal(0, 0.1)
        add(T, L)

    # ── Red giant branch ─────────────────────────────────────
    n_rgb = int(TARGET_TOTAL * 0.12)
    for _ in range(n_rgb):
        m = 10.0 ** rng.uniform(np.log10(0.7), np.log10(8.0))
        feh = rng.normal(-0.5, 0.7)
        feh = np.clip(feh, -2.5, 0.4)
        frac = rng.beta(1.5, 1.0)  # spread along branch

        T_ms, L_ms = ms_properties(m)
        L_tip = L_ms * (30.0 + 200.0 * (m / 2.0) ** 0.5)
        L = L_ms * 5.0 * (L_tip / (L_ms * 5.0)) ** frac

        # Broad temperature range, metallicity-dependent
        T_base = 5200.0 + 400.0 * feh
        T = T_base - 1500.0 * frac + rng.normal(0, 250)
        T = np.clip(T, 2800, 5500)

        L *= 10.0 ** rng.normal(0.0, 0.15)
        add(T, L)

    # ── Red clump / horizontal branch ─────────────────────────
    n_rc = int(TARGET_TOTAL * 0.06)
    for _ in range(n_rc):
        feh = rng.normal(-0.3, 0.6)
        feh = np.clip(feh, -2.5, 0.4)

        if feh < -1.0:
            # Metal-poor: blue HB
            T = rng.uniform(7000, 25000)
            L = rng.uniform(20, 80) * 10.0 ** rng.normal(0, 0.12)
        else:
            # Metal-rich: red clump
            T = rng.normal(4750, 300)
            L = rng.normal(45, 15)
            L = max(L, 10)

        add(T, L)

    # ── AGB ───────────────────────────────────────────────────
    n_agb = int(TARGET_TOTAL * 0.04)
    for _ in range(n_agb):
        m = rng.uniform(1.0, 7.0)
        frac = rng.random()
        L = (800.0 + 8000.0 * frac) * 10.0 ** rng.normal(0, 0.15)
        T = 2600.0 + 900.0 * (1.0 - frac) + rng.normal(0, 200)
        T = max(T, 2200.0)
        add(T, L)

    # ── Supergiants ───────────────────────────────────────────
    n_sup = int(TARGET_TOTAL * 0.06)
    sg_masses = 10.0 ** rng.uniform(np.log10(8.0), np.log10(120.0), n_sup)
    for m in sg_masses:
        T_ms, L_ms = ms_properties(m)
        phase = rng.random()

        if phase < 0.3:
            # Blue supergiant
            T = T_ms * rng.uniform(0.6, 1.0) + rng.normal(0, 1000)
            L = L_ms * rng.uniform(1.5, 8.0)
        elif phase < 0.5:
            # Yellow supergiant
            T = rng.uniform(4500, 7500) + rng.normal(0, 500)
            L = L_ms * rng.uniform(2.0, 10.0)
        else:
            # Red supergiant
            T = rng.uniform(2800, 4200) + rng.normal(0, 300)
            L = L_ms * rng.uniform(3.0, 15.0)

        L *= 10.0 ** rng.normal(0, 0.12)
        T = max(T, 2500)
        add(T, L)

    # ── Wolf-Rayet / very hot luminous ────────────────────────
    n_wr = int(TARGET_TOTAL * 0.01)
    for _ in range(n_wr):
        T = rng.uniform(30000, 100000)
        L = 10.0 ** rng.uniform(4.5, 6.2)
        T += rng.normal(0, 3000)
        L *= 10.0 ** rng.normal(0, 0.15)
        add(T, L)

    # ── White dwarfs ──────────────────────────────────────────
    n_wd = int(TARGET_TOTAL * 0.06)
    for _ in range(n_wd):
        wd_mass = rng.normal(0.6, 0.15)
        wd_mass = np.clip(wd_mass, 0.3, 1.2)
        cooling = rng.exponential(2.5)
        T = (50000.0 - 15000.0 * wd_mass) * np.exp(-cooling * 0.35) + 3800.0
        R_wd = 0.0115 * (1.44 / wd_mass) ** (1.0 / 3.0)
        L = (R_wd ** 2) * (T / 5778.0) ** 4
        T += rng.normal(0, 500)
        L *= 10.0 ** rng.normal(0, 0.1)
        T = np.clip(T, 3500, 80000)
        L = np.clip(L, 1e-5, 2.0)
        add(T, L)

    # ── Pre-main-sequence ─────────────────────────────────────
    n_pms = int(TARGET_TOTAL * 0.02)
    for _ in range(n_pms):
        m = 10.0 ** rng.uniform(np.log10(0.2), np.log10(4.0))
        T_ms, L_ms = ms_properties(m)
        T = T_ms * rng.uniform(0.55, 0.90) + rng.normal(0, 200)
        L = L_ms * rng.uniform(2.0, 30.0) * 10.0 ** rng.normal(0, 0.2)
        add(T, L)

    stars = np.array(stars)
    teff = stars[:, 0]
    lum = stars[:, 1]

    # ── Stats ─────────────────────────────────────────────────
    print(f"Generated {len(teff)} stars\n")
    print("Distribution by luminosity:")
    for lo, hi, label in [
        (0, 0.01, "L < 0.01 (WD)"),
        (0.01, 0.1, "0.01 ≤ L < 0.1"),
        (0.1, 1, "0.1 ≤ L < 1"),
        (1, 10, "1 ≤ L < 10"),
        (10, 100, "10 ≤ L < 100"),
        (100, 1000, "100 ≤ L < 1k"),
        (1000, 10000, "1k ≤ L < 10k"),
        (10000, 100000, "10k ≤ L < 100k"),
        (100000, 1e7, "L ≥ 100k"),
    ]:
        n = ((lum >= lo) & (lum < hi)).sum()
        print(f"  {label:22s}: {n:>5d}")

    print(f"\n  Teff range: {teff.min():.0f} – {teff.max():.0f} K")
    print(f"  L range:    {lum.min():.6f} – {lum.max():.0f} L☉")

    # ── Write ─────────────────────────────────────────────────
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets")
    os.makedirs(outdir, exist_ok=True)
    outpath = os.path.join(outdir, "hr_data.csv")

    with open(outpath, "w") as f:
        f.write("# Theoretical H-R diagram data (teff, luminosity in solar units)\n")
        for t, l in zip(teff, lum):
            f.write(f"{t:.1f},{l:.4f}\n")

    print(f"\nWrote {len(teff)} stars to {outpath}")


if __name__ == "__main__":
    generate()