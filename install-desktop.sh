#!/usr/bin/env bash
# Installs an app-menu entry for the desktop build (~/.local/share/applications).
# Re-run after moving the repo; the entry points at this checkout's build/.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN="$ROOT/build/nbody_dynamics"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
APPS="$DATA/applications"
ICONS="$DATA/icons/hicolor/512x512/apps"

if [ ! -x "$BIN" ]; then
    echo "No desktop build at $BIN. Build first: cmake -B build && cmake --build build" >&2
    exit 1
fi

mkdir -p "$APPS" "$ICONS"
install -m 644 "$ROOT/assets/icon.png" "$ICONS/nbody-explorer.png"

# --timeout 0: the kiosk's inactivity quit makes no sense on a desktop
cat > "$APPS/nbody-explorer.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=N-Body Explorer
GenericName=Gravity Simulator
GenericName[de]=Gravitationssimulator
GenericName[es]=Simulador de gravedad
Comment=Place stars and watch them orbit each other
Comment[de]=Platziere Sterne und sieh zu, wie sie einander umkreisen
Comment[es]=Coloca estrellas y observa cómo orbitan entre sí
Exec="$BIN" --root "$ROOT/build" --timeout 0
Path=$ROOT/build
Icon=nbody-explorer
Terminal=false
# GLFW runs under XWayland here and uses the window title as WM_CLASS
StartupWMClass=N-Body Dynamics — Astrophysics Explorer
Categories=Education;Astronomy;Physics;
Keywords=gravity;stars;orbit;n-body;astronomy;simulation;
DESKTOP

update-desktop-database "$APPS" 2>/dev/null || true
gtk-update-icon-cache -q "$DATA/icons/hicolor" 2>/dev/null || true
echo "Installed $APPS/nbody-explorer.desktop"
