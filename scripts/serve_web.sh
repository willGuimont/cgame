#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"
PORT="${1:-8000}"

if [[ ! -f "$BUILD_DIR/cgame_app.html" ]]; then
    echo "Missing $BUILD_DIR/cgame_app.html"
    echo "Run ./scripts/build_web.sh first."
    exit 1
fi

echo "==> Serving web build at http://localhost:$PORT/cgame_app.html"
python -m http.server "$PORT" --directory "$BUILD_DIR"
