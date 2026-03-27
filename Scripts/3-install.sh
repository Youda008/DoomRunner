#!/bin/bash

# Installs the application from the selected build output into this system according to Ubuntu conventions.

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"

BUILD_DIR="$1"

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
for SIZE in 16 24 32 48 64 128; do
	copy "Install/XDG/$PROJECT_NAME.${SIZE}x${SIZE}.png" "/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps/$PROJECT_NAME.png"
done

echo
echo "Done"
