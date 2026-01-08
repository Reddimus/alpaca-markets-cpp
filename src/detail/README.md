# Internal Implementation Details

This directory contains internal-only header utilities used by the Alpaca Markets C++ SDK implementation.

## Files

| File | Description |
|------|-------------|
| `json.hpp` | JSON parsing macros and utilities using RapidJSON |

## Building

This directory is header-only. Building triggers a full library build:

```bash
make build
# or from the repo root:
make build
```

## Make Targets

| Target | Description |
|--------|-------------|
| `build` | Build the full library |
| `clean` | Clean the build directory |
| `lint` | Lint header files |
| `help` | Show available targets |

## Notes

- These headers are **not** part of the public API
- Do not include these headers directly in user code
- Changes here may affect all modules that use JSON parsing
