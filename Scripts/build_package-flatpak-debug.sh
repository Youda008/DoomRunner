SCRIPT_DIR=$(realpath "$(dirname "$0")")

"$SCRIPT_DIR/2-package.sh" "$HOME/Builds/DoomRunner/Linux-x86_64-dynamic-flatpak-release" Linux x86_64 flatpak debug
