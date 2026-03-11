#!/bin/bash

# Builds all parts of the project using requested build parameters.
#
# Usage: 1-build.sh <package_type> <build_type>
#   package_type - what kind of package is this build meant for, some of them require specific build config
#                    deb = Debian/Ubuntu package that relies on the package maneger to install its dependencies
#                    appimage = self-mounting application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed application bundle containing all dependencies but running with restricted permissions
#   build_type - QMake build type
#                  release = enables most optimizations, generates debug symbols into a separate file
#                  profile = enables some optimizations, generates debug symbols into a separate file
#                  debug = disables optimizations, generates debug symbols into the executable

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
PROJECT_DIR="$(pwd)"
PACKAGE_TYPE=$1
BUILD_TYPE=$2
BUILD_DIR=Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE

echo "Building the application"
echo "Source dir: $PROJECT_DIR"
echo "Output dir: $PROJECT_DIR/$BUILD_DIR"
echo "Build type: $PACKAGE_TYPE $BUILD_TYPE"

# select and verify the Qt build tools
QMAKE=qmake6
if [ -z $(which $QMAKE) ]; then
	echo
	echo "Qt build tools not found: $QMAKE"
	echo "Build aborted."
	exit 2
fi

# prepare QMake build config
if [ $BUILD_TYPE == debug ]; then    QMAKE_CONFIG="CONFIG+=debug"; fi
if [ $BUILD_TYPE == profile ]; then  QMAKE_CONFIG="CONFIG+=profile CONFIG+=separate_debug_info"; fi
if [ $BUILD_TYPE == release ]; then  QMAKE_CONFIG="CONFIG+=release CONFIG+=separate_debug_info"; fi
if [ $PACKAGE_TYPE == flatpak ]; then  QMAKE_CONFIG="$QMAKE_CONFIG CONFIG+=flatpak"; fi

[ ! -d "$BUILD_DIR" ] && mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# generate the Makefile
echo
COMMAND="$QMAKE \"$PROJECT_DIR/DoomRunner.pro\" $QMAKE_CONFIG"
echo "$COMMAND"
eval "$COMMAND"
if [ $? -ne 0 ]; then
	echo
	echo "QMake exited with error: $?"
	echo "Build failed."
	exit 5
fi

# run the Makefile
echo
make -j 10
if [ $? -ne 0 ]; then
	echo
	echo "Make exited with error: $?"
	echo "Build failed."
	exit 6
fi

echo
echo "Build finished successfully."
echo "Output: $PROJECT_DIR/$BUILD_DIR"
exit 0
