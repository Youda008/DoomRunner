#!/bin/bash

# Install the application from the selected build output into this system according to Ubuntu conventions.

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null" EXIT
PACKAGE_TYPE=$1
BUILD_TYPE=$2
BUILD_DIR=Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE

echo "Installing the application from $BUILD_DIR into this system"
echo

echo "Installing binaries"
pushd "$BUILD_DIR" 1>/dev/null
make install
popd 1>/dev/null
echo

copy() { echo "cp $1 $2"; cp $1 $2; }

echo "Installing desktop files"
copy "Install/XDG/DoomRunner.desktop" "/usr/share/applications/DoomRunner.desktop"
copy "Install/XDG/DoomRunner.16x16.png" "/usr/share/icons/hicolor/16x16/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.24x24.png" "/usr/share/icons/hicolor/24x24/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.32x32.png" "/usr/share/icons/hicolor/32x32/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.48x48.png" "/usr/share/icons/hicolor/48x48/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.64x64.png" "/usr/share/icons/hicolor/64x64/apps/DoomRunner.png"
copy "Install/XDG/DoomRunner.128x128.png" "/usr/share/icons/hicolor/128x128/apps/DoomRunner.png"
echo
