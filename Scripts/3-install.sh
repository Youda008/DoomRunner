#!/bin/bash

# Installs the application from the selected build output into this system according to Ubuntu conventions.

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"

PACKAGE_TYPE=$1
BUILD_TYPE=$2

# We cannot build on a shared NTFS drive because then we run into troubles with Linux permissions.
BUILD_DIR="$HOME/Builds/DoomRunner/Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE"

echo "Installing the application from \"$BUILD_DIR\" into this system" | eval $SHORTEN_PATHS
echo

echo "Installing binaries"
pushd "$BUILD_DIR" 1>/dev/null
sudo make install
popd 1>/dev/null
echo

copy() { echo "cp \"{SOURCE_DIR}/$1\" \"$2\""; sudo cp "$SOURCE_DIR/$1" "$2"; }

echo "Installing desktop files"
copy "Install/XDG/DoomRunner.desktop" "/usr/share/applications/DoomRunner.desktop"
copy "Install/XDG/DoomRunner.16x16.png" "/usr/share/icons/hicolor/16x16/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.24x24.png" "/usr/share/icons/hicolor/24x24/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.32x32.png" "/usr/share/icons/hicolor/32x32/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.48x48.png" "/usr/share/icons/hicolor/48x48/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.64x64.png" "/usr/share/icons/hicolor/64x64/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.128x128.png" "/usr/share/icons/hicolor/128x128/apps/DoomRunner.png"

echo
echo "Done"
