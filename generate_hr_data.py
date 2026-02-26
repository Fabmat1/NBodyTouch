#!/usr/bin/env python3
"""
Generate realistic H-R diagram background data for the N-Body app.
Outputs CSV to assets/hr_data.csv with (teff_K, luminosity_Lsun) pairs.

Uses approximate stellar distributions:
  - Main sequence (dominant population)
  - Red giant branch
  - Horizontal branch / red clump
  - White dwarfs

Run: python3 generate_hr_data.py
"""

import numpy as np
import os

RNG = np.random.default_rng(42)
OUTPUT = os.path.join(os.path.dirname(__file__), "assets", "hr_data.csv")


def ms_luminosity(mass):
    """Main-sequence mass-luminosity relation (approximate)."""
    if mass < 0.43:
        return 0.23 * mass**2.3
    elif mass < 2.0:
        return mass**4.0
    elif mass < 55.0:
        return 1.4 * mass**3.5
    else:
        return 32000.0 * mass


def ms_temperature(mass, lum):
    """Main-sequence effective temperature from L and R(M)."""
    radius = mass**0.8 if mass < 1.0 else mass**0.57
    # Stefan-Boltzmann: L = 4pi R^2 sigma T^4  (solar units)
    # T/Tsun = (L/Lsun)^0.25 / (R/Rsun)^0.5
    t = 5778.0 * (lum**0.25) / (radius**0.5)
    return t


def generate_main_sequence(n=600):
    """Sample stars along the main sequence with a Salpeter-like IMF."""
    # Salpeter IMF: dN/dM ~ M^-2.35, sample via inverse CDF
    mmin, mmax = 0.08, 40.0
    alpha = 2.35
    u = RNG.uniform(0, 1, n)
    masses = (mmin**(1 - alpha) + u * (mmax**(1 - alpha) - mmin**(1 - alpha))) ** (1.0 / (1 - alpha))

    points = []
    for m in masses:
        l = ms_luminosity(m)
        t = ms_temperature(m, l)
        # Add scatter (real MS has width)
        t *= RNG.normal(1.0, 0.03)
        l *= 10 ** RNG.normal(0.0, 0.08)
        points.append((t, l))
    return points


def generate_red_giants(n=120):
    """Red giant branch: cool, luminous."""
    points = []
    for _ in range(n):
        # RGB luminosity range ~10 to 3000 Lsun
        log_l = RNG.uniform(1.0, 3.5)
        l = 10**log_l
        # Temperature narrows as luminosity increases
        t_base = 5100 - 400 * log_l  # cooler at higher L
        t = RNG.normal(t_base, 150)
        t = max(3000, min(5200, t))
        points.append((t, l))
    return points


def generate_red_clump(n=80):
    """Horizontal branch / red clump: concentrated region."""
    points = []
    for _ in range(n):
        t = RNG.normal(4800, 200)
        l = 10 ** RNG.normal(1.7, 0.15)  # ~50 Lsun
        points.append((t, l))
    return points


def generate_subgiants(n=60):
    """Subgiant branch: between MS and RGB."""
    points = []
    for _ in range(n):
        t = RNG.normal(5500, 300)
        l = 10 ** RNG.normal(0.7, 0.2)  # ~5 Lsun
        points.append((t, l))
    return points


def generate_white_dwarfs(n=100):
    """White dwarf cooling sequence."""
    points = []
    for _ in range(n):
        # WD temperatures: 5000 to 40000 K
        t = 10 ** RNG.uniform(np.log10(4500), np.log10(40000))
        # WD luminosities: ~0.5 Rsun → L ~ (R/Rsun)^2 * (T/Tsun)^4
        r_wd = RNG.normal(0.012, 0.003)
        r_wd = max(0.005, r_wd)
        l = r_wd**2 * (t / 5778.0)**4
        l = max(1e-5, min(1.0, l))
        points.append((t, l))
    return points


def generate_blue_stragglers(n=20):
    """A few blue stragglers above the MS turnoff."""
    points = []
    for _ in range(n):
        t = RNG.normal(8500, 800)
        l = 10 ** RNG.normal(1.5, 0.3)
        points.append((t, l))
    return points


def main():
    all_points = []
    all_points += generate_main_sequence(1000)
    all_points += generate_red_giants(50)
    all_points += generate_red_clump(80)
    all_points += generate_subgiants(60)
    all_points += generate_white_dwarfs(30)
    all_points += generate_blue_stragglers(20)

    # Clamp to reasonable ranges
    filtered = []
    for t, l in all_points:
        t = max(2000, min(45000, t))
        l = max(1e-5, min(1e6, l))
        filtered.append((t, l))

    # Shuffle so populations are mixed in file
    RNG.shuffle(filtered)

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, "w") as f:
        f.write("# H-R Diagram data (auto-generated)\n")
        f.write("# teff_K,luminosity_Lsun\n")
        for t, l in filtered:
            f.write(f"{t:.0f},{l:.6g}\n")

    print(f"Written {len(filtered)} points to {OUTPUT}")

    # Quick stats
    print(f"  Main sequence:   600")
    print(f"  Red giants:      120")
    print(f"  Red clump:        80")
    print(f"  Subgiants:        60")
    print(f"  White dwarfs:    100")
    print(f"  Blue stragglers:  20")


if __name__ == "__main__":
    main()