#!/bin/bash

# Creates a distributable package from the selected build output.
#
# Usage: 2-package.sh <build_dir> <os_type> <cpu_arch> <package_type> <build_type>
#   build_dir - path to the directory where the application has been built
#   os_type - the target operating system of the package (only needed to compose the package file name)
#   cpu_arch - the target CPU architecture of the package (only needed to compose the package file name)
#   package_type - what kind of package should be produced from the build output
#                    dynamic_exe = only zips the dynamically linked executable, dependencies must be installed manually by the user
#                    static_exe = zipped statically linked executable that integrates all dependencies into itself (currently unsupported)
#                    deb = Debian/Ubuntu package that relies on the package manager to install its dependencies
#                    appimage = self-mounting Linux application bundle that contains all dependencies compressed in the executable
#                    flatpak = sandboxed Linux application bundle containing all dependencies but running with restricted permissions
#                    dmg = mountable MacOS image containing all dependencies bundled in a standard application bundle (.app file)
#   build_type - only required for producing package_type=flatpak, see 1-build.sh for description
#
# NOTE: This script outputs some of its variables (like PACKAGE_PATH) into /tmp/$PROJECT_NAME/package_vars.sh,
#       which can be loaded using the 'source' command. This, however, has to be cleaned up by the caller.

set -o errexit -o nounset -o pipefail

SCRIPT_DIR=$(realpath "$(dirname "$0")")
SOURCE_DIR=$(realpath "$SCRIPT_DIR/..")
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '{SOURCE_DIR}'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"
pushd "$SOURCE_DIR" 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT

DESKTOP_APP_NAME="Doom Runner"
APP_NAME_UNDERSCORED="${DESKTOP_APP_NAME// /_}"

function echo_and_eval() {
	COMMAND="$1"
	echo "$COMMAND" | eval $SHORTEN_PATHS
	COMMAND=$(echo "$COMMAND" | tr -d '\n')  # remove newlines from the command
	eval "$COMMAND"
	return $?
}

# validate the arguments
BUILD_DIR="$1"
OS_TYPE=$2
if [[ $OS_TYPE != Linux && $OS_TYPE != MacOS ]]; then
	echo "Unsupported os_type \"$OS_TYPE\", possible values: Linux, MacOS"
	exit 1
fi
CPU_ARCH=$3
if [[ $OS_TYPE == MacOS && $CPU_ARCH != arm64 && $CPU_ARCH != x86_64 ]]; then
	echo "Invalid cpu_arch \"$CPU_ARCH\" for MacOS, possible values: arm64, x86_64."
	exit 1
fi
PACKAGE_TYPE=$4
if [[ $OS_TYPE == Linux && $PACKAGE_TYPE != dynamic_exe && $PACKAGE_TYPE != static_exe && $PACKAGE_TYPE != deb && $PACKAGE_TYPE != appimage && $PACKAGE_TYPE != flatpak ]]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: dynamic_exe, static_exe, deb, appimage, flatpak"
	exit 1
elif [[ $OS_TYPE == MacOS && $PACKAGE_TYPE != dmg ]]; then
	echo "Invalid package_type \"$PACKAGE_TYPE\", possible values: dmg"
	exit 1
fi

# verify the build output and detect build type
if [[ $PACKAGE_TYPE == flatpak ]]; then
	BUILD_TYPE=$5
	if [[ $BUILD_TYPE != release && $BUILD_TYPE != profile && $BUILD_TYPE != debug ]]; then
		echo "Invalid build_type \"$BUILD_TYPE\", possible values: release, profile, debug"
		exit 1
	fi
else
	if [[ $OS_TYPE == MacOS ]]; then
		APP_PATH="$BUILD_DIR/$PROJECT_NAME.app"
		EXECUTABLE_PATH="$APP_PATH/Contents/MacOS/$PROJECT_NAME"
	else
		EXECUTABLE_PATH="$BUILD_DIR/$PROJECT_NAME"
	fi
	MAKEFILE_PATH="$BUILD_DIR/Makefile"
	if [[ ! -f "$EXECUTABLE_PATH" || ! -f "$MAKEFILE_PATH" ]]; then
		echo "There is no build output in \"$BUILD_DIR\"" | eval $SHORTEN_PATHS
		echo "Packaging aborted."
		exit 3
	fi
	if   [[ ! -z $(cat "$MAKEFILE_PATH" | grep "CONFIG+=release") ]]; then
		BUILD_TYPE=release
	elif [[ ! -z $(cat "$MAKEFILE_PATH" | grep "CONFIG+=profile") ]]; then
		BUILD_TYPE=profile
	elif [[ ! -z $(cat "$MAKEFILE_PATH" | grep "CONFIG+=debug") ]]; then
		BUILD_TYPE=debug
	else
		echo "Failed to auto-detect build_type in \"$BUILD_DIR\", please update this code"
		exit 3
	fi
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
BASE_NAME="$PROJECT_NAME-$APP_VERSION-$OS_TYPE-$CPU_ARCH"
[ $BUILD_TYPE != release ] && BASE_NAME="$BASE_NAME-$BUILD_TYPE"

[ ! -d "$RELEASE_DIR" ] && mkdir -p "$RELEASE_DIR"

if [ $PACKAGE_TYPE == dynamic_exe ] || [ $PACKAGE_TYPE == static_exe ]; then

	# verify the archive tool
	ZIP_TOOL=$(which 7z) || true
	if [ -z "$ZIP_TOOL" ]; then
		echo "Archive tool not available: 7z"
		echo "Please install it first"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME-$PACKAGE_TYPE.zip"
	echo "Packaging the executable"
	echo " Executable: $EXECUTABLE_PATH" | eval $SHORTEN_PATHS
	echo " Archive: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"

	echo
	echo_and_eval "$ZIP_TOOL a -tzip -mx=7 \"$PACKAGE_PATH\" \"$EXECUTABLE_PATH\"" || exit $((100+$?))

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS

elif [ $PACKAGE_TYPE == deb ]; then

	# verify the packaging tool
	DPKG_TOOL=$(which dpkg-deb) || true
	if [ -z "$DPKG_TOOL" ]; then
		echo "Packaging tool not available: dpkg-deb"
		echo "Please install 'dpkg' package first"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.deb"
	TEMP_PACKAGE_PATH="$BUILD_DIR/$BASE_NAME.deb"  # otherwise we risk issues with Unix permissions
	echo "Generating Debian package from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Package file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	# cleanup the previous staging files and start from scratch
	STAGING_DIR="$BUILD_DIR/deb-staging"
	[ -d "$STAGING_DIR" ] && rm -r "$STAGING_DIR"
	mkdir -p "$STAGING_DIR/DEBIAN"
	mkdir -p "$STAGING_DIR/usr/bin"
	mkdir -p "$STAGING_DIR/usr/share/applications"
	mkdir -p "$STAGING_DIR/usr/share/metainfo"
	mkdir -p "$STAGING_DIR/usr/share/doc/$PROJECT_NAME"

	# copy executable
	cp "$EXECUTABLE_PATH" "$STAGING_DIR/usr/bin/"

	# copy desktop file and appdata
	cp "$SOURCE_DIR/Install/XDG/$PROJECT_NAME.desktop" "$STAGING_DIR/usr/share/applications/"
	cp "$SOURCE_DIR/Install/XDG/io.github.Youda008.DoomRunner.appdata.xml" "$STAGING_DIR/usr/share/metainfo/"

	# copy icons
	for SIZE in 16 24 32 48 64 128; do
		ICON_DIR="$STAGING_DIR/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps"
		mkdir -p "$ICON_DIR"
		cp "$SOURCE_DIR/Install/XDG/$PROJECT_NAME.${SIZE}x${SIZE}.png" "$ICON_DIR/$PROJECT_NAME.png"
	done

	# copy copyright
	cp "$SOURCE_DIR/Packaging/deb/copyright" "$STAGING_DIR/usr/share/doc/$PROJECT_NAME/"

	# translate the CPU architecture to the debian style
	DEB_ARCH=$CPU_ARCH
	[ $DEB_ARCH == x86_64 ] && DEB_ARCH=amd64

	# generate control file
	sed -e "s|\${VERSION}|$APP_VERSION|" \
	    -e "s|\${ARCH}|$DEB_ARCH|" \
	    < "$SOURCE_DIR/Packaging/deb/control.in" > "$STAGING_DIR/DEBIAN/control"

	# build the package
	echo
	echo_and_eval "$DPKG_TOOL --build \"$STAGING_DIR\" \"$TEMP_PACKAGE_PATH\"" || exit $((100+$?))

	cp "$TEMP_PACKAGE_PATH" "$PACKAGE_PATH"

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS

elif [ $PACKAGE_TYPE == appimage ]; then

	# verify the packaging tool
	DEPLOY_TOOL="$HOME/Apps/linuxdeploy-$CPU_ARCH.AppImage"
	if [ ! -f "$DEPLOY_TOOL" ]; then
		echo "Packaging tool not present: $DEPLOY_TOOL"
		echo "Please download it first"
		echo "Packaging aborted."
		exit 2
	fi

	export QMAKE=$(which qmake6)  # required by the qt plugin of linuxdeploy

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.AppImage"
	echo "Generating AppImage from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Image file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	# Create the package inside the build dir, otherwise we risk running into issues with Unix permissions.
	# We also need to make it the working dir, because the output is produced there and we can't control it.
	pushd "$BUILD_DIR" 1>/dev/null

	# cleanup the previous staging files and start from scratch
	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"
	[ -d "AppDir" ] && rm -r "AppDir"
	mkdir -p "AppDir"

	echo
	COMMAND="$DEPLOY_TOOL
      --executable \"$EXECUTABLE_PATH\"
      --desktop-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.desktop\"
      --icon-file \"$SOURCE_DIR/Install/XDG/$PROJECT_NAME.128x128.png\"
      --icon-filename $PROJECT_NAME
      --appdir \"$BUILD_DIR/AppDir\"
      --plugin qt
      --output appimage
      "
	echo_and_eval "$COMMAND" || exit $((100+$?))

	# some tools will just always do their own thing, no matter what -_-
	TEMP_PACKAGE_NAME="$APP_NAME_UNDERSCORED-$CPU_ARCH.AppImage"
	cp "$TEMP_PACKAGE_NAME" "$PACKAGE_PATH"

	popd 1>/dev/null

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS

elif [ $PACKAGE_TYPE == flatpak ]; then

	# verify the packaging tool
	FLATPAK_BUILDER=$(which flatpak-builder) || true
	if [ -z "$FLATPAK_BUILDER" ]; then
		echo "Packaging tool not available: flatpak-builder"
		echo "Please install 'flatpak-builder' package first"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.flatpak"
	TEMP_PACKAGE_PATH="$BUILD_DIR/$BASE_NAME.flatpak"  # otherwise we risk issues with Unix permissions
	echo "Generating Flatpak package from the source code"
	echo " Package file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	# cleanup the previous staging files and start from scratch
	[ -f "$PACKAGE_PATH" ] && rm "$PACKAGE_PATH"
	#[ -d "$BUILD_DIR" ] && rm -r "$BUILD_DIR"
	mkdir -p "$BUILD_DIR"

	FLATPAK_STATE_DIR="$BUILD_DIR/flatpak-state"
	FLATPAK_BUILD_DIR="$BUILD_DIR/flatpak-build"
	FLATPAK_REPO_DIR="$BUILD_DIR/flatpak-repo"

	sed -e "s|\${SOURCE_DIR}|$SOURCE_DIR|" \
	    -e "s|\${BUILD_TYPE}|$BUILD_TYPE|" \
	    < "$SOURCE_DIR/Packaging/flatpak/manifest.yml.in" > "$BUILD_DIR/manifest.yml"

	echo
	COMMAND="$FLATPAK_BUILDER
      --force-clean
      --install-deps-from=flathub
      --state-dir=\"$FLATPAK_STATE_DIR\"
      --repo=\"$FLATPAK_REPO_DIR\"
      \"$FLATPAK_BUILD_DIR\"
      \"$BUILD_DIR/manifest.yml\"
      "
	echo_and_eval "$COMMAND" || exit $((100+$?))

	echo
	echo_and_eval "flatpak build-bundle \"$FLATPAK_REPO_DIR\" \"$TEMP_PACKAGE_PATH\" io.github.Youda008.DoomRunner" || exit $((200+$?))

	cp "$TEMP_PACKAGE_PATH" "$PACKAGE_PATH"

	# This directory contains a copy of the whole project directory and a new copy is made for every next build!
	# If we don't delete it, we can quickly run out of disk space.
	rm -r "$FLATPAK_STATE_DIR/build"

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS
	exit 0

elif [ $PACKAGE_TYPE == dmg ]; then

	# verify the packaging tools
	if [[ $CPU_ARCH == arm64 ]]; then
		eval "$(/opt/homebrew/bin/brew shellenv)"
		DEPLOY_TOOL="/opt/homebrew/bin/macdeployqt"
	elif [[ $CPU_ARCH == x86_64 ]]; then
		eval "$(/usr/local/bin/brew shellenv)"
		DEPLOY_TOOL="/usr/local/bin/macdeployqt"
	fi
	if [ ! -f "$DEPLOY_TOOL" ]; then
		echo "Packaging tool not available: $DEPLOY_TOOL"
		echo "Please install Qt build tools"
		echo "Packaging aborted."
		exit 2
	fi

	PACKAGE_PATH="$RELEASE_DIR/$BASE_NAME.dmg"
	VOLUME_NAME="$DESKTOP_APP_NAME $APP_VERSION"
	echo "Generating MacOS application bundle from the build output"
	echo " Build dir: $BUILD_DIR" | eval $SHORTEN_PATHS
	echo " Image file: $PACKAGE_PATH" | eval $SHORTEN_PATHS

	echo
	echo_and_eval "$DEPLOY_TOOL \"$APP_PATH\"" || exit $((100+$?))
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
			DEPENDENCIES=$(python3 "$SCRIPT_DIR/get_mac_deps.py" --executable-path="$EXECUTABLE_PATH" "$PLUGIN_PATH")
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
	DEPENDENCIES=$(python3 "$SCRIPT_DIR/get_mac_deps.py" "$EXECUTABLE_PATH")
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

	# create a writable DMG image first, so that we can do some adjustments
	TEMP_DMG_PATH="$BUILD_DIR/$PROJECT_NAME-temp.dmg"
	MOUNTED_DMG_VOLUME="/Volumes/$VOLUME_NAME"
	echo
	COMMAND="hdiutil create
      -volname \"$VOLUME_NAME\"
      -fs HFS+
      -size 200m
      -ov \"$TEMP_DMG_PATH\"
      "
	echo_and_eval "$COMMAND" || exit $((200+$?))

	echo
	echo_and_eval "hdiutil attach \"$TEMP_DMG_PATH\"" || exit $((300+$?))

	sleep 1

	echo
	echo "Copying data into the image..."
	cp -f -R "$APP_PATH" "$MOUNTED_DMG_VOLUME/"
	ln -f -s /Applications "$MOUNTED_DMG_VOLUME/Applications"
	mkdir "$MOUNTED_DMG_VOLUME/.background"
	cp "$SOURCE_DIR/Packaging/dmg/background-960x720.png" "$MOUNTED_DMG_VOLUME/.background/background.png"
	chflags hidden "$MOUNTED_DMG_VOLUME/.background"

	sleep 1

	# Setup the background and layout of the mounted DMG image.
	#cp "$SOURCE_DIR/Packaging/dmg/DS_Store" "$MOUNTED_DMG_VOLUME/.DS_Store"
	osascript "$SCRIPT_DIR/set_finder_layout.applescript" "$VOLUME_NAME" "$PROJECT_NAME"

	echo
	echo "Now you can make manual changes in the mounted image."
	echo "When you're finished, press Enter to continue"
	read

	echo_and_eval "hdiutil detach \"$MOUNTED_DMG_VOLUME\"" || exit $((400+$?))

	sleep 2

	echo
	COMMAND="hdiutil convert
      \"$TEMP_DMG_PATH\"
      -format UDZO
      -imagekey zlib-level=9
      -ov -o \"$PACKAGE_PATH\"
      "
	echo_and_eval "$COMMAND" || exit $((500+$?))

	rm "$TEMP_DMG_PATH"

	echo
	echo "Packaging finished successfully."
	echo "Output: $PACKAGE_PATH" | eval $SHORTEN_PATHS

fi

# Writing to a file is the only way we can return data to the caller.
TEMP_DIR="/tmp/$PROJECT_NAME"
[ ! -d "$TEMP_DIR" ] && mkdir -p "$TEMP_DIR"
:> "$TEMP_DIR/package_vars.sh"  # clear the file
echo "PACKAGE_PATH='$PACKAGE_PATH'"  >> "$TEMP_DIR/package_vars.sh"
echo "RELEASE_DIR='$RELEASE_DIR'"  >> "$TEMP_DIR/package_vars.sh"
exit 0
