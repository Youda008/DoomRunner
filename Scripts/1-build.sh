#!/bin/bash

function exit_with_help() {
	echo 'Builds all parts of the project using requested build parameters.'
	echo ''
	echo 'Usage: 1-build.sh <cpu_arch> <linkage> <build_preset> <build_type>'
	echo '  cpu_arch - the target CPU architecture of the binaries (cross-compilation is currently only supported for MacOS)'
	echo '               default = the architecture of the computer running this script'
	echo '               other common options: i386, x86_64, arm64'
	echo '  linkage - how the library dependencies are linked'
	echo '              static = produces large standalone executable with all libraries integrated into it (currently unsupported)'
	echo '              dynamic = produces small executable, but the libraries have to be installed into the system or bundled with the application'
	echo '  build_preset - build type that defines additional build configuration'
	echo '                   plain = without any additional build config values'
	echo '                   flatpak = special build configuration for producing a Flatpak package'
	echo '  build_type - QMake build type'
	echo '                 release = enables most optimizations, generates debug symbols into a separate file'
	echo '                 profile = enables some optimizations, generates debug symbols into a separate file'
	echo '                 debug = disables optimizations, generates debug symbols into the executable'
	echo ''
	echo 'NOTE: This script outputs some of its variables (like BUILD_DIR) into /tmp/$PROJECT_NAME/build_vars.sh,'
	echo '      which can be loaded using the `source` command. This, however, has to be cleaned up by the caller.'
	exit $1
}

set -o errexit -o nounset -o pipefail

SCRIPT_DIR=$(realpath "$(dirname "$0")")
SOURCE_DIR=$(realpath "$SCRIPT_DIR/..")
SHORTEN_PATHS="python3 '$SCRIPT_DIR/replace.py' '$SOURCE_DIR' '\$SOURCE_DIR' | python3 '$SCRIPT_DIR/replace.py' '$HOME' '\$HOME'"
PROJECT_NAME="$(basename "$SOURCE_DIR")"
pushd "$SOURCE_DIR" 1>/dev/null
trap "popd 1>/dev/null; echo" EXIT

# detect the operating system
if [[ -d "/Applications" && -d "/Library" ]]; then
	OS_TYPE=MacOS
elif [[ -d "/usr/bin" ]]; then
	OS_TYPE=Linux
else
	echo "Unrecognized operating system."
	exit 2
fi

# detect the CPU architecture of this computer
THIS_CPU_ARCH=$(uname -m)

# validate the arguments
if [[ $# -lt 4 ]]; then
	exit_with_help 1
elif [[ $1 == "-h" || $1 == "--help" ]]; then
	exit_with_help 0
fi
CPU_ARCH=$1
if [[ $CPU_ARCH == default ]]; then
	CPU_ARCH=$THIS_CPU_ARCH
elif [[ $OS_TYPE == Linux && $CPU_ARCH != $THIS_CPU_ARCH ]]; then
	echo "Cross-compilation is currently only supported for MacOS, the only available cpu_arch is $THIS_CPU_ARCH."
	exit 1
elif [[ $OS_TYPE == MacOS && $CPU_ARCH != arm64 && $CPU_ARCH != x86_64 ]]; then
	echo "Invalid cpu_arch \"$CPU_ARCH\" for MacOS, possible values: arm64, x86_64."
	exit 1
fi
LINKAGE=$2
if [[ $LINKAGE != static && $LINKAGE != dynamic ]]; then
	echo "Invalid linkage \"$LINKAGE\", possible values: static, dynamic"
	exit 1
elif [[ $LINKAGE == static ]]; then
	echo "Static linking is currently not supported."
	exit 1
fi
BUILD_PRESET=$3
if [[ $OS_TYPE == Linux && $BUILD_PRESET != plain && $BUILD_PRESET != flatpak ]]; then
	echo "Invalid build_preset \"$BUILD_PRESET\", possible values: plain, flatpak"
	exit 1
elif [[ $OS_TYPE == MacOS && $BUILD_PRESET != plain ]]; then
	echo "Invalid build_preset \"$BUILD_PRESET\", possible values: plain"
	exit 1
fi
BUILD_TYPE=$4
if [[ $BUILD_TYPE != release && $BUILD_TYPE != profile && $BUILD_TYPE != debug ]]; then
	echo "Invalid build_type \"$BUILD_TYPE\", possible values: release, profile, debug"
	exit 1
fi

# compose the build directory
BUILD_DIR_NAME="$OS_TYPE-$CPU_ARCH-$LINKAGE-$BUILD_PRESET-$BUILD_TYPE"
if [[ "$SOURCE_DIR" == "/media/"* ]]; then
	# We cannot build on a shared NTFS drive because then we run into troubles with Linux permissions.
	BUILD_DIR="$HOME/Builds/$PROJECT_NAME/$BUILD_DIR_NAME"
else
	BUILD_DIR="$SOURCE_DIR/Builds/$BUILD_DIR_NAME"
fi

# Writing to a file is the only way we can return data to the caller.
TEMP_DIR="/tmp/$PROJECT_NAME"
[ ! -d "$TEMP_DIR" ] && mkdir -p "$TEMP_DIR"
:> "$TEMP_DIR/build_vars.sh"  # clear the file
echo "BUILD_DIR='$BUILD_DIR'"  >> "$TEMP_DIR/build_vars.sh"
echo "OS_TYPE='$OS_TYPE'"      >> "$TEMP_DIR/build_vars.sh"
echo "CPU_ARCH='$CPU_ARCH'"    >> "$TEMP_DIR/build_vars.sh"

# select and verify the Qt build tools
if [[ $OS_TYPE == MacOS ]]; then
	if [[ $CPU_ARCH == arm64 ]]; then
		eval "$(/opt/homebrew/bin/brew shellenv)"
		QMAKE="/opt/homebrew/bin/qmake6"
	elif [[ $CPU_ARCH == x86_64 ]]; then
		eval "$(/usr/local/bin/brew shellenv)"
		QMAKE="/usr/local/bin/qmake6"
	fi
else
	QMAKE=$(which qmake6) || true
fi
if [[ -z "$QMAKE" || ! -f "$QMAKE" ]]; then
	echo
	echo "Qt build tools not found: $QMAKE"
	echo "Build aborted."
	exit 2
fi

# prepare QMake build config
if [ $BUILD_TYPE == debug ]; then    QMAKE_CONFIG="CONFIG+=debug"; fi
if [ $BUILD_TYPE == profile ]; then  QMAKE_CONFIG="CONFIG+=profile CONFIG+=separate_debug_info"; fi
if [ $BUILD_TYPE == release ]; then  QMAKE_CONFIG="CONFIG+=release CONFIG+=separate_debug_info"; fi
if [ $BUILD_PRESET == flatpak ]; then  QMAKE_CONFIG="$QMAKE_CONFIG CONFIG+=flatpak"; fi

echo "Building the application"
echo " Source dir: $SOURCE_DIR"
echo " Output dir: $BUILD_DIR"
echo " Build type: $CPU_ARCH $LINKAGE $BUILD_PRESET $BUILD_TYPE"

[ ! -d "$BUILD_DIR" ] && mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 2-package.sh modifies the app and `make` doesn't recognize it and regenerate it,
# so we need to delete it manually to get into the expected state.
[[ $OS_TYPE == MacOS && -d "$BUILD_DIR/$PROJECT_NAME.app" ]] && rm -r "$BUILD_DIR/$PROJECT_NAME.app"

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
