#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/cmake-build-web"
WORKTREE_DIR="$(mktemp -d)"

if [[ ! -f "$BUILD_DIR/index.html" ]]; then
  echo "==> Web build not found, running build first..."
  "$REPO_ROOT/scripts/build_web.sh"
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

cp "$BUILD_DIR"/index.html "$WORKTREE_DIR"/
cp "$BUILD_DIR"/*.html "$WORKTREE_DIR"/
cp "$BUILD_DIR"/*.js "$WORKTREE_DIR"/
cp "$BUILD_DIR"/*.wasm "$WORKTREE_DIR"/
cp "$BUILD_DIR"/*.data "$WORKTREE_DIR"/ 2>/dev/null || true
cp "$BUILD_DIR"/*.worker.js "$WORKTREE_DIR"/ 2>/dev/null || true
cp "$BUILD_DIR"/favicon.* "$WORKTREE_DIR"/ 2>/dev/null || true

echo "==> Committing and pushing..."

git -C "$WORKTREE_DIR" add -A

if git -C "$WORKTREE_DIR" diff --cached --quiet; then
  echo "==> No changes to deploy."
  exit 0
fi

git -C "$WORKTREE_DIR" commit -m "Deploy web build $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
git -C "$WORKTREE_DIR" push origin gh-pages

echo "==> Done."
