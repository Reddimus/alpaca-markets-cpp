# Changelog

All notable changes to **alpaca-markets-cpp** are recorded here. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### CI

- First-ever CI workflow added — build + test + lint on Ubuntu 24.04,
  build-only on macos-latest (test step disabled pending investigation
  of linker errors).
- `.markdownlint-cli2.yaml` config disables `MD013` and other style-
  noise rules; enforces blank-line discipline.
- `.github/workflows/release.yml` auto-creates a GitHub Release when
  a `vX.Y.Z` tag is pushed (body sourced from this CHANGELOG).

### Known issues

- macOS `Test` step is disabled (`if: false`) pending root-cause of:
  - `clang++: error: linker command failed with exit code 1` when
    building the test binary on macos-latest

  Likely brotli/zlib symbol mismatch or cpp-httplib SSL feature flag
  drift between brew and apt. The Build step still verifies macOS
  portability of the library compile path.

## [0.0.1] — 2026-01-17 (pre-release)

### Added

- Initial pre-release: C++ client library for the Alpaca Trading API
  + Market Data API
- Modular libraries: `alpaca_markets_models`, `alpaca_markets_rest`,
  `alpaca_markets_stream` (stream is placeholder)
- 40+ REST endpoints across Trading API v2 and Market Data API v2
- Comprehensive data models (94 unit tests): Account, Order, Position,
  Asset, Bars, Quotes, Options, Corporate Actions, News, Crypto
- Smart diff-aware Makefile (tracks git HEAD, rebuilds only changed
  modules)
- Three integration modes documented: FetchContent, find_package,
  git submodule
- Paper / live env handling via `APCA_API_KEY_ID` /
  `ALPACA_MARKETS_KEY_ID` env vars
- Header-only RapidJSON + cpp-httplib via FetchContent (system
  fallback via `USE_SYSTEM_*` options)
- C++20 (pragmatic; avoids C++23 compiler fragmentation that the
  sibling SDKs require)

### Placeholder

- WebSocket streaming module exists as scaffolding but no
  implementation yet

[Unreleased]: https://github.com/Reddimus/alpaca-markets-cpp/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/Reddimus/alpaca-markets-cpp/releases/tag/v0.0.1
