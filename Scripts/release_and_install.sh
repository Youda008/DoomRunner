#!/bin/bash

# Produces all currently supported release packages for Linux,
# and installs the application into this system.

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_NAME="$(basename "$(dirname "$SCRIPT_DIR")")"

TEMP_DIR="/tmp/$PROJECT_NAME"
trap '[ -d "$TEMP_DIR" ] && rm -r "$TEMP_DIR"' EXIT

"$SCRIPT_DIR/1-build.sh" default dynamic plain release
if [ $? -ne 0 ]; then
	exit 1
fi
source "$TEMP_DIR/build_vars.sh"  # load return values from the 1-build.sh

"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH dynamic_exe
"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH appimage
"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH deb
"$SCRIPT_DIR/3-install.sh" from_build "$BUILD_DIR"
