#!/bin/bash

# Creates a distributable package from the selected build output.
#
# Usage: 2-package.sh <package_type> <build_type>
#   package_type - what kind of package is this build meant for, some of them require specific build config
#                    deb = Debian/Ubuntu package that relies on the package maneger to install its dependencies
#                    appimage = self-mounting application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed application bundle containing all dependencies but running with restricted permissions
#   build_type - release|profile|debug

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
EXECUTABLE_PATH="$BUILD_DIR/$PROJECT_NAME"

# read version number
APP_VERSION=$(eval "echo $(cat version.txt)")
if [ $? -ne 0 ] || [ -z $APP_VERSION ]; then
	echo "Cannot read application version from version.txt"
	echo "Packaging aborted."
	exit 3
fi

# compose the package file name
BASE_NAME="$PROJECT_NAME-$APP_VERSION-Linux-64bit"

RELEASE_DIR="$SOURCE_DIR/Releases"
[ ! -d "$RELEASE_DIR" ] && mkdir -p $RELEASE_DIR

if [ $PACKAGE_TYPE == deb ]; then
	# verify the archive tool
	ZIP_TOOL=7z
	if [ -z $(which $ZIP_TOOL) ]; then
		echo "Archive tool not found: $ZIP_TOOL"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME-dynamic.zip"
	echo "Debian/Ubuntu package not implemented yet"
	echo "Packaging only the executable"
	echo " Executable: $EXECUTABLE_PATH" | eval $SHORTEN_PATHS
	echo " Archive: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	echo
	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"
	COMMAND="$ZIP_TOOL a -tzip -mx=7 \"$PACKAGE_PATH\" \"$EXECUTABLE_PATH\""
	echo "$COMMAND" |  eval $SHORTEN_PATHS
	eval "$COMMAND" || exit $((100+$?))
	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0
elif [ $PACKAGE_TYPE == appimage ]; then
	# verify the packaging tool
	PKG_TOOL=~/Apps/linuxdeploy-x86_64.AppImage
	if [ ! -f $PKG_TOOL ]; then
		echo "Packaging tool not found: $PKG_TOOL"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.AppImage"
	echo "Generating AppImage from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Image file: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	echo
	# re-create the build dir and start from scratch
	[ -d "$BUILD_DIR/AppDir" ] && rm -r "$BUILD_DIR/AppDir"
	mkdir -p "$BUILD_DIR/AppDir"
	# We need to make it the working directory, because the output is produced there and we can't control it.
	pushd "$BUILD_DIR" 1>/dev/null
	COMMAND="$PKG_TOOL
		--executable \"$EXECUTABLE_PATH\"
		--desktop-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.desktop\"
		--icon-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.128x128.png\"
		--icon-filename $PROJECT_NAME
		--appdir \"$BUILD_DIR/AppDir\"
		--output appimage"
	echo "$COMMAND" | eval $SHORTEN_PATHS
	COMMAND=$(echo "$COMMAND" | sed -z 's/\n/ /g')  # remove newlines from the command
	eval "$COMMAND" || exit $((100+$?))
	popd 1>/dev/null
	# Some tools will just always do their own thing, no matter what -_-
	mv "$BUILD_DIR/Doom_Runner-x86_64.AppImage" "$PACKAGE_PATH"
	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0
elif [ $PACKAGE_TYPE == flatpak ]; then
	echo "Flatpak package not implemented yet"
	exit 1
fi
