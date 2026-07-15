.PHONY: configure configure-debug configure-release configure-web \
        build build-debug build-release \
        run run-debug run-release \
        test test-debug test-release \
        format \
        web-build web-build-app web-serve web-deploy \
        asan-verbose asan-no-leaks

DEBUG_BUILD_DIR := cmake-build-debug
RELEASE_BUILD_DIR := cmake-build-release
WEB_BUILD_DIR := cmake-build-web
FORMAT_FILES := $(shell find src tests -type f \( -name '*.c' -o -name '*.h' \))

APP ?= hexatak

configure: configure-debug configure-release

configure-debug:
	cmake -S . -B $(DEBUG_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

configure-release:
	cmake -S . -B $(RELEASE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

configure-web:
	emcmake cmake -S . -B $(WEB_BUILD_DIR) -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release

build: build-debug build-release

build-debug: configure-debug
	cmake --build $(DEBUG_BUILD_DIR) --target $(APP)

build-release: configure-release
	cmake --build $(RELEASE_BUILD_DIR) --target $(APP)

run: run-debug

run-debug: build-debug
	./$(DEBUG_BUILD_DIR)/$(APP)

run-release: build-release
	./$(RELEASE_BUILD_DIR)/$(APP)

test: test-debug test-release

test-debug: configure-debug
	cmake --build $(DEBUG_BUILD_DIR) --target check

test-release: configure-release
	cmake --build $(RELEASE_BUILD_DIR) --target check

format:
	clang-format -i $(FORMAT_FILES)

web-build: configure-web
	cmake --build $(WEB_BUILD_DIR) --target all_apps --parallel

web-build-app: configure-web
	cmake --build $(WEB_BUILD_DIR) --target $(APP) --parallel

web-serve: web-build
	./scripts/serve_web.sh

web-deploy: web-build
	./scripts/deploy_web.sh

asan-verbose: build-debug
	ASAN_OPTIONS=verbosity=2 ./$(DEBUG_BUILD_DIR)/$(APP)

asan-no-leaks: build-debug
	ASAN_OPTIONS=detect_leaks=0 ./$(DEBUG_BUILD_DIR)/$(APP)
