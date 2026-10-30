#!/usr/bin/env python3
"""
Build the H-R diagram data from real observations.

  assets/hr_data.csv            Gaia DR3 colour-magnitude points (BP-RP, M_G)
  include/main_sequence_table.h Dwarf sequence (mass -> Teff, L, R, BP-RP, M_G)

Background stars (two samples, merged on source_id):
  * Gaia Catalogue of Nearby Stars (Gaia Collaboration, Smart et al. 2021):
    volume-complete within 100 pc -> M dwarfs, white dwarfs
  * Hipparcos stars with Gaia DR3 photometry: magnitude-limited, so it reaches
    the rarer luminous stars (upper main sequence, giants, red clump)
Both are restricted to precise parallaxes and clean BP/RP photometry.

Main sequence: Pecaut & Mamajek (2013), "A Modern Mean Dwarf Stellar Color and
Effective Temperature Sequence", version 2022.04.16 or later. Gaia colours are
only tabulated for B9V and cooler; hotter types are extrapolated from B-V.

Usage: python3 fetch_hr_data.py [--cache DIR]
Downloads are cached (default .hr_cache/) because the anonymous Gaia archive
limits queries to 60 s, so the samples are fetched in source_id chunks.
"""

import argparse
import os
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor

import numpy as np

ROOT = os.path.dirname(os.path.abspath(__file__))
TAP = "https://gea.esac.esa.int/tap-server/tap/async"
MAMAJEK_URL = "https://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt"

GCNS_QUERY = """
SELECT source_id, phot_g_mean_mag, phot_bp_mean_mag, phot_rp_mean_mag,
       parallax, parallax_error, ruwe, phot_bp_rp_excess_factor,
       phot_bp_mean_flux_over_error AS bp_snr, phot_rp_mean_flux_over_error AS rp_snr
FROM external.gaiaedr3_gcns_main_1
WHERE source_id >= {lo} AND source_id < {hi}
"""

HIP_QUERY = """
SELECT g.source_id, g.phot_g_mean_mag, g.bp_rp, g.parallax, g.parallax_over_error,
       g.ruwe, g.phot_bp_rp_excess_factor, g.l, g.b,
       ap.ebpminrp_gspphot, ap.ag_gspphot
FROM gaiadr3.hipparcos2_best_neighbour AS h
JOIN gaiadr3.gaia_source AS g ON g.source_id = h.source_id
LEFT OUTER JOIN gaiadr3.astrophysical_parameters AS ap ON ap.source_id = g.source_id
WHERE h.source_id >= {lo} AND h.source_id < {hi}
"""


# ── Gaia archive ────────────────────────────────────────────────────────────

def tap_async(query, out):
    data = urllib.parse.urlencode({
        "REQUEST": "doQuery", "LANG": "ADQL", "FORMAT": "csv",
        "PHASE": "RUN", "MAXREC": "3000000", "QUERY": query,
    }).encode()
    with urllib.request.urlopen(urllib.request.Request(TAP, data=data)) as r:
        job = r.geturl()
    while True:
        phase = urllib.request.urlopen(job + "/phase").read().decode().strip()
        if phase in ("COMPLETED", "ERROR", "ABORTED"):
            break
        time.sleep(5)
    if phase != "COMPLETED":
        raise RuntimeError(f"{job}: {phase}")
    urllib.request.urlretrieve(job + "/results/result", out + ".tmp")
    os.rename(out + ".tmp", out)


def fetch_chunked(template, out, chunks):
    """Run template over source_id ranges in parallel and concatenate.
    A range that keeps hitting the 60 s limit is split into four."""
    if os.path.exists(out):
        return
    edges = [int(i * 6.92e18 / chunks) for i in range(chunks)] + [7 * 10**18]

    def job(lo, hi, depth=0):
        part = f"{out}.{lo}"
        if os.path.exists(part):
            return [part]
        for attempt in range(2):
            try:
                tap_async(template.format(lo=lo, hi=hi), part)
                return [part]
            except Exception as e:
                print(f"  range {lo}..{hi} attempt {attempt + 1}: {e}", file=sys.stderr)
        if depth >= 3:
            sys.exit(f"{out}: range {lo}..{hi} keeps failing, rerun to retry")
        cuts = [lo + (hi - lo) * k // 4 for k in range(5)]
        return [p for k in range(4) for p in job(cuts[k], cuts[k + 1], depth + 1)]

    with ThreadPoolExecutor(4) as ex:
        parts = [p for ps in ex.map(lambda i: job(edges[i], edges[i + 1]), range(chunks)) for p in ps]
    with open(out, "w") as f:
        for i, part in enumerate(parts):
            lines = open(part).read().splitlines()
            if i == 0:
                f.write(lines[0] + "\n")
            f.writelines(line + "\n" for line in lines[1:])
    for part in parts:
        os.remove(part)


def read_csv(path):
    return np.genfromtxt(path, delimiter=",", names=True, dtype=None, encoding="utf-8")


# ── Main sequence table ─────────────────────────────────────────────────────

def mamajek_table(path):
    rows, block = [], 0
    for line in open(path):
        if line.startswith("#SpT"):
            block += 1
            continue
        t = line.split()
        if block == 1 and len(t) >= 31:
            rows.append(t)

    def num(x):
        try:
            return float(x.rstrip(":"))
        except ValueError:
            return np.nan

    a = np.array([[num(x) for x in r[1:31]] for r in rows])
    spt = np.array([r[0] for r in rows])
    teff, logl, rad, mv, bv, gv, bprp, mg, mass = (
        a[:, 0], a[:, 3], a[:, 5], a[:, 6], a[:, 7], a[:, 9], a[:, 10], a[:, 12], a[:, 29])

    # Hot end: BP-RP and G-V are linear in B-V over late B / early A dwarfs
    hot = (teff >= 8800) & (teff <= 10700) & np.isfinite(bprp)
    k = np.polyfit(bv[hot], bprp[hot], 1)
    bprp = np.where(np.isfinite(bprp), bprp, np.polyval(k, bv))
    hot = (teff >= 10700) & (teff <= 25000) & np.isfinite(gv)
    k = np.polyfit(bv[hot], gv[hot], 1)
    gv = np.where(np.isfinite(gv), gv, np.polyval(k, bv))
    mg = np.where(np.isfinite(mg), mg, mv + gv)

    ok = np.isfinite(mass) & np.isfinite(logl) & np.isfinite(rad) & np.isfinite(bprp) & np.isfinite(mg)
    # The table runs hot -> cool, but its masses wiggle (e.g. A3V-A5V) and the
    # coolest M dwarfs turn back in colour. Keep only rows that continue the
    # trend, so mass <-> colour <-> magnitude can be interpolated both ways.
    keep, last = [], None
    for i in np.flatnonzero(ok):
        if last is None or (mass[i] < mass[last] and bprp[i] > bprp[last] and mg[i] > mg[last]):
            keep.append(i)
            last = i
    keep = keep[::-1]
    return [c[keep] for c in (spt, mass, teff, logl, rad, bprp, mg)]


def write_ms_header(table, path):
    spt, mass, teff, logl, rad, bprp, mg = table
    with open(path, "w") as f:
        f.write("#pragma once\n")
        f.write("// Generated by fetch_hr_data.py from Pecaut & Mamajek (2013),\n")
        f.write("// mean dwarf sequence. BP-RP / M_G hotter than B9V are extrapolated.\n\n")
        f.write("struct MSRow { float mass, teff, logL, radius, bpRp, absG; };\n\n")
        f.write("inline constexpr MSRow MS_TABLE[] = {\n")
        for row in zip(spt, mass, teff, logl, rad, bprp, mg):
            s, *v = row
            f.write("    {%7.3ff, %7.1ff, %6.2ff, %7.3ff, %6.3ff, %6.2ff},  // %s\n"
                    % (*v, s))
        f.write("};\n\n")
        f.write("inline constexpr int MS_TABLE_COUNT = sizeof(MS_TABLE) / sizeof(MS_TABLE[0]);\n")
    print(f"Wrote {len(mass)} main-sequence rows to {path}")


# ── Background stars ────────────────────────────────────────────────────────

# Hipparcos stars are not dereddened, so keep only low-extinction sight lines:
# everything inside the Local Bubble, farther out only away from the plane.
NEAR_PLX = 5.0    # mas (200 pc)
FAR_PLX  = 2.5    # mas (400 pc), allowed when |b| > MIN_LAT
MIN_LAT  = 20.0   # deg

def clean_excess(bprp, excess):
    """Riello et al. 2021-style BP/RP flux-excess cut (blends, bad photometry)."""
    return (excess > 1.0 + 0.015 * bprp**2) & (excess < 1.3 + 0.06 * bprp**2)


def build_points(gcns_path, hip_path):
    g = read_csv(gcns_path)
    g_bprp = g["phot_bp_mean_mag"] - g["phot_rp_mean_mag"]
    g_mg = g["phot_g_mean_mag"] + 5.0 * np.log10(g["parallax"] / 100.0)
    keep = ((g["parallax"] / g["parallax_error"] > 10) & (g["ruwe"] < 1.4)
            & (g["bp_snr"] > 20) & (g["rp_snr"] > 20)
            & clean_excess(g_bprp, g["phot_bp_rp_excess_factor"]))
    near = dict(source=g["source_id"][keep], bprp=g_bprp[keep], mg=g_mg[keep])
    print(f"GCNS: {keep.sum()} of {len(g)} stars pass the quality cuts")

    h = read_csv(hip_path)
    h = h[(h["parallax"] > 0) & np.isfinite(h["bp_rp"])]
    h_mg = h["phot_g_mean_mag"] + 5.0 * np.log10(h["parallax"] / 100.0)
    keep = ((h["parallax_over_error"] > 10) & (h["ruwe"] < 1.4)
            & clean_excess(h["bp_rp"], h["phot_bp_rp_excess_factor"])
            & ~np.isin(h["source_id"], near["source"])
            & ((h["parallax"] > NEAR_PLX)
               | ((h["parallax"] > FAR_PLX) & (np.abs(h["b"]) > MIN_LAT))))
    print(f"Hipparcos: {keep.sum()} of {len(h)} stars added")

    bprp = np.concatenate([near["bprp"], h["bp_rp"][keep]])
    mg = np.concatenate([near["mg"], h_mg[keep]])
    return bprp, mg



def write_points(bprp, mg, path):
    with open(path, "w") as f:
        f.write("# Gaia DR3 colour-magnitude diagram: BP-RP, absolute G (see fetch_hr_data.py)\n")
        for c, m in zip(bprp, mg):
            f.write(f"{c:.3f},{m:.2f}\n")
    print(f"Wrote {len(bprp)} stars to {path}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--cache", default=os.path.join(ROOT, ".hr_cache"))
    args = ap.parse_args()
    os.makedirs(args.cache, exist_ok=True)

    mamajek = os.path.join(args.cache, "mamajek.txt")
    if not os.path.exists(mamajek):
        urllib.request.urlretrieve(MAMAJEK_URL, mamajek)
    write_ms_header(mamajek_table(mamajek), os.path.join(ROOT, "include", "main_sequence_table.h"))

    gcns = os.path.join(args.cache, "gcns.csv")
    hip = os.path.join(args.cache, "hip.csv")
    print("Querying the Gaia archive (this can take a while)...")
    fetch_chunked(GCNS_QUERY, gcns, 24)
    fetch_chunked(HIP_QUERY, hip, 16)
    write_points(*build_points(gcns, hip), os.path.join(ROOT, "assets", "hr_data.csv"))


if __name__ == "__main__":
    main()
