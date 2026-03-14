#!/bin/bash

SCRIPT_DIR="$(dirname "$0")"

PACKAGE_TYPE=appimage
BUILD_TYPE=release

$SCRIPT_DIR/1-build.sh $PACKAGE_TYPE $BUILD_TYPE || exit

$SCRIPT_DIR/2-package.sh $PACKAGE_TYPE $BUILD_TYPE || exit
