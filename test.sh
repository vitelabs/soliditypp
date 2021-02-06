#!/usr/bin/env bash
set -e

ROOTDIR="$(dirname "$0")"

echo "=================================="
echo "*  Automatic Tests               *"
echo "=================================="
./build/test/solpptest

echo
echo "=================================="
echo "*  Interactive Tests             *"
echo "=================================="
./build/test/interactive/itest --testpath="${ROOTDIR}/test"