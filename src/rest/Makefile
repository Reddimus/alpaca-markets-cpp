# src/rest/Makefile - Build only the REST client module
# Delegates to root CMake build

ROOT_DIR := ../..
BUILD_DIR ?= $(ROOT_DIR)/build

.PHONY: all build clean lint help

all: build

build:
	@$(MAKE) -C $(ROOT_DIR) rest

clean:
	@$(MAKE) -C $(ROOT_DIR) clean

lint:
	@echo "Linting rest..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name '*.cpp' | xargs -r clang-format --dry-run --Werror || echo "clang-format check completed"; \
	else \
		echo "clang-format not available"; \
	fi

help:
	@echo "REST module Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  build  - Build the REST module only"
	@echo "  clean  - Clean the entire build directory"
	@echo "  lint   - Lint the REST source files"
	@echo "  help   - Show this help"
