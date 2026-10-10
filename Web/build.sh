#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

: "${FS_WEB_DEPS_PREFIX:?set FS_WEB_DEPS_PREFIX to the prefix holding wasm zlib and libpng}"
: "${FS_PICOJSON_DIR:?set FS_PICOJSON_DIR to the directory holding picojson.h}"

build_dir="${FS_WEB_BUILD_DIR:-$here/build}"
build_type="${FS_WEB_BUILD_TYPE:-Debug}"
jobs="${FS_WEB_JOBS:-2}"

emcmake cmake -S "$here" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DFS_WEB_DEPS_PREFIX="$FS_WEB_DEPS_PREFIX" \
    -DFS_PICOJSON_DIR="$FS_PICOJSON_DIR"

cmake --build "$build_dir" --target FloatingSandboxWebAll -j "$jobs"
