#!/bin/bash

# Installs the application from the selected build output into this system according to Ubuntu conventions.

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"

PACKAGE_TYPE=$1
BUILD_TYPE=$2

# We cannot build on a shared NTFS drive because then we run into troubles with Linux permissions.
BUILD_DIR="$HOME/Builds/$PROJECT_NAME/Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE"

echo "Installing the application from \"$BUILD_DIR\" into this system" | eval $SHORTEN_PATHS
echo

echo "Installing binaries"
pushd "$BUILD_DIR" 1>/dev/null
sudo make install
popd 1>/dev/null
echo

copy() { echo "cp \"{SOURCE_DIR}/$1\" \"$2\""; sudo cp "$SOURCE_DIR/$1" "$2"; }

echo "Installing desktop files"
copy "Install/XDG/$PROJECT_NAME.desktop" "/usr/share/applications/$PROJECT_NAME.desktop"
copy "Install/XDG/$PROJECT_NAME.16x16.png" "/usr/share/icons/hicolor/16x16/apps/$PROJECT_NAME.png"
copy "Install/XDG/$PROJECT_NAME.24x24.png" "/usr/share/icons/hicolor/24x24/apps/$PROJECT_NAME.png"
copy "Install/XDG/$PROJECT_NAME.32x32.png" "/usr/share/icons/hicolor/32x32/apps/$PROJECT_NAME.png"
copy "Install/XDG/$PROJECT_NAME.48x48.png" "/usr/share/icons/hicolor/48x48/apps/$PROJECT_NAME.png"
copy "Install/XDG/$PROJECT_NAME.64x64.png" "/usr/share/icons/hicolor/64x64/apps/$PROJECT_NAME.png"
copy "Install/XDG/$PROJECT_NAME.128x128.png" "/usr/share/icons/hicolor/128x128/apps/$PROJECT_NAME.png"

echo
echo "Done"
