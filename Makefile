.PHONY: configure configure-debug configure-release build build-debug build-release run run-debug run-release test test-debug test-release web-build web-serve run-web asan-verbose asan-no-leaks

DEBUG_BUILD_DIR := cmake-build-debug
RELEASE_BUILD_DIR := cmake-build-release

configure: configure-debug configure-release

configure-debug:
	cmake -S . -B $(DEBUG_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

configure-release:
	cmake -S . -B $(RELEASE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build: build-debug build-release

build-debug: configure-debug
	cmake --build $(DEBUG_BUILD_DIR) --target cgame_app

build-release: configure-release
	cmake --build $(RELEASE_BUILD_DIR) --target cgame_app

run: run-debug

run-debug: build-debug
	./$(DEBUG_BUILD_DIR)/cgame_app

run-release: build-release
	./$(RELEASE_BUILD_DIR)/cgame_app

test: test-debug test-release

test-debug: configure-debug
	cmake --build $(DEBUG_BUILD_DIR) --target check

test-release: configure-release
	cmake --build $(RELEASE_BUILD_DIR) --target check

web-build:
	./scripts/build_web.sh

web-serve:
	./scripts/serve_web.sh

run-web: web-build
	./scripts/serve_web.sh

asan-verbose: build-debug
	ASAN_OPTIONS=verbosity=2 ./$(DEBUG_BUILD_DIR)/cgame_app

asan-no-leaks: build-debug
	ASAN_OPTIONS=detect_leaks=0 ./$(DEBUG_BUILD_DIR)/cgame_app
