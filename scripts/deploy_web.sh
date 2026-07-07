#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <target_name>"
    exit 1
fi
TARGET="$1"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"
WORKTREE_DIR="$(mktemp -d)"

if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/${TARGET}.wasm" ]; then
    echo "==> Target web build not found, running build first..."
    "$REPO_ROOT/scripts/build_web.sh" "$TARGET"
fi

cleanup() {
    git -C "$REPO_ROOT" worktree remove --force "$WORKTREE_DIR" 2>/dev/null || true
    rm -rf "$WORKTREE_DIR"
}
trap cleanup EXIT

echo "==> Setting up gh-pages worktree..."
git -C "$REPO_ROOT" fetch origin gh-pages 2>/dev/null || true
if git -C "$REPO_ROOT" show-ref --verify --quiet refs/remotes/origin/gh-pages; then
    git -C "$REPO_ROOT" branch -f gh-pages origin/gh-pages
    git -C "$REPO_ROOT" worktree add "$WORKTREE_DIR" gh-pages
elif git -C "$REPO_ROOT" show-ref --verify --quiet refs/heads/gh-pages; then
    git -C "$REPO_ROOT" worktree add "$WORKTREE_DIR" gh-pages
else
    git -C "$REPO_ROOT" worktree add --orphan -b gh-pages "$WORKTREE_DIR"
fi

echo "==> Staging site..."
find "$WORKTREE_DIR" -mindepth 1 -not -path '*/.git*' -delete 2>/dev/null || true

cp "$BUILD_DIR/${TARGET}.html" "$WORKTREE_DIR/index.html"
cp "$BUILD_DIR/${TARGET}.js" "$WORKTREE_DIR/"
cp "$BUILD_DIR/${TARGET}.wasm" "$WORKTREE_DIR/"
if [ -f "$BUILD_DIR/${TARGET}.data" ]; then
    cp "$BUILD_DIR/${TARGET}.data" "$WORKTREE_DIR/"
fi

echo "==> Committing and pushing..."
git -C "$WORKTREE_DIR" add -A
git -C "$WORKTREE_DIR" commit -m "Deploy web build of ${TARGET} $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
git -C "$WORKTREE_DIR" push origin gh-pages

echo "==> Done. Enable GitHub Pages on the gh-pages branch in repo Settings → Pages."
