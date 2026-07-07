#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <target_name>"
    exit 1
fi
TARGET="$1"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"

echo "==> Building target '$TARGET' for web..."
emcmake cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -S "$REPO_ROOT"
cmake --build "$BUILD_DIR" --target "$TARGET" --parallel

echo "==> Web build complete at $BUILD_DIR"
