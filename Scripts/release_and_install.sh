#!/bin/bash

# Produces all currently supported release packages for Linux,
# and installs the application into this system.

SCRIPT_DIR="$(dirname "$0")"

trap "[ -f /tmp/BUILD_DIR ] && rm /tmp/BUILD_DIR" EXIT

BUILD_TYPE=release

PACKAGE_TYPE=appimage
$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	BUILD_DIR=$(cat /tmp/BUILD_DIR)
	$SCRIPT_DIR/2-package.sh $BUILD_DIR $PACKAGE_TYPE
fi

PACKAGE_TYPE=deb
$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	BUILD_DIR=$(cat /tmp/BUILD_DIR)
	$SCRIPT_DIR/2-package.sh $BUILD_DIR $PACKAGE_TYPE
	$SCRIPT_DIR/3-install.sh $BUILD_DIR $PACKAGE_TYPE
fi
