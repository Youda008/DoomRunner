#!/bin/bash

# Builds all parts of the project using requested build parameters.
#
# Usage: 1-build.sh <package_type> <build_type>
#   package_type - what kind of package is this build meant for, some of them require specific build config
#                    deb = Debian/Ubuntu package that relies on the package maneger to install its dependencies
#                    appimage = self-mounting Linux application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed Linux application bundle containing all dependencies but running with restricted permissions
#                    app = standard MacOS application bundle that contains all dependencies bundled in its directory
#   build_type - QMake build type
#                  release = enables most optimizations, generates debug symbols into a separate file
#                  profile = enables some optimizations, generates debug symbols into a separate file
#                  debug = disables optimizations, generates debug symbols into the executable
#
# NOTE: This script outputs the build directory path in a file /tmp/BUILD_DIR. The caller should delete the file after no longer needed.

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"

# detect the operating system
if [ -d "/Applications" ] && [ -d "/Library" ]; then
	OS_TYPE=MacOS
else
	OS_TYPE=Linux
fi

# validate the arguments
PACKAGE_TYPE=$1
if [ $OS_TYPE == Linux ] && [ $PACKAGE_TYPE != deb ] && [ $PACKAGE_TYPE != appimage ] && [ $PACKAGE_TYPE != flatpak ]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: deb, appimage, flatpak"
	exit 1
elif [ $OS_TYPE == MacOS ] && [ $PACKAGE_TYPE != app ]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: app"
	exit 1
fi
BUILD_TYPE=$2
if [ $BUILD_TYPE != release ] && [ $BUILD_TYPE != profile ] && [ $BUILD_TYPE != debug ]; then
	echo "Invalid build_type \"$BUILD_TYPE\", possible values: release, profile, debug"
	exit 1
fi

# determine the build directory
BUILD_DIR_NAME="Build-$OS_TYPE-$PACKAGE_TYPE-$BUILD_TYPE"
if [[ "$SOURCE_DIR" == "/media/"* ]]; then
	# We cannot build on a shared NTFS drive because then we run into troubles with Linux permissions.
	BUILD_DIR="$HOME/Builds/$PROJECT_NAME/$BUILD_DIR_NAME"
else
	BUILD_DIR="$SOURCE_DIR/$BUILD_DIR_NAME"
fi
# Writing to file is the only way we can return this path to the caller.
echo $BUILD_DIR > /tmp/BUILD_DIR

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
echo " Source dir: $SOURCE_DIR"
echo " Output dir: $BUILD_DIR"
echo " Build type: $PACKAGE_TYPE $BUILD_TYPE"

[ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 2-package.sh modifies the app and `make` doesn't recognize it and regenerate it,
# so we need to delete it manually to get into the expected state.
[ $OS_TYPE == MacOS ] && [ -d "$BUILD_DIR/$PROJECT_NAME.app" ] && rm -r "$BUILD_DIR/$PROJECT_NAME.app"

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
