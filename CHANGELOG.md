# Changelog

All notable changes to **alpaca-markets-cpp** are recorded here. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-05-12

### Changed

- **BREAKING**: C++ standard bumped from C++20 to C++23. Glaze's
  compile-time reflection path requires C++23, and the sibling SDK
  family (kalshi-cpp, polymarket-cpp, open-meteo-cpp, nws-cpp,
  ncei-cpp) all target C++23 — alpaca-markets-cpp was the lone
  C++20 holdout. Consumers must compile with `-std=c++23` (or
  `cxx_std_23`).
- **BREAKING**: JSON library migrated from RapidJSON to
  [Glaze](https://github.com/stephenberry/glaze) v7.6.0 via
  FetchContent. The `Client::*` public API surface is unchanged
  and the existing `Status T::fromJSON(const std::string&)` member
  method contracts on model structs are preserved at the signature
  level — downstream callers (notably the `ibkr-trainer` fallback)
  build without modification. What changes is the dependency chain
  itself: the `-DALPACA_MARKETS_USE_SYSTEM_RAPIDJSON` CMake option
  is gone and FetchContent now pulls Glaze instead of RapidJSON.
  Sites that pre-cached the RapidJSON FetchContent dir, or that
  override `USE_SYSTEM_RAPIDJSON=ON` against a system header, must
  drop the override.

  Benchmark (x86_64-v3, GCC 13.3, -O3 -DNDEBUG, 21.6KB / 200 bars
  across two symbols, 1000 iters):

      RapidJSON v1.1.0 : ~1100 us/op  (pre-migration baseline)
      glaze v7.6.0     :  ~330 us/op  (post-migration)
      speedup          :  ~3.3x

  See `tests/parse_benchmark.cpp` for the regression guard (1500
  us/op cap, ctest --timeout = 30s). (PR #11)

- The streaming `{stream, data}` envelope keeps a `glz::generic`
  outer parse + write_json data-passthrough; typed-union support
  was considered but the `data` sub-object is open-ended and lives
  in downstream consumers' code. Documented in
  `src/stream/streaming.cpp::parseReply`.

### Added

- `tests/glaze_test.cpp` — 11 shape-parity cases covering the
  migration's highest-risk fixtures: Market Data v2 bars-by-symbol
  envelope, `LatestTrade`/`LatestQuote` nested-envelope passthrough,
  `Snapshot`'s 5 sub-models, `OptionContract`'s deliverables array
  + string-typed enum fields, `News` with images, and the stream-
  reply discriminator. All 108 tests (94 pre-existing + 11 new +
  parse benchmark + smoke) pass.
- `tests/parse_benchmark.cpp` — parse-throughput regression guard,
  capped at 1500 us/op with a 30s ctest timeout.

### Removed

- RapidJSON FetchContent and the `ALPACA_MARKETS_USE_SYSTEM_RAPIDJSON`
  CMake option. The `tests/CMakeLists.txt` RapidJSON include dir
  is no longer needed.

## [0.0.2] - 2026-05-10

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

[Unreleased]: https://github.com/Reddimus/alpaca-markets-cpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Reddimus/alpaca-markets-cpp/compare/v0.0.2...v0.1.0
[0.0.2]: https://github.com/Reddimus/alpaca-markets-cpp/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/Reddimus/alpaca-markets-cpp/releases/tag/v0.0.1
