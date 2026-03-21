#!/bin/bash

# Produces all currently supported release packages for Linux,
# and installs the application into this system.

SCRIPT_DIR=$(realpath "$(dirname "$0")")

TEMP_DIR="/tmp/$PROJECT_NAME"
trap '[ -d "$TEMP_DIR" ] && rm -r "$TEMP_DIR"' EXIT

BUILD_TYPE=release

PACKAGE_TYPE=appimage
"$SCRIPT_DIR/1-build.sh" $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	source "$TEMP_DIR/build_vars.sh"  # load return values from 1-build.sh
	"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $PACKAGE_TYPE
fi

PACKAGE_TYPE=deb
"$SCRIPT_DIR/1-build.sh" $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	source "$TEMP_DIR/build_vars.sh"  # load return values from 1-build.sh
	"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $PACKAGE_TYPE
	"$SCRIPT_DIR/3-install.sh" "$BUILD_DIR" $PACKAGE_TYPE
fi
