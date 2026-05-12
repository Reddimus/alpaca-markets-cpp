// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Microbenchmark: parse a representative Market Data API v2 bars-by-symbol
// payload 1k times and report wall-clock. Used as a parse-throughput
// regression guard with `ctest --timeout` plus an absolute upper bound on
// us/op.
//
// Historical baseline (recorded at migration time on x86_64-v3, GCC 13.3,
// -O3 -DNDEBUG, payload=~21KB / 200 bars across two symbols, iters=1000):
//
//     RapidJSON v1.1.0  : ~1100 us/op  (pre-migration baseline, estimated)
//     glaze   v7.6.0    :  ~330 us/op  (post-migration via glz::generic)
//     speedup           :  ~3.3x
//
// The speedup is lower than open-meteo-cpp's 3-4x because Alpaca's
// bars-by-symbol response is a map-of-array-of-OBJECT, and the existing
// string-based dispatch pattern (Bars::fromJSON walks glz::generic,
// write_json each Bar element back to a JSON string, hand it to
// Bar::fromJSON which re-parses) keeps a per-element round-trip. Removing
// that round-trip via Glaze's `convert_from_generic<Bar>` reflected path
// would close the gap to ~4x, but it touches the public
// `Status fromJSON(std::string)` contract, which would require a
// coordinated cross-SDK API change.
//
// The pre-migration RapidJSON path lived in src/models/bars.cpp at the
// time of the Glaze migration commit; it has since been removed along
// with the RapidJSON FetchContent dep. Re-introduce a side-by-side bench
// only if a future regression suspicion warrants it.
//
// Glaze uses a single glz::generic AST pass here, not the reflected
// glz::meta path, because the outer envelope's "bars" map has dynamic
// per-symbol keys (AAPL, GOOG, ...). The reflected path would still win
// on the per-bar inner objects, but the outer dispatch dominates the
// allocation cost on a multi-symbol response.

#include <alpaca/markets/bars.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

// ~7 days of 1-minute bars for two symbols. Mimics a typical
// /v2/stocks/bars?timeframe=1Min&symbols=AAPL,GOOG response.
std::string make_payload() {
    std::string json;
    json.reserve(32 * 1024);
    json += R"({"bars":{"AAPL":[)";

    constexpr int kBarsPerSymbol = 100;
    for (int i = 0; i < kBarsPerSymbol; ++i) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "{\"t\":\"2026-01-01T%02d:%02d:00Z\","
                      "\"o\":%.2f,\"h\":%.2f,\"l\":%.2f,\"c\":%.2f,"
                      "\"v\":%d,\"n\":%d,\"vw\":%.2f}",
                      9 + (i / 60), i % 60,
                      150.0 + 0.5 * std::sin(i * 0.13),
                      151.5 + 0.5 * std::sin(i * 0.13),
                      149.5 + 0.5 * std::sin(i * 0.13),
                      150.5 + 0.5 * std::sin(i * 0.13),
                      1000000 + i * 100,
                      5000 + i,
                      150.5 + 0.5 * std::sin(i * 0.13));
        if (i != 0) {
            json += ',';
        }
        json += buf;
    }
    json += R"(],"GOOG":[)";
    for (int i = 0; i < kBarsPerSymbol; ++i) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "{\"t\":\"2026-01-01T%02d:%02d:00Z\","
                      "\"o\":%.2f,\"h\":%.2f,\"l\":%.2f,\"c\":%.2f,"
                      "\"v\":%d,\"n\":%d,\"vw\":%.2f}",
                      9 + (i / 60), i % 60,
                      2800.0 + 5.0 * std::cos(i * 0.11),
                      2810.0 + 5.0 * std::cos(i * 0.11),
                      2790.0 + 5.0 * std::cos(i * 0.11),
                      2800.5 + 5.0 * std::cos(i * 0.11),
                      500000 + i * 50,
                      2000 + i,
                      2800.5 + 5.0 * std::cos(i * 0.11));
        if (i != 0) {
            json += ',';
        }
        json += buf;
    }
    json += R"(]},"next_page_token":"abc123"})";
    return json;
}

}  // namespace

int main() {
    const std::string payload = make_payload();
    constexpr int kIterations = 1000;

    // Warmup -- let the allocator and CPU settle.
    for (int i = 0; i < 50; ++i) {
        alpaca::markets::Bars warm;
        (void)warm.fromJSON(payload);
    }

    std::chrono::nanoseconds total{0};
    std::size_t checksum = 0;
    for (int i = 0; i < kIterations; ++i) {
        alpaca::markets::Bars bars;
        std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
        alpaca::markets::Status s = bars.fromJSON(payload);
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
        if (!s.ok()) {
            std::fprintf(stderr, "glaze parse failed: %s\n", s.getMessage().c_str());
            return 1;
        }
        total += (t1 - t0);
        // Touch the parsed data so the optimizer can't elide it.
        if (auto it = bars.bars.find("AAPL"); it != bars.bars.end()) {
            checksum += it->second.size();
        }
        if (auto it = bars.bars.find("GOOG"); it != bars.bars.end()) {
            checksum += it->second.size();
        }
    }

    if (checksum != static_cast<std::size_t>(200) * kIterations) {
        std::fprintf(stderr, "checksum mismatch: got=%zu (expected %d)\n", checksum,
                     200 * kIterations);
        return 1;
    }

    const double total_ms = total.count() / 1e6;
    const double us_per_op = (total.count() / 1e3) / kIterations;

    std::printf("parse_benchmark: payload=%zuB iters=%d\n", payload.size(), kIterations);
    std::printf("  glaze: %8.3f ms total  (%8.3f us/op)\n", total_ms, us_per_op);

    // Regression guard: at migration time, Glaze parsed this payload at
    // ~330 us/op on x86_64-v3 (see header comment). Cap at 1500 us/op --
    // that leaves slack for Debug builds, slower CI runners, and cold-cache
    // single-shot runs (the first invocation post-build measured ~720us
    // before the second-and-onward runs settled to ~330us). Future
    // tightening: once the fromJSON contract is updated to take
    // string_view and parse directly via glz::meta<Bar> (no per-element
    // re-serialise), the cap can drop to ~200 us/op.
    constexpr double kMaxUsPerOp = 1500.0;
    if (us_per_op > kMaxUsPerOp) {
        std::fprintf(stderr, "REGRESSION: %.3f us/op exceeds cap of %.0f us/op\n", us_per_op,
                     kMaxUsPerOp);
        return 1;
    }
    return 0;
}
