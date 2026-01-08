# Alpaca Markets C++ SDK

A modern C++ SDK for the [Alpaca](https://alpaca.markets/) Trading and Market Data APIs.

## Features

- **Trading API v2**: Full support for account, orders, positions, assets, watchlists, and portfolio history
- **Market Data API v2**: Historical bars, latest trades, and latest quotes for stocks
- **Modern C++20**: Clean API with proper namespacing and modern idioms
- **CMake-based**: Easy integration with CMake-based projects
- **Header-only friendly**: Include only what you need
- **Warning-free builds**: Third-party library warnings silenced; our code compiles with strict warnings (`-Wall -Wextra -Wpedantic`)

## Quick Start

### Prerequisites

- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.25+
- OpenSSL development libraries

### Building

```bash
# Clone the repository
git clone https://github.com/your-org/alpaca-markets-cpp.git
cd alpaca-markets-cpp

# Build
make build

# Run tests
make test
```

### Environment Variables

Set the following environment variables before running:

```bash
# Required
export APCA_API_KEY_ID="your-api-key"
export APCA_API_SECRET_KEY="your-secret-key"

# Optional (defaults to paper trading)
export APCA_API_BASE_URL="https://paper-api.alpaca.markets"  # or https://api.alpaca.markets for live
export APCA_API_DATA_URL="https://data.alpaca.markets"
```

Or use the new naming convention:

```bash
export ALPACA_MARKETS_KEY_ID="your-api-key"
export ALPACA_MARKETS_SECRET_KEY="your-secret-key"
export ALPACA_MARKETS_TRADING_URL="https://paper-api.alpaca.markets"  # base host (no /v2)
export ALPACA_MARKETS_DATA_URL="https://data.alpaca.markets"
```

If you prefer a `.env` file, this repo also supports a more explicit naming scheme (and maps it to the variables above):

```bash
# Paper trading (endpoint + key id)
ALPACA_MARKETS_PAPER_TRADING_API_ENDPOINT="https://paper-api.alpaca.markets"
ALPACA_MARKETS_PAPER_TRADING_API_KEY_ID="your-paper-key-id"

# Live/individual (endpoint + key id + secret)
ALPACA_MARKETS_INDIVIDUAL_API_ENDPOINT="https://api.alpaca.markets"
ALPACA_MARKETS_INDIVIDUAL_API_KEY_ID="your-live-key-id"
ALPACA_MARKETS_INDIVIDUAL_API_SECRET_KEY="your-live-secret"
```

### Example Usage

```cpp
#include <alpaca/markets/markets.hpp>
#include <iostream>

int main() {
    // Parse environment configuration
    auto env = alpaca::markets::Environment();
    if (auto status = env.parse(); !status.ok()) {
        std::cerr << "Error: " << status.getMessage() << std::endl;
        return 1;
    }

    // Create client
    auto client = alpaca::markets::Client(env);

    // Get account information
    auto [status, account] = client.getAccount();
    if (!status.ok()) {
        std::cerr << "Error: " << status.getMessage() << std::endl;
        return 1;
    }

    std::cout << "Account ID: " << account.id << std::endl;
    std::cout << "Buying Power: $" << account.buying_power << std::endl;

    // Get latest quote for a stock
    auto [quote_status, quote] = client.getLatestQuote("AAPL");
    if (quote_status.ok()) {
        std::cout << "AAPL Bid: $" << quote.quote.bid_price << std::endl;
        std::cout << "AAPL Ask: $" << quote.quote.ask_price << std::endl;
    }

    return 0;
}
```

## Project Structure

```text
alpaca-markets-cpp/
├── CMakeLists.txt          # Main CMake configuration
├── Makefile                # Developer workflow targets
├── include/
│   └── alpaca/
│       └── markets/        # Public headers
│           ├── markets.hpp # Umbrella header
│           ├── client.hpp  # REST API client
│           ├── config.hpp  # Environment configuration
│           └── ...         # Model headers
├── src/                    # Implementation files
├── tests/                  # Unit tests
├── examples/               # Example applications
└── cmake/                  # CMake modules (for future use)
```

## API Reference

### Client Methods

#### Account

- `getAccount()` - Get account information
- `getAccountConfigurations()` - Get account settings
- `updateAccountConfigurations()` - Update account settings
- `getAccountActivity()` - Get account activity history

#### Orders

- `getOrders()` - List orders
- `getOrder()` - Get specific order
- `submitOrder()` - Submit new order
- `replaceOrder()` - Replace existing order
- `cancelOrder()` - Cancel order
- `cancelOrders()` - Cancel all orders

#### Positions

- `getPositions()` - List all positions
- `getPosition()` - Get position for symbol
- `closePosition()` - Close position
- `closePositions()` - Close all positions

#### Assets

- `getAssets()` - List assets
- `getAsset()` - Get asset details

#### Market Data (v2)

- `getBars()` - Get historical bar data
- `getLatestTrade()` - Get latest trade for symbol
- `getLatestQuote()` - Get latest quote for symbol

#### Watchlists

- `getWatchlists()` - List watchlists
- `getWatchlist()` - Get watchlist by ID
- `createWatchlist()` - Create new watchlist
- `updateWatchlist()` - Update watchlist
- `deleteWatchlist()` - Delete watchlist

#### Clock & Calendar

- `getClock()` - Get market clock
- `getCalendar()` - Get market calendar

## Make Targets

| Target       | Description                                      |
|--------------|--------------------------------------------------|
| `make build` | Build the project                                |
| `make test`  | Run unit tests                                   |
| `make lint`  | Run linting (requires clang-format, clang-tidy)  |
| `make clean` | Remove build directory                           |
| `make help`  | Show available targets                           |

## Migration from Deprecated Library

If migrating from the `Deprecated/` library:

1. Update includes: `#include "alpaca/alpaca.h"` → `#include <alpaca/markets/markets.hpp>`
2. Update namespace: `alpaca::` → `alpaca::markets::`
3. Update environment parsing (same API, new namespace)
4. Market data methods now use v2 API (response format changed)

### Breaking Changes

- Market data endpoints updated to v2 API
- `getLastTrade()` → `getLatestTrade()` (legacy alias available)
- `getLastQuote()` → `getLatestQuote()` (legacy alias available)
- Bar timestamps now use ISO 8601 strings instead of Unix timestamps
- Quote/Trade responses include additional v2 fields

## License

See [LICENSE](LICENSE) file.

## Contributing

Contributions are welcome! Please see the [Deprecated/CONTRIBUTING.md](Deprecated/CONTRIBUTING.md) for guidelines.
