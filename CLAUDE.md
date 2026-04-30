# alpaca-markets-cpp Development Guide

## Build & Test

```bash
make build              # Release build (CMake + make)
make debug              # Debug build
make test               # Run unit tests (ctest)
make lint               # Check formatting (clang-format --dry-run)
make format             # Format in place
make clean              # Remove build/

make models             # Build only the models module
make rest               # Build only the REST module
make stream             # Build only the stream module
make smart-build        # Diff-aware rebuild (tracks .build_sha)
```

## Architecture

- **Modular OBJECT libraries**: `alpaca_markets_models`, `alpaca_markets_rest`, `alpaca_markets_stream` — combined into `alpaca_markets` (with `alpaca::markets` ALIAS)
- **C++20** (not C++23): cpp-httplib transport, RapidJSON parsing — pragmatic intentional drift from sibling SDKs that use C++23 + nlohmann/json + libcurl
- **Origin**: forked from archived upstream `marpaia/alpaca-trade-api-cpp`. Inherited 2020 `v0.0.2`/`v0.0.3` tags are not Reddimus releases — Reddimus's first release is `v0.0.1` (2026-01-17).
- **REST coverage**: 40+ endpoints across Trading API v2 + Market Data API v2 (Account, Order, Position, Asset, Bars, Quotes, Options, Corporate Actions, News, Crypto)
- **Stream**: scaffolding only, no WebSocket yet
- **Tests**: 94 unit tests for models. macOS test binary fails to link (`clang++: linker command failed exit 1`) — likely brotli/zlib symbol mismatch or cpp-httplib SSL feature drift between brew HEAD and apt. Linux Tests run; macOS Test step disabled with `if: false`.

## Conventions

- Code style: `.clang-format` (LLVM base, tabs, 100 cols)
- Namespace: `alpaca::markets`
- **No `auto`** for local declarations — explicit types preferred (consistent with sibling SDKs).
- Examples in `examples/`: `view_account_info.cpp`, `check_market_status.cpp`, `get_latest_quote.cpp`. Built as plain binaries (no `example_` prefix), no `run-*` Makefile targets — invoke `./build/examples/<name>` directly.

## CI

GitHub Actions workflow `.github/workflows/ci.yml`: build on Ubuntu 24.04 + macos-latest, lint via clang-format and markdown-lint. macOS Test step disabled (`if: false`) pending linker-error root-cause. Release workflow auto-creates a GitHub Release on `vX.Y.Z` tag push (notes extracted from `CHANGELOG.md`). Default branch is `main` (migrated 2026-04-30 from `master` for sibling-repo consistency).
