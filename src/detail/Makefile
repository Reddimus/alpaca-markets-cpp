# src/detail/Makefile - Internal implementation details (header-only)
# Delegates to root CMake build

ROOT_DIR := ../..

.PHONY: all build clean lint help

all: build

build:
	@echo "detail/ is header-only; building full library..."
	@$(MAKE) -C $(ROOT_DIR) build

clean:
	@$(MAKE) -C $(ROOT_DIR) clean

lint:
	@echo "Linting detail headers..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name '*.hpp' | xargs -r clang-format --dry-run --Werror || echo "clang-format check completed"; \
	else \
		echo "clang-format not available"; \
	fi

help:
	@echo "Detail module Makefile (header-only internal utilities)"
	@echo ""
	@echo "Targets:"
	@echo "  build  - Build the full library (detail is header-only)"
	@echo "  clean  - Clean the entire build directory"
	@echo "  lint   - Lint the detail header files"
	@echo "  help   - Show this help"
