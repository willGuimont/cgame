#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <target_name> [port]"
    exit 1
fi
TARGET="$1"
PORT="${2:-8000}"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"

if [[ ! -f "$BUILD_DIR/${TARGET}.html" ]]; then
    echo "Missing $BUILD_DIR/${TARGET}.html"
    echo "Run ./scripts/build_web.sh $TARGET first."
    exit 1
fi

echo "==> Serving web build at http://localhost:$PORT/${TARGET}.html"
python -m http.server "$PORT" --directory "$BUILD_DIR"
