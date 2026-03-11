#!/bin/bash

pushd "$(dirname "$0")" 1>/dev/null
trap "popd 1>/dev/null" EXIT

PACKAGE_TYPE=appimage
BUILD_TYPE=release

./1-build.sh $PACKAGE_TYPE $BUILD_TYPE
[ $? -ne 0 ] && exit 1

./2-package.sh $PACKAGE_TYPE $BUILD_TYPE
[ $? -ne 0 ] && exit 2
