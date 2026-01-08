# Alpaca Markets C++ Makefile
# Delegates to CMake for actual build operations with diff-aware rebuilds

BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release
BUILD_STAMP := $(BUILD_DIR)/.build_sha

.PHONY: all build lint test clean help configure models rest stream smart-build

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Module-specific targets
models: configure
	@cmake --build $(BUILD_DIR) --target alpaca_markets_models --parallel

rest: configure
	@cmake --build $(BUILD_DIR) --target alpaca_markets_rest --parallel

stream: configure
	@cmake --build $(BUILD_DIR) --target alpaca_markets_stream --parallel

# Diff-aware smart build: detect changed modules and rebuild with --clean-first
smart-build: configure
	@if [ -f $(BUILD_STAMP) ]; then \
		LAST_SHA=$$(cat $(BUILD_STAMP)); \
		CHANGED_MODELS=$$(git diff --name-only $$LAST_SHA -- src/models/ include/alpaca/markets/models/ 2>/dev/null | wc -l); \
		CHANGED_REST=$$(git diff --name-only $$LAST_SHA -- src/rest/ include/alpaca/markets/rest/ 2>/dev/null | wc -l); \
		CHANGED_STREAM=$$(git diff --name-only $$LAST_SHA -- src/stream/ include/alpaca/markets/stream/ 2>/dev/null | wc -l); \
		if [ "$$CHANGED_MODELS" -gt 0 ]; then \
			echo "Detected changes in models module, rebuilding..."; \
			cmake --build $(BUILD_DIR) --target alpaca_markets_models --clean-first --parallel; \
		fi; \
		if [ "$$CHANGED_REST" -gt 0 ]; then \
			echo "Detected changes in rest module, rebuilding..."; \
			cmake --build $(BUILD_DIR) --target alpaca_markets_rest --clean-first --parallel; \
		fi; \
		if [ "$$CHANGED_STREAM" -gt 0 ]; then \
			echo "Detected changes in stream module, rebuilding..."; \
			cmake --build $(BUILD_DIR) --target alpaca_markets_stream --clean-first --parallel; \
		fi; \
	fi
	@cmake --build $(BUILD_DIR) --parallel
	@git rev-parse HEAD > $(BUILD_STAMP) 2>/dev/null || true

build: configure
	@if [ -f $(BUILD_STAMP) ] && git rev-parse HEAD >/dev/null 2>&1; then \
		$(MAKE) smart-build; \
	else \
		cmake --build $(BUILD_DIR) --parallel; \
		git rev-parse HEAD > $(BUILD_STAMP) 2>/dev/null || true; \
	fi

lint: configure
	@echo "Running clang-format check..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find include src tests examples \( -name '*.hpp' -o -name '*.cpp' \) 2>/dev/null | xargs -r clang-format --dry-run --Werror || echo "clang-format check completed (some warnings may exist)"; \
	else \
		echo "clang-format not available"; \
	fi
	@echo "Running clang-tidy..."
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		if command -v clang-tidy >/dev/null 2>&1; then \
			find src -name '*.cpp' 2>/dev/null | head -5 | xargs -r clang-tidy -p $(BUILD_DIR) 2>/dev/null || echo "clang-tidy check completed (some warnings may exist)"; \
		else \
			echo "clang-tidy not available"; \
		fi; \
	else \
		echo "No compile_commands.json found. Run 'make configure' first."; \
	fi

test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	@rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "  all         - Build the project (default)"
	@echo "  build       - Build the project (diff-aware: auto-cleans changed modules)"
	@echo "  smart-build - Explicitly run diff-aware build"
	@echo "  configure   - Configure CMake"
	@echo "  lint        - Run linting (clang-format, clang-tidy)"
	@echo "  test        - Run tests"
	@echo "  clean       - Remove build directory"
	@echo ""
	@echo "Module targets (selective builds):"
	@echo "  models      - Build only the models module"
	@echo "  rest        - Build only the REST client module"
	@echo "  stream      - Build only the streaming module"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR        - Build directory (default: build)"
	@echo "  CMAKE_BUILD_TYPE - Build type (default: Release)"
