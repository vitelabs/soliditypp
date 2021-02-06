#!/usr/bin/env bash
set -e

ROOTDIR="$(dirname "$0")"
BUILDDIR="${ROOTDIR}/build"

if [[ $# -eq 0 ]]; then
    BUILD_TYPE=Release
else
    BUILD_TYPE="$1"
fi

mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"

cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "${@:2}"
make -j10

# if [[ "${CI}" == "" ]]; then
# 	echo "Installing ..."
# 	sudo make install
# fi