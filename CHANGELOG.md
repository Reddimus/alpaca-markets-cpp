# Changelog

All notable changes to **alpaca-markets-cpp** are recorded here. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### CI

- First-ever CI workflow added — build + test + lint on Ubuntu 24.04,
  build-only on macos-latest (test step disabled pending investigation
  of linker errors)
  ([`e30d931`](https://github.com/Reddimus/alpaca-markets-cpp/commit/e30d931)).
- `build-windows` job added via vcpkg (parity with the rest of the SDK
  family — see PR#4).
- `.markdownlint-cli2.yaml` config disables `MD013` and other style-
  noise rules; enforces blank-line discipline.
- `MD004` (asterisk style) disabled — accept literal `+` in prose
  continuations
  ([`1016c41`](https://github.com/Reddimus/alpaca-markets-cpp/commit/1016c41)).
- `.github/workflows/release.yml` auto-creates a GitHub Release when
  a `vX.Y.Z` tag is pushed (body sourced from this CHANGELOG via
  `--notes-file` so inline ` `code` ` spans survive)
  ([`5cb3c70`](https://github.com/Reddimus/alpaca-markets-cpp/commit/5cb3c70),
  [`f2ac561`](https://github.com/Reddimus/alpaca-markets-cpp/commit/f2ac561)).
- Tag/CMakeLists `VERSION` drift caught at release time
  ([`7a6944a`](https://github.com/Reddimus/alpaca-markets-cpp/commit/7a6944a)).
- `actions/checkout@v6` upgrade for Node 24 runtime
  ([`c8c3c6a`](https://github.com/Reddimus/alpaca-markets-cpp/commit/c8c3c6a)).
- Drop stale `master` trigger after default-branch migration to `main`
  ([`f72b07c`](https://github.com/Reddimus/alpaca-markets-cpp/commit/f72b07c)).
- `tests/README.md` MD031 fixup so the new lint job passes
  ([`b4234be`](https://github.com/Reddimus/alpaca-markets-cpp/commit/b4234be)).

### Docs

- Add a Contributing section to the README
  ([`6a43c49`](https://github.com/Reddimus/alpaca-markets-cpp/commit/6a43c49)).
- Add CI / release / C++ standard / license badges to the top of the
  README
  ([`963326f`](https://github.com/Reddimus/alpaca-markets-cpp/commit/963326f)).
- Squash double-blank-line inserted by the Contributing section
  ([`acae6fd`](https://github.com/Reddimus/alpaca-markets-cpp/commit/acae6fd)).
- Add `CLAUDE.md` build/architecture/conventions reference
  ([`8d639b9`](https://github.com/Reddimus/alpaca-markets-cpp/commit/8d639b9)).
- Add this `CHANGELOG.md` + the auto-release-on-tag workflow it feeds
  ([`5cb3c70`](https://github.com/Reddimus/alpaca-markets-cpp/commit/5cb3c70)).

### Chore

- Production-hardening `.gitignore` patterns mirrored from the rest
  of the SDK family
  ([`9ccb5cf`](https://github.com/Reddimus/alpaca-markets-cpp/commit/9ccb5cf)).

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
