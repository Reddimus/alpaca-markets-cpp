# Alpaca Markets C++ SDK

Third-party C++23 client for selected Alpaca Trading v2 and Market Data
endpoints. Forked from archived `marpaia/alpaca-trade-api-cpp`.

## Build and test

Use CMake 3.31+, Git, a C++23 compiler (GCC 13+, Clang 19+ with
libstdc++, Clang 18+ with libc++, or current MSVC), OpenSSL 3+, zlib,
and optionally Brotli. Tests and consumer checks also need Python 3.

```sh
# Ubuntu 24.04; install CMake 3.31+ separately if needed.
sudo apt install build-essential git pkg-config libssl-dev zlib1g-dev libbrotli-dev clang-format
# macOS: brew install cmake pkg-config openssl zlib brotli clang-format
make build
make test
make lint
make consumers
```

CMake fetches Glaze 9.0.0, cpp-httplib 0.63.0, and Google Test 1.18.0.
`ALPACA_MARKETS_USE_SYSTEM_HTTPLIB=ON` uses an installed httplib 0.63+
CMake package. `make debug` uses build-debug; add
`CMAKE_ARGS=-DALPACA_MARKETS_ENABLE_SANITIZERS=ON` for ASan/UBSan on
GCC/Clang. `make models`, `make rest`, and `make stream` build individual
object modules. `make smart-build` is an alias for the incremental build.

## Use in CMake

```cmake
include(FetchContent)
FetchContent_Declare(alpaca_cpp
    GIT_REPOSITORY https://github.com/Reddimus/alpaca-markets-cpp.git
    GIT_TAG v0.2.0)
FetchContent_MakeAvailable(alpaca_cpp)
target_link_libraries(your_target PRIVATE alpaca::markets)
```

Tests and examples default to OFF when embedded. For an installed SDK,
run `cmake --install build --prefix /your/prefix`, then use
`find_package(alpaca_markets CONFIG REQUIRED)` and the same target.
OpenSSL, zlib, and enabled Brotli remain consumer dependencies; private
JSON/HTTP headers are not installed.

## Credentials and first request

```sh
export ALPACA_MARKETS_KEY_ID="your-paper-key"
export ALPACA_MARKETS_SECRET_KEY="your-paper-secret"
# Optional; paper trading is the default.
export ALPACA_MARKETS_TRADING_URL="https://paper-api.alpaca.markets"
```

Legacy `APCA_API_KEY_ID`, `APCA_API_SECRET_KEY`, `APCA_API_BASE_URL`,
and `APCA_API_DATA_URL` are also accepted. The ALPACA_MARKETS names take
precedence. `ALPACA_MARKETS_DATA_URL` defaults to `https://data.alpaca.markets`.
REST base URLs contain scheme, host, and optional port; use HTTPS for
credentials. `ALPACA_MARKETS_STREAM_URL` configures message scaffolding.

```cpp
#include <alpaca/markets/markets.hpp>
#include <iostream>

int main() {
    alpaca::markets::Environment environment;
    if (alpaca::markets::Status status = environment.parse(); !status.ok()) {
        std::cerr << status.getMessage() << '\n';
        return 1;
    }
    alpaca::markets::Client client(environment);
    auto [status, account] = client.getAccount();
    if (!status.ok()) {
        std::cerr << status.getMessage() << '\n';
        return 1;
    }
    std::cout << account.id << '\n';
}
```

Configure Environment before constructing Client, which keeps a copy.
REST methods return Status with the model; check Status before using it.
Expected HTTP/API/parse failures use Status; allocation failures can throw.
Each call creates a transport. TLS certificate verification is enabled;
redirects are disabled.
Responses are limited to 64 MiB. Connection/read/write timeouts default to
10/30/30 seconds. Nonpositive values use those defaults; positive values
are capped at one day.

RetryConfig applies only to GET responses with status 429 or 5xx. The
retry count is clamped to 0..100, delays to 0..one day, and invalid
multipliers use 1. Network failures and writes receive one attempt.
Defaults are three additional GET attempts with 100 ms exponential delay,
capped at 5 seconds. Set max_retries to zero to disable retries.

## Coverage and model behavior

See [Client declarations](include/alpaca/markets/rest/client.hpp) for
implemented account, order, position, asset, watchlist, calendar, stock,
crypto, option, news, and corporate-action methods. This is a selected
endpoint client; it does not implement the entire Alpaca platform.
`stream::Handler::run` returns an unsupported status: message generation
and reply parsing exist, but no WebSocket transport is implemented.

Model JSON readers require a complete JSON object. Missing/null fields
use defaults and successful reuse clears old fields. Integer IDs retain
all 64 bits; fractional/out-of-range integer fields fail. Trade sizes,
quote sizes, and bar volumes are double to retain fractional quantities.
This changes their types from releases before 0.2.0. Validate Status after
a failed parse; partial output is not a usable result.

Examples in build/examples read account information, market status, and
latest quotes. Default tests use fixtures and a local HTTP server with
dummy credentials; they do not place orders or require a live account.
See [CONTRIBUTING.md](CONTRIBUTING.md), [CHANGELOG.md](CHANGELOG.md), and
[SECURITY.md](SECURITY.md).
