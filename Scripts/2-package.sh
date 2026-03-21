#!/bin/bash

# Creates a distributable package from the selected build output.
#
# Usage: 2-package.sh <build_dir> <package_type>
#   build_dir - path to the directory where the application has been built
#   package_type - what kind of package should be produced from the build output
#                    deb = Debian/Ubuntu package that relies on the package maneger to install its dependencies
#                    appimage = self-mounting Linux application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed Linux application bundle containing all dependencies but running with restricted permissions
#                    dmg = mountable MacOS image containing all dependencies bundled in a standard application bundle (.app file)

set -o errexit -o nounset -o pipefail

SCRIPT_DIR=$(realpath "$(dirname "$0")")
SOURCE_DIR=$(realpath "$SCRIPT_DIR/..")
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"
pushd "$SOURCE_DIR" 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT

# detect the operating system
if [[ -d "/Applications" && -d "/Library" ]]; then
	OS_TYPE=MacOS
else
	OS_TYPE=Linux
fi

# detect the hardware architecture (TODO)
if [ $OS_TYPE == MacOS ]; then
	HW_ARCH=arm64
else
	HW_ARCH=x86_64
fi

# validate the arguments
PACKAGE_TYPE=$2
if [[ $OS_TYPE == Linux && $PACKAGE_TYPE != deb && $PACKAGE_TYPE != appimage && $PACKAGE_TYPE != flatpak ]]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: deb, appimage, flatpak"
	exit 1
elif [[ $OS_TYPE == MacOS && $PACKAGE_TYPE != dmg ]]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: dmg"
	exit 1
fi

BUILD_DIR="$1"
if [ $OS_TYPE == MacOS ]; then
	APP_PATH="$BUILD_DIR/$PROJECT_NAME.app"
	EXECUTABLE_PATH="$APP_PATH/Contents/MacOS/$PROJECT_NAME"
else
	EXECUTABLE_PATH="$BUILD_DIR/$PROJECT_NAME"
fi
if [ ! -f "$EXECUTABLE_PATH" ]; then
	echo "There is no build output in \"$BUILD_DIR\"" | eval $SHORTEN_PATHS
	echo "Packaging aborted."
	exit 3
fi

RELEASE_DIR="$SOURCE_DIR/Releases"

# read version number
APP_VERSION=$(eval "echo $(cat version.txt)")
if [[ $? -ne 0 || -z $APP_VERSION ]]; then
	echo "Cannot read application version from version.txt"
	echo "Packaging aborted."
	exit 3
fi

# compose the package file name
BASE_NAME="$PROJECT_NAME-$APP_VERSION-$OS_TYPE-$HW_ARCH"

[ ! -d "$RELEASE_DIR" ] && mkdir -p "$RELEASE_DIR"

if [ $PACKAGE_TYPE == deb ]; then

	# verify the archive tool
	ZIP_TOOL=7z
	if [ -z "$(which $ZIP_TOOL)" ]; then
		echo "Archive tool not available: $ZIP_TOOL"
		echo "Please install it first"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME-dynamic.zip"
	echo "Debian/Ubuntu package not implemented yet"
	echo "Packaging only the executable"
	echo " Executable: $EXECUTABLE_PATH" | eval $SHORTEN_PATHS
	echo " Archive: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"

	echo
	COMMAND="$ZIP_TOOL a -tzip -mx=7 \"$PACKAGE_PATH\" \"$EXECUTABLE_PATH\""
	echo "$COMMAND" |  eval $SHORTEN_PATHS
	eval "$COMMAND" || exit $((100+$?))

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0

elif [ $PACKAGE_TYPE == appimage ]; then

	# verify the packaging tool
	PKG_TOOL="$HOME/Apps/linuxdeploy-$HW_ARCH.AppImage"
	if [ ! -f "$PKG_TOOL" ]; then
		echo "Packaging tool not present: $PKG_TOOL"
		echo "Please download it first"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.AppImage"
	echo "Generating AppImage from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Image file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	# re-create the build dir and start from scratch
	[ -d "$BUILD_DIR/AppDir" ] && rm -r "$BUILD_DIR/AppDir"
	mkdir -p "$BUILD_DIR/AppDir"

	# We need to make it the working directory, because the output is produced there and we can't control it.
	pushd "$BUILD_DIR" 1>/dev/null
	echo
	COMMAND="$PKG_TOOL
		--executable \"$EXECUTABLE_PATH\"
		--desktop-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.desktop\"
		--icon-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.128x128.png\"
		--icon-filename $PROJECT_NAME
		--appdir \"$BUILD_DIR/AppDir\"
		--output appimage"
	echo "$COMMAND" | eval $SHORTEN_PATHS
	echo
	COMMAND=$(echo "$COMMAND" | sed -z 's/\n/ /g')  # remove newlines from the command
	eval "$COMMAND" || exit $((100+$?))
	popd 1>/dev/null

	# Some tools will just always do their own thing, no matter what -_-
	mv "$BUILD_DIR/Doom_Runner-$HW_ARCH.AppImage" "$PACKAGE_PATH"

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0

elif [ $PACKAGE_TYPE == flatpak ]; then

	echo "Flatpak package not implemented yet"
	exit 1

elif [ $PACKAGE_TYPE == dmg ]; then

	# verify the packaging tools
	PKG_TOOL=macdeployqt
	if [ -z "$(which $PKG_TOOL)" ]; then
		echo "Packaging tool not available: $PKG_TOOL"
		echo "Please install Qt build tools"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.dmg"
	echo "Generating MacOS application bundle from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Image file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	echo
	COMMAND="macdeployqt \"$APP_PATH\""
	echo "$COMMAND" |  eval $SHORTEN_PATHS
	eval "$COMMAND" || exit $((100+$?))
	echo
	echo "The 'Cannot resolve rpath' errors can be ignored"

	# The `macdeployqt` bundles a lot of unnecessary dependencies.
	# We must get rid of them, otherwise the bundle is ridiculously large.

	echo
	echo "Pruning unnecessary dependencies"

	FRAMEWORK_DIR="$APP_PATH/Contents/Frameworks"
	LIBRARY_DIR=$FRAMEWORK_DIR
	PLUGIN_DIR="$APP_PATH/Contents/PlugIns"

	REQUIRED_FRAMEWORKS=()
	REQUIRED_LIBRARIES=()
	REQUIRED_PLUGINS=()

	# Check for unused plugins
	# This must be figured out manually by running the executable using the following command
	#   DYLD_PRINT_LIBRARIES=1 DoomRunner.app/Contents/MacOS/DoomRunner
	# and inspecting the output.

	REQUIRED_PLUGINS=(
		platforms/libqcocoa
		styles/libqmacstyle
		imageformats/libqico
		tls/libqsecuretransportbackend
		tls/libqopensslbackend
		tls/libqcertonlybackend
	)

	echo
	echo "Deleting unused plugins"
	while IFS= read -r PLUGIN_PATH; do
		PLUGIN_NAME="${PLUGIN_PATH#"$PLUGIN_DIR/"}"
		if [[ ! " ${REQUIRED_PLUGINS[*]} " == *" ${PLUGIN_NAME%.*} "* ]]; then
			echo "Deleting plugin: $PLUGIN_NAME"
			rm "$PLUGIN_PATH"
		else
			echo "Keeping plugin: $PLUGIN_NAME"
			# we also need the dependencies of the plugin
			DEPENDENCIES=$("$SCRIPT_DIR/get_mac_deps.py" --executable-path="$EXECUTABLE_PATH" "$PLUGIN_PATH")
			while IFS= read -r DEP_PATH; do
				#echo "  it depends on: $DEP_PATH"
				REQUIRED_LIBRARIES+=("$DEP_PATH")
			done < <(echo "$DEPENDENCIES")
		fi
	done < <(find "$PLUGIN_DIR" -name "*.dylib")
	find "$PLUGIN_DIR" -type d -empty -delete

	# Check for unused frameworks and dylibs
	# This can be detected automatically using the otool -L command.

	# recursively inspect dependencies and get a list of unique ones
	DEPENDENCIES=$("$SCRIPT_DIR/get_mac_deps.py" "$EXECUTABLE_PATH")
	while read -r DEP_PATH; do
		if [[ $DEP_PATH == *".framework/"* ]]; then
			DEP_PATH=$(dirname "$(dirname "$(dirname "$DEP_PATH")")")
			REQUIRED_FRAMEWORKS+=("$DEP_PATH")
		else
			REQUIRED_LIBRARIES+=("$DEP_PATH")
		fi
	done < <(echo "$DEPENDENCIES")

	echo
	echo "Deleting unused frameworks"
	while IFS= read -r FW_PATH; do
		FW_NAME="${FW_PATH#"$FRAMEWORK_DIR/"}"

		if [[ ! " ${REQUIRED_FRAMEWORKS[*]} " == *" ${FW_PATH} "* ]]; then
			echo "Deleting framework: $FW_NAME"
			rm -r "$FW_PATH"
		else
			echo "Keeping framework: $FW_NAME"
		fi
	done < <(find "$FRAMEWORK_DIR" -name "*.framework")

	echo
	echo "Deleting unused libraries"
	while IFS= read -r LIB_PATH; do
		LIB_NAME="${LIB_PATH#"$LIBRARY_DIR/"}"

		if [[ ! " ${REQUIRED_LIBRARIES[*]} " == *" ${LIB_PATH} "* ]]; then
			echo "Deleting library: $LIB_NAME"
			rm "$LIB_PATH"
		else
			echo "Keeping library: $LIB_NAME"
		fi
	done < <(find "$LIBRARY_DIR" -name "*.dylib")

	# Update the signature

	echo
	echo "Updating signature"
	codesign --force --deep --sign - "$APP_PATH"

	# Package it into a DMG

	echo
	echo "Creating DMG image"
	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"
	# TODO: volname with version
	COMMAND="hdiutil create -volname \"Doom Runner\"
		-srcfolder \"$APP_PATH\"
		-format UDZO
		-imagekey zlib-level=9
		\"$PACKAGE_PATH\""
	echo "$COMMAND" | eval $SHORTEN_PATHS
	COMMAND=$(echo "$COMMAND" | tr -d '\n')  # remove newlines from the command
	eval "$COMMAND" || exit $((100+$?))

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0

fi
