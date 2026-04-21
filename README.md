# Building N-Body Dynamics on Raspberry Pi

A real-time N-body gravitational simulation built with [raylib](https://www.raylib.com/). This guide covers building on a Raspberry Pi (4 or 5 recommended).

---

## 1. Choose Your Backend

The build system supports three backends, selected via `-DBACKEND=...`:

| Backend | Use when | Input |
|---------|----------|-------|
| **GLFW** | Running on Raspberry Pi OS Desktop, mouse only | Mouse/keyboard |
| **SDL** | You want multi-touch on a touchscreen (under X11/Wayland) | Touch + mouse |
| **DRM** | Headless kiosk mode (no desktop environment) | evdev (touch/mouse) |

---

## 2. Install Dependencies

Update your system first:

```sh
sudo apt update
sudo apt upgrade -y
```

### Common (required for all backends)

```sh
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxi-dev \
    libxcursor-dev \
    libxinerama-dev \
    libasound2-dev
```

### Backend: `SDL` (multi-touch)

```sh
sudo apt install -y libsdl2-dev
```

For native Wayland (recommended on Raspberry Pi OS Bookworm):

```sh
sudo apt install -y libwayland-dev libxkbcommon-dev wayland-protocols
```

### Backend: `DRM` (kiosk, no desktop)

```sh
sudo apt install -y \
    libdrm-dev \
    libgbm-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libinput-dev
```

Your user must be in the `input`, `video`, and `render` groups:

```sh
sudo usermod -aG input,video,render $USER
```

Log out and back in for this to take effect.

---

## 3. Clone & Build

```sh
git clone <your-repo-url> nbody_dynamics
cd nbody_dynamics
mkdir build && cd build
```

Configure with the backend of your choice:

```sh
# Default desktop (GLFW, mouse only)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Multi-touch via SDL2 (Wayland-preferred)
cmake -DCMAKE_BUILD_TYPE=Release -DBACKEND=SDL ..

# Headless kiosk via DRM/KMS (auto-enables GLES 2.0)
cmake -DCMAKE_BUILD_TYPE=Release -DBACKEND=DRM ..
```

Then build (use all cores):

```sh
cmake --build . -j$(nproc)
```

> **Note:** The first build fetches and compiles raylib 5.5 via CMake's `FetchContent`. Expect ~3–5 minutes on a Pi 4.

---

## 4. Run

### GLFW / SDL (under desktop)

```sh
./nbody_dynamics
```

For the SDL backend, a wrapper script is generated that forces native Wayland touch:

```sh
./run.sh
```

### DRM (kiosk, from TTY)

Switch to a text TTY (`Ctrl+Alt+F3`), log in, then:

```sh
cd ~/nbody_dynamics/build
./nbody_dynamics
```

Stop with `Ctrl+C`. Return to the desktop with `Ctrl+Alt+F7` (or `F1` depending on distro).

---

## 5. Command-Line Flags

The application accepts several runtime flags:

| Short | Long | Argument | Description |
|-------|------|----------|-------------|
| `-d` | `--debug` | — | Show FPS counter and debug overlay |
| `-w` | `--width` | `<int>` | Window width in pixels (default: `SCREEN_W`) |
| `-h` | `--height` | `<int>` | Window height in pixels (default: `SCREEN_H`) |
| `-r` | `--root` | `<path>` | Program root directory for asset lookups |
| `-t` | `--timeout` | `<seconds>` | Inactivity timeout before auto-reset (default: `300`, `0` disables) |
| `-q` | `--quiet` | — | Suppress terminal output |
|      | `--help` | — | Print usage information and exit |

### Examples

Run fullscreen-sized with debug overlay:

```sh
./nbody_dynamics --debug --width 1920 --height 1080
```

Kiosk mode with no inactivity reset and silent logging:

```sh
./nbody_dynamics -t 0 -q
```

Point at an alternate asset directory (useful when running outside the build folder):

```sh
./nbody_dynamics --root /opt/nbody_dynamics
```

Combine with the SDL Wayland wrapper:

```sh
./run.sh --debug -t 600
```

> **Note:** On the `DRM` backend, window dimensions are ignored — the simulation always uses the native display resolution.

---

You may also want to tweak the **Troubleshooting** section to mention `--debug` as a first step for diagnosing rendering issues, e.g.:

> **Black screen / nothing appears** — run with `--debug` to confirm the renderer is producing frames, and check the terminal output (drop `--quiet` if set).

## 6. Raspberry Pi Performance Tips

- Enable the **GL (Fake KMS)** or **KMS** driver via `sudo raspi-config` → *Advanced Options* → *GL Driver*. Required for hardware-accelerated OpenGL.
- On a Pi 4, bumping the GPU memory split to 256 MB can help: `sudo raspi-config` → *Performance* → *GPU Memory*.
- Build in **Release** mode (`-DCMAKE_BUILD_TYPE=Release`) — a Debug build will be unusably slow.
- If OpenGL feels sluggish under the desktop, try the `DRM` backend — it bypasses the compositor entirely.

---

## 7. Build Options Summary

| Option | Default | Description |
|--------|---------|-------------|
| `BACKEND` | `GLFW` | One of `GLFW`, `SDL`, `DRM` |
| `USE_GLES` | `OFF` | Force OpenGL ES 2.0 (auto-on for DRM) |
| `USE_WAYLAND` | `ON` | Prefer Wayland over X11/XWayland (SDL only) |

Example — SDL on Wayland with forced GLES:

```sh
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBACKEND=SDL \
      -DUSE_GLES=ON \
      -DUSE_WAYLAND=ON ..
```

---

## Troubleshooting

**`Could NOT find OpenGL`** — install `libgl1-mesa-dev`.

**`fatal error: GLES2/gl2.h: No such file`** — install `libgles2-mesa-dev` (only needed for GLES/DRM).

**DRM backend fails with `Permission denied`** — user is missing from `input`/`video`/`render` groups (see above).

**SDL touch doesn't work under Wayland** — ensure `SDL_VIDEODRIVER=wayland` is set (the generated `run.sh` handles this), and that `libwayland-dev` was installed *before* configuring raylib. If not, delete `build/` and reconfigure.

**Assets not loading** — they should be auto-copied to `build/assets/` on each build. If missing, verify the `assets/` folder exists in the repo root.