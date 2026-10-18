#!/usr/bin/env python3
"""
Generate the Android launcher icon for N-Body Explorer.

A binary star on tilted orbits with fading trails (drawn like the app's
glowing star billboards) over a deep-space background.

Writes into android/app/src/main/res:
  mipmap-<density>/ic_launcher_{background,foreground,monochrome}.png
      adaptive-icon layers (108 dp, API 26+; monochrome = themed icons)
  mipmap-<density>/ic_launcher.png, ic_launcher_round.png
      legacy icons (48 dp, API 24-25)
  mipmap-anydpi-v26/ic_launcher.xml, ic_launcher_round.xml

Usage:  python3 generate_icon.py [--preview out.png]
Requires numpy and Pillow.
"""
import argparse
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

RES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "android", "app", "src", "main", "res")

UNITS = 108                   # adaptive icon layer size in dp
MASTER = 1728                 # master render size (16 px per dp)
PX = MASTER / UNITS
DENSITIES = {"mdpi": 1.0, "hdpi": 1.5, "xhdpi": 2.0, "xxhdpi": 3.0, "xxxhdpi": 4.0}

C = UNITS / 2                 # icon centre (dp)
TILT = math.radians(-24)      # orbit plane tilt on screen

# Heavier warm star on the inner orbit, lighter blue-white star outside
STARS = [
    # (semi-major, semi-minor, phase deg, core radius dp, rgb)
    (11.5, 4.9, 200, 5.4, (1.00, 0.70, 0.28)),
    (27.0, 11.5, 20, 4.0, (0.62, 0.80, 1.00)),
]
TRAIL_ARC = 230               # degrees of trail behind each star


def grid():
    y, x = np.mgrid[0:MASTER, 0:MASTER].astype(np.float32)
    return (x + 0.5) / PX, (y + 0.5) / PX      # in dp


def orbit_point(a, b, deg):
    t = math.radians(deg)
    x, y = a * math.cos(t), b * math.sin(t)
    return (C + x * math.cos(TILT) - y * math.sin(TILT),
            C + x * math.sin(TILT) + y * math.cos(TILT))


def render_background():
    x, y = grid()
    r = np.hypot(x - C * 0.92, y - C * 0.85) / (UNITS * 0.75)
    inner = np.array([0.105, 0.135, 0.29])
    outer = np.array([0.020, 0.026, 0.065])
    t = np.clip(r, 0, 1)[..., None] ** 1.3
    img = inner * (1 - t) + outer * t

    # Faint red nebula like the skybox
    neb = np.exp(-(((x - 80) / 22) ** 2 + ((y - 24) / 14) ** 2))
    img += neb[..., None] * np.array([0.10, 0.015, 0.03])

    # Sprinkle of background stars
    rng = np.random.default_rng(7)
    for _ in range(70):
        sx, sy = rng.uniform(0, UNITS, 2)
        br = rng.uniform(0.15, 0.6) ** 2
        sig = rng.uniform(0.18, 0.35)
        d2 = (x - sx) ** 2 + (y - sy) ** 2
        img += (np.exp(-d2 / (2 * sig * sig)) * br)[..., None] * np.array([0.85, 0.9, 1.0])

    rgb = np.clip(img, 0, 1)
    alpha = np.ones(rgb.shape[:2], np.float32)
    return rgb, alpha


def draw_trails(draw, fade_color):
    steps = 300
    for a, b, phase, _, col in STARS:
        # Whole orbit, faint
        ring = [orbit_point(a, b, i * 360 / steps) for i in range(steps + 1)]
        draw.line([(px * PX, py * PX) for px, py in ring],
                  fill=fade_color(col, 0.16), width=round(0.45 * PX))
        # Trail behind the star, fading out
        pts = [orbit_point(a, b, phase - TRAIL_ARC * i / steps) for i in range(steps + 1)]
        for i in range(steps):
            f = (1 - i / steps) ** 1.3
            p0 = (pts[i][0] * PX, pts[i][1] * PX)
            p1 = (pts[i + 1][0] * PX, pts[i + 1][1] * PX)
            draw.line([p0, p1], fill=fade_color(col, f),
                      width=max(1, round(1.25 * PX * (0.45 + 0.55 * f))))


def render_foreground():
    x, y = grid()
    light = np.zeros((MASTER, MASTER, 3), np.float32)

    # Trails: fading polylines, blurred for a soft glow
    trail = Image.new("RGB", (MASTER, MASTER))
    draw_trails(ImageDraw.Draw(trail),
                lambda col, f: tuple(int(255 * ch * f) for ch in col))
    sharp = np.asarray(trail, np.float32) / 255
    soft = np.asarray(trail.filter(ImageFilter.GaussianBlur(1.8 * PX)), np.float32) / 255
    light += sharp * 0.95 + soft * 1.6

    # Stars: same falloff as the app's glow texture (core + glow + halo)
    for a, b, phase, core, col in STARS:
        sx, sy = orbit_point(a, b, phase)
        d = np.hypot(x - sx, y - sy) / (core * 3.2)
        inten = (np.exp(-d * d * 20) * 2.2 + np.exp(-d * d * 5) * 0.75 +
                 np.exp(-d * d * 1.8) * 0.3)
        hot = np.exp(-d * d * 60)                   # white-hot centre
        light += inten[..., None] * np.array(col) + hot[..., None] * 0.9

    # Additive light → straight RGBA on transparent
    alpha = np.clip(light.max(axis=2), 0, 1)
    rgb = np.where(alpha[..., None] > 1e-4,
                   np.clip(light / np.maximum(alpha[..., None], 1e-4), 0, 1), 0)
    return rgb, alpha


def render_monochrome():
    # Themed icons are tinted flat by the launcher, so glows would turn into
    # blobs: use crisp discs and trails instead.
    mask = Image.new("L", (MASTER, MASTER), 0)
    draw = ImageDraw.Draw(mask)
    draw_trails(draw, lambda col, f: int(255 * min(1.0, f * 1.4)))
    for a, b, phase, core, _ in STARS:
        sx, sy = orbit_point(a, b, phase)
        r = core * 0.95 * PX
        draw.ellipse((sx * PX - r, sy * PX - r, sx * PX + r, sy * PX + r), fill=255)
    alpha = np.asarray(mask, np.float32) / 255
    return np.ones((MASTER, MASTER, 3), np.float32), alpha


def to_image(rgb, alpha):
    arr = np.dstack([rgb, alpha[..., None]]) * 255
    return Image.fromarray(np.round(arr).astype(np.uint8), "RGBA")


def over(bg, fg):
    out = bg.copy()
    out.alpha_composite(fg)
    return out


def legacy(full, shape):
    # The visible part of an adaptive icon is the centre 72 of 108 dp
    m = round(MASTER * 18 / 108)
    crop = full.crop((m, m, MASTER - m, MASTER - m))
    size = crop.size[0]
    mask = Image.new("L", crop.size, 0)
    md = ImageDraw.Draw(mask)
    inset = round(size * 2 / 72)               # legacy icons keep a small margin
    box = (inset, inset, size - inset, size - inset)
    if shape == "round":
        md.ellipse(box, fill=255)
    else:
        md.rounded_rectangle(box, radius=round(size * 0.22), fill=255)
    out = Image.new("RGBA", crop.size, (0, 0, 0, 0))
    out.paste(crop, (0, 0), mask)
    return out


ADAPTIVE_XML = """<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@mipmap/ic_launcher_background" />
    <foreground android:drawable="@mipmap/ic_launcher_foreground" />
    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />
</adaptive-icon>
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", help="also write a composited preview PNG")
    args = ap.parse_args()

    bg = to_image(*render_background())
    fg_rgb, fg_a = render_foreground()
    fg = to_image(fg_rgb, fg_a)
    mono = to_image(*render_monochrome())
    full = over(bg, fg)
    layers = {
        "ic_launcher_background": bg,
        "ic_launcher_foreground": fg,
        "ic_launcher_monochrome": mono,
        "ic_launcher": legacy(full, "square"),
        "ic_launcher_round": legacy(full, "round"),
    }

    for name, scale in DENSITIES.items():
        d = os.path.join(RES_DIR, f"mipmap-{name}")
        os.makedirs(d, exist_ok=True)
        for key, img in layers.items():
            size = round((48 if key in ("ic_launcher", "ic_launcher_round") else UNITS) * scale)
            img.resize((size, size), Image.LANCZOS).save(os.path.join(d, key + ".png"),
                                                         optimize=True)

    d = os.path.join(RES_DIR, "mipmap-anydpi-v26")
    os.makedirs(d, exist_ok=True)
    for name in ("ic_launcher", "ic_launcher_round"):
        with open(os.path.join(d, name + ".xml"), "w") as f:
            f.write(ADAPTIVE_XML)

    if args.preview:
        full.resize((432, 432), Image.LANCZOS).save(args.preview)
    print(f"Icons written to {RES_DIR}")


if __name__ == "__main__":
    main()
