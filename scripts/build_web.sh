#!/usr/bin/env bash
set -euo pipefail

TARGET="${1:-all_apps}"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"

echo "==> Building '$TARGET' for web..."

emcmake cmake \
  -B "$BUILD_DIR" \
  -S "$REPO_ROOT" \
  -DPLATFORM=Web \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" --target "$TARGET" --parallel

echo "==> Web build complete."
echo "==> Open: $BUILD_DIR/index.html"
