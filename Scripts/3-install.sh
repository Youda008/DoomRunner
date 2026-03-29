#!/bin/bash

# Installs the application from the selected build output into this system.
#
# Usage: 3-install.sh <install_method> <path>
#   install_method - how and from which source the package is installed
#                      from_build - the package will be installed from the build output, <path> is the path of the build directory
#                      from_deb - the package will be installed from a Debian package, <path> is the path of the package file

set -o errexit -o nounset -o pipefail

pushd "$(dirname "$0")/.." 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT
SOURCE_DIR="$(pwd)"
SCRIPT_DIR="$SOURCE_DIR/Scripts"
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '\$SOURCE_DIR' | python3 '$SCRIPT_DIR/replace.py' '$HOME' '\$HOME'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"

# detect the operating system
if [[ -d "/Applications" && -d "/Library" ]]; then
	OS_TYPE=MacOS
elif [[ -d "/usr/bin" ]]; then
	OS_TYPE=Linux
else
	echo "Unrecognized operating system."
	exit 1
fi

function echo_and_eval() {
	COMMAND="$1"
	echo "$COMMAND" | eval $SHORTEN_PATHS
	COMMAND=$(echo "$COMMAND" | tr -d '\n')  # remove newlines from the command
	eval "$COMMAND"
	return $?
}

function copy() { echo "cp \"\$SOURCE_DIR/$1\" \"$2\""; sudo cp "$SOURCE_DIR/$1" "$2"; }

INSTALL_METHOD="$1"

if [ $INSTALL_METHOD == from_build ]; then

	BUILD_DIR="$2"

	echo "Installing the application from the build output in \"$BUILD_DIR\" into this system" | eval $SHORTEN_PATHS
	echo

	if [ $OS_TYPE == Linux ]; then

		echo "Installing binaries"
		pushd "$BUILD_DIR" 1>/dev/null
		sudo make install
		popd 1>/dev/null
		echo

		echo "Installing desktop files"
		copy "Install/XDG/$PROJECT_NAME.desktop" "/usr/share/applications/$PROJECT_NAME.desktop"
		for SIZE in 16 24 32 48 64 128; do
			copy "Install/XDG/$PROJECT_NAME.${SIZE}x${SIZE}.png" "/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps/$PROJECT_NAME.png"
		done

	elif [ $OS_TYPE == MacOS ]; then

		echo "Installing $PROJECT_NAME.app"
		copy "$BUILD_DIR/$PROJECT_NAME.app" "/Applications"

	fi

elif [ $INSTALL_METHOD == from_deb ]; then

	PACKAGE_PATH="$2"

	echo "Installing the application from the package at \"$PACKAGE_PATH\" into this system" | eval $SHORTEN_PATHS
	echo
	echo_and_eval "sudo apt install \"$PACKAGE_PATH\""

else

	echo "Invalid install_method \"$INSTALL_METHOD\", possible values: from_build, from_deb"
	exit 1

fi

echo
echo "Done"
