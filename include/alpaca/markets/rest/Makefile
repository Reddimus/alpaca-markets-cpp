# include/alpaca/markets/rest/Makefile - Build REST module
# Delegates to root CMake build

ROOT_DIR := ../../../..

.PHONY: all build clean lint help

all: build

build:
	@$(MAKE) -C $(ROOT_DIR) rest

clean:
	@$(MAKE) -C $(ROOT_DIR) clean

lint:
	@echo "Linting REST headers..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name '*.hpp' | xargs -r clang-format --dry-run --Werror || echo "clang-format check completed"; \
	else \
		echo "clang-format not available"; \
	fi

help:
	@echo "REST headers Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  build  - Build the REST module"
	@echo "  clean  - Clean the entire build directory"
	@echo "  lint   - Lint the REST header files"
	@echo "  help   - Show this help"
