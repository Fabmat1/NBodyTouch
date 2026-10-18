#!/usr/bin/env bash
# Builds a signed release APK of N-Body Explorer and installs it as an update.
#
#   android/release.sh            # installs on the single connected phone, if any
#   android/release.sh <serial>   # installs on that device
#
# versionCode = commit count, so the tree must be committed: every release
# then has a higher versionCode than the one before and installs in place.
set -euo pipefail
cd "$(dirname "$0")"

export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"
ADB="$ANDROID_HOME/platform-tools/adb"

if [ -n "$(git status --porcelain)" ]; then
    echo "Working tree not clean: commit first." >&2
    exit 1
fi
if ! grep -q '^NBODY_STORE_FILE=' "$HOME/.gradle/gradle.properties" 2>/dev/null; then
    echo "NBODY_* signing entries missing from ~/.gradle/gradle.properties." >&2
    exit 1
fi

./gradlew --console=plain assembleRelease
CODE=$(git rev-list --count HEAD)
APK=app/build/outputs/apk/release/NBodyTouch-1.$CODE.apk
cp app/build/outputs/apk/release/app-release.apk "$APK"
git tag -f "release-$CODE" >/dev/null
echo "Built $(realpath "$APK"), tagged release-$CODE."

PHONE="${1:-}"
if [ -z "$PHONE" ]; then
    PHONES=$("$ADB" devices | awk 'NR > 1 && $2 == "device" && $1 !~ /^emulator-/ {print $1}')
    if [ "$(echo "$PHONES" | grep -c .)" -ne 1 ]; then
        echo "No single phone connected; install the APK above manually."
        exit 0
    fi
    PHONE="$PHONES"
fi

# -r = update in place
if ! "$ADB" -s "$PHONE" install -r "$APK"; then
    cat >&2 <<'EOF'

INSTALL FAILED. INSTALL_FAILED_UPDATE_INCOMPATIBLE means the installed app was
signed with another key (e.g. an old debug build from before the release key
existed). Uninstall that old copy once; every build from now on updates in place.
EOF
    exit 1
fi
echo "Installed release-$CODE on $PHONE."
