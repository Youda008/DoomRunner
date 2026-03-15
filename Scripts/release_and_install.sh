#!/bin/bash

# Produces all currently supported release packages for Linux,
# and installs the application into this system.

SCRIPT_DIR="$(dirname "$0")"

BUILD_TYPE=release

PACKAGE_TYPE=appimage
$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	$SCRIPT_DIR/2-package.sh $PACKAGE_TYPE $BUILD_TYPE
fi

PACKAGE_TYPE=deb
$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE
if [ $? -eq 0 ]; then
	$SCRIPT_DIR/2-package.sh $PACKAGE_TYPE $BUILD_TYPE
	$SCRIPT_DIR/3-install.sh $PACKAGE_TYPE $BUILD_TYPE
fi
