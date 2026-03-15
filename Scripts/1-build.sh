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

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"

# validate the arguments
PACKAGE_TYPE=$1
if [ $PACKAGE_TYPE != deb ] && [ $PACKAGE_TYPE != appimage ] && [ $PACKAGE_TYPE != flatpak ]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: deb, appimage, flatpak"
	exit 1
fi
BUILD_TYPE=$2
if [ $BUILD_TYPE != release ] && [ $BUILD_TYPE != profile ] && [ $BUILD_TYPE != debug ]; then
	echo "Invalid build_type \"$BUILD_TYPE\", possible values: release, profile, debug"
	exit 1
fi

# We cannot build on a shared NTFS drive because then we run into troubles with Linux permissions.
BUILD_DIR="$HOME/Builds/$PROJECT_NAME/Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE"

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

echo "Building the application"
echo " Project dir: $SOURCE_DIR"
echo " Output  dir: $BUILD_DIR"
echo " Build type: $PACKAGE_TYPE $BUILD_TYPE"

[ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# generate the Makefile
echo
COMMAND="$QMAKE \"$SOURCE_DIR/$PROJECT_NAME.pro\" $QMAKE_CONFIG"
echo "$COMMAND" |  eval $SHORTEN_PATHS
eval "$COMMAND" || exit $((100+$?))

# run the Makefile
echo
COMMAND="make -j 10"
echo "$COMMAND"
eval "$COMMAND" || exit $((200+$?))

echo
echo "Build finished successfully."
echo "Output: $BUILD_DIR" | eval $SHORTEN_PATHS
exit 0
