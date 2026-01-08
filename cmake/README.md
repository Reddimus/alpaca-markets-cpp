# CMake Directory

This directory is reserved for CMake modules and configuration files.

## Purpose

This directory can be used for:

- Custom CMake find modules (`Find*.cmake`)
- CMake package configuration (`alpaca_markets-config.cmake`)
- CMake utility functions
- Toolchain files for cross-compilation

## Current Status

The project currently uses CMake's built-in FetchContent for dependencies and does not require additional modules in this directory.

## Future Additions

Potential future additions:

- `alpaca_markets-config.cmake.in` - Package config for `find_package()` support
- `FindRapidJSON.cmake` - If switching from FetchContent to system packages
- Cross-compilation toolchain files

## Make Targets

From this directory or the project root:

- `make configure` - Run CMake configuration
- `make build` - Build the project
