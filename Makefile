BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release
CMAKE_ARGS ?=
NPROC ?= 4
SOURCES := $(shell find include src tests examples -type f \( -name '*.hpp' -o -name '*.cpp' \))

.PHONY: all configure build debug smart-build models rest stream test lint format clean consumers help
all: build
configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) $(CMAKE_ARGS)
build: configure
	cmake --build $(BUILD_DIR) --parallel $(NPROC)
debug:
	$(MAKE) build BUILD_DIR=build-debug CMAKE_BUILD_TYPE=Debug
smart-build: build
models rest stream: configure
	cmake --build $(BUILD_DIR) --target alpaca_markets_$@ --parallel $(NPROC)
test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure
lint:
	clang-format --dry-run --Werror $(SOURCES)
format:
	clang-format -i $(SOURCES)
consumers:
	bash tools/test_consumers.sh $(CMAKE_ARGS)
clean:
	rm -rf $(BUILD_DIR) build-debug
help:
	@echo 'build debug test lint format consumers clean models rest stream; NPROC=4, CMAKE_ARGS=...'
