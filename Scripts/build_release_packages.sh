#!/bin/bash

# Produces all currently supported release packages for Linux and MacOS.

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_NAME="$(basename "$(dirname "$SCRIPT_DIR")")"

TEMP_DIR="/tmp/$PROJECT_NAME"
trap '[ -d "$TEMP_DIR" ] && rm -r "$TEMP_DIR"' EXIT

# detect the operating system
if [[ -d "/Applications" && -d "/Library" ]]; then
	OS_TYPE=MacOS
elif [[ -d "/usr/bin" ]]; then
	OS_TYPE=Linux
else
	echo "Unrecognized operating system."
	exit 1
fi

if [ $OS_TYPE == Linux ]; then

	"$SCRIPT_DIR/1-build.sh" default dynamic plain release
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from the 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH dynamic_exe
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH appimage
	fi

elif [ $OS_TYPE == MacOS ]; then

	"$SCRIPT_DIR/1-build.sh" arm64 dynamic plain release
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from the 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH dmg
	fi

	"$SCRIPT_DIR/1-build.sh" x86_64 dynamic plain release
	if [ $? -eq 0 ]; then
		source "$TEMP_DIR/build_vars.sh"  # load return values from the 1-build.sh
		"$SCRIPT_DIR/2-package.sh" "$BUILD_DIR" $OS_TYPE $CPU_ARCH dmg
	fi

fi
