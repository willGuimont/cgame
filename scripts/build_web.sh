#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"

echo "==> Building for web..."
emcmake cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -S "$REPO_ROOT"
cmake --build "$BUILD_DIR" --parallel

echo "==> Web build complete at $BUILD_DIR"
