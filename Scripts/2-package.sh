#!/bin/bash

# Creates a distributable package from the selected build output.
#
# Usage: 2-package.sh <package_type> <build_type>
#   package_type - what kind of package is this build meant for, some of them require specific build config
#                    deb = Debian/Ubuntu package that relies on the package maneger to install its dependencies
#                    appimage = self-mounting application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed application bundle containing all dependencies but running with restricted permissions
#   build_type - release|profile|debug

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
PROJECT_DIR="$(pwd)"
PACKAGE_TYPE=$1
BUILD_TYPE=$2
BUILD_DIR=Build-Linux-$PACKAGE_TYPE-$BUILD_TYPE

# verify the built executable
EXECUTABLE_PATH="$BUILD_DIR/DoomRunner"
if [ ! -f "$EXECUTABLE_PATH" ]; then
	echo "Build output not found: $EXECUTABLE_PATH"
	echo "Packaging aborted."
	exit 3
fi

# read version number
APP_VERSION=$(eval "echo $(cat version.txt)")
if [ $? -ne 0 ] || [ -z $APP_VERSION ]; then
	echo "Cannot read application version from version.txt"
	echo "Packaging aborted."
	exit 4
fi

# determine package file name
BASE_NAME="DoomRunner-$APP_VERSION-Linux-64bit"

RELEASE_DIR_NAME="Releases"
[ ! -d "$RELEASE_DIR_NAME" ] && mkdir -p $RELEASE_DIR_NAME

if [ $PACKAGE_TYPE == deb ]; then
	# verify the archive tool
	ZIP_TOOL=7z
	if [ -z $(which $ZIP_TOOL) ]; then
		echo "Archive tool not found: $ZIP_TOOL"
		echo "Packaging aborted."
		exit 2
	fi

	ARCHIVE_PATH="$RELEASE_DIR_NAME/$BASE_NAME-dynamic.zip"
	echo "Debian/Ubuntu package not implemented yet"
	echo "Packaging only the executable $EXECUTABLE_PATH into $ARCHIVE_PATH"
	echo
	[ -f "$ARCHIVE_PATH" ] && rm "$ARCHIVE_PATH"
	COMMAND="$ZIP_TOOL a -tzip -mx=7 \"$ARCHIVE_PATH\" \"$EXECUTABLE_PATH\""
	echo "$COMMAND"
	eval "$COMMAND"
	if [ $? -ne 0 ]; then
		echo
		echo "$ZIP_TOOL exited with error: $?"
		echo "Packaging aborted."
		exit 5
	fi
	echo
	echo "Packaging finished successfully."
	echo "Output: $PROJECT_DIR/$ARCHIVE_PATH"
	exit 0
elif [ $PACKAGE_TYPE == appimage ]; then
	# verify the packaging tool
	PKG_TOOL=~/Apps/linuxdeploy-x86_64.AppImage
	if [ ! -f $PKG_TOOL ]; then
		echo "Packaging tool not found: $PKG_TOOL"
		echo "Packaging aborted."
		exit 2
	fi

	APPIMAGE_PATH="$RELEASE_DIR_NAME/$BASE_NAME.AppImage"
	echo "Generating AppImage from $BUILD_DIR into $APPIMAGE_PATH"
	echo
	# AppImage cannot be build on a shared NTFS drive because we would run into troubles with permissions.
	APPIMAGE_BUILD_DIR="$HOME/Builds/DoomRunner"
	# re-create the build dir and start from scratch
	[ -d "$APPIMAGE_BUILD_DIR/AppDir" ] && rm -r "$APPIMAGE_BUILD_DIR/AppDir"
	mkdir -p "$APPIMAGE_BUILD_DIR/AppDir"
	# We need to make it the working directory, because the output is produced there and we can't control it.
	cd "$APPIMAGE_BUILD_DIR"
	COMMAND="$PKG_TOOL
		--executable \"$PROJECT_DIR/$EXECUTABLE_PATH\"
		--desktop-file \"$PROJECT_DIR/Install/XDG/DoomRunner.desktop\"
		--icon-file \"$PROJECT_DIR/Install/XDG/DoomRunner.128x128.png\"
		--icon-filename DoomRunner
		--appdir \"$APPIMAGE_BUILD_DIR/AppDir\"
		--output appimage"
	echo "$COMMAND"
	eval $(echo "$COMMAND" | sed -z 's/\n/ /g')  # remove newlines
	if [ $? -ne 0 ]; then
		echo
		echo "$PKG_TOOL exited with error: $?"
		echo "Packaging aborted."
		exit 5
	fi
	mv Doom_Runner-x86_64.AppImage "$PROJECT_DIR/$APPIMAGE_PATH"
	echo
	echo "Packaging finished successfully."
	echo "Output: $PROJECT_DIR/$APPIMAGE_PATH"
	exit 0
elif [ $PACKAGE_TYPE == flatpak ]; then
	echo "Flatpak package not implemented yet"
	exit 1
else
	echo "Unknown package type: $PACKAGE_TYPE"
	exit 1
fi
