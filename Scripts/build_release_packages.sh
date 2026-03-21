#!/bin/bash

# Produces all currently supported release packages for Linux

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_NAME="$(basename "$SCRIPT_DIR/..")"

# detect the operating system
if [[ -d "/Applications" && -d "/Library" ]]; then
	OS_TYPE=MacOS
else
	OS_TYPE=Linux
fi

TEMP_DIR="/tmp/$PROJECT_NAME"
trap '[ -d "$TEMP_DIR" ] && rm -r "$TEMP_DIR"' EXIT

BUILD_TYPE=release

if [ $OS_TYPE == Linux ]; then

	PACKAGE_TYPE=deb
	"$SCRIPT_DIR/1-build.sh" $PACKAGE_TYPE $BUILD_TYPE
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $PACKAGE_TYPE
	fi

	PACKAGE_TYPE=appimage
	"$SCRIPT_DIR/1-build.sh" $PACKAGE_TYPE $BUILD_TYPE
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $PACKAGE_TYPE
	fi

elif [ $OS_TYPE == MacOS ]; then

	"$SCRIPT_DIR/1-build.sh" app $BUILD_TYPE
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" dmg
	fi

fi
