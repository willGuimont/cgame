#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-8000}"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"

if [[ ! -f "$BUILD_DIR/index.html" ]]; then
  echo "Missing $BUILD_DIR/index.html"
  echo "Run ./scripts/build_web.sh first."
  exit 1
fi

echo "==> Serving web build at http://localhost:$PORT/"
python -m http.server "$PORT" --directory "$BUILD_DIR"
