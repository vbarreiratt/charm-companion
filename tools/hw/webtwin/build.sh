#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
REPO_ROOT="$(cd ../../.. && pwd)"
mkdir -p dist

SOURCES=$(find "$REPO_ROOT/src" -name '*.cpp' ! -name 'main.cpp')

em++ -std=c++17 -O1 \
  -I "$REPO_ROOT/src" \
  -I "$REPO_ROOT/config" \
  -I "$REPO_ROOT" \
  $SOURCES \
  twin_main.cpp \
  -o dist/twin.js \
  -s EXPORTED_FUNCTIONS='["_twin_buffer","_twin_buffer_size","_twin_width","_twin_height","_twin_select","_twin_touch","_twin_tick","_main"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU16"]' \
  -s EXPORT_ES6=0 \
  -s MODULARIZE=0

echo "built dist/twin.js + dist/twin.wasm"
