#!/bin/bash

# Produces all currently supported release packages for Linux

SCRIPT_DIR="$(dirname "$0")"

# detect the operating system
if [ -d "/Applications" ] && [ -d "/Library" ]; then
	OS_TYPE=MacOS
else
	OS_TYPE=Linux
fi

trap "[ -f /tmp/BUILD_DIR ] && rm /tmp/BUILD_DIR" EXIT

BUILD_TYPE=release

if [ $OS_TYPE == Linux ]; then

	PACKAGE_TYPE=deb
	$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
	if [ $? -eq 0 ]; then
		BUILD_DIR=$(cat /tmp/BUILD_DIR)
		$SCRIPT_DIR/2-package.sh $BUILD_DIR $PACKAGE_TYPE
	fi

	PACKAGE_TYPE=appimage
	$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
	if [ $? -eq 0 ]; then
		BUILD_DIR=$(cat /tmp/BUILD_DIR)
		$SCRIPT_DIR/2-package.sh $BUILD_DIR $PACKAGE_TYPE
	fi

elif [ $OS_TYPE == MacOS ]; then

	$SCRIPT_DIR/1-build.sh app $BUILD_TYPE
	if [ $? -eq 0 ]; then
		BUILD_DIR=$(cat /tmp/BUILD_DIR)
		$SCRIPT_DIR/2-package.sh $BUILD_DIR dmg
	fi

fi
