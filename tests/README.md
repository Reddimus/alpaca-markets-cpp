# Tests Directory

This directory contains unit tests for the Alpaca Markets C++ SDK.

## Test Files

| File | Description |
|------|-------------|
| `status_test.cpp` | Tests for Status class and action status conversions |
| `account_test.cpp` | Tests for Account, AccountConfigurations, TradeActivity models |
| `order_test.cpp` | Tests for Order model and enum string conversions |
| `bars_test.cpp` | Tests for Bar and Bars models (v2 format) |
| `quote_test.cpp` | Tests for Quote and LatestQuote models (v2 format) |
| `trade_test.cpp` | Tests for Trade and LatestTrade models (v2 format) |
| `streaming_test.cpp` | Tests for streaming message generation and reply parsing |

## Running Tests

From the project root:

```bash
make test
```

Or run the test executable directly:

```bash
./build/tests/alpaca_markets_tests
```

Run specific tests:

```bash
./build/tests/alpaca_markets_tests --gtest_filter="AccountTest.*"
./build/tests/alpaca_markets_tests --gtest_filter="*FromJSON*"
```

## Test Framework

Tests use Google Test (gtest), which is automatically fetched via CMake FetchContent.

## Integration Tests

Integration tests (requiring real API credentials) are not included in the default test suite. To test against the live API:

1. Set environment variables (see root `README.md` for all supported names):
   ```bash
   export APCA_API_KEY_ID="your-key"
   export APCA_API_SECRET_KEY="your-secret"
   # or:
   export ALPACA_MARKETS_KEY_ID="your-key"
   export ALPACA_MARKETS_SECRET_KEY="your-secret"
   ```

2. Run the example applications:
   ```bash
   ./build/examples/view_account_info
   ./build/examples/check_market_status
   ./build/examples/get_latest_quote AAPL
   ```

## Make Targets

From this directory or the project root:

- `make test` - Build and run all tests
- `make build` - Build tests (without running)
