#!/bin/bash

# Produces all currently supported release packages for Linux,
# and installs the application into this system.

pushd "$(dirname "$0")" 1>/dev/null
trap "popd 1>/dev/null" EXIT

./1-build.sh appimage release
if [ $? -eq 0 ]; then
	./2-package.sh appimage release
fi

./1-build.sh deb release
if [ $? -eq 0 ]; then
	./2-package.sh deb release
	sudo ./2-deploy.sh deb release
fi
