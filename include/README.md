# Include Directory

This directory contains the public headers for the Alpaca Markets C++ SDK.

## Directory Structure

```
include/
└── alpaca/
    └── markets/
        ├── markets.hpp         # Umbrella header - includes everything
        ├── *.hpp               # Forwarding headers for backward compatibility
        ├── models/             # Data transfer objects (DTOs)
        │   ├── status.hpp      # Status/error handling
        │   ├── account.hpp     # Account models
        │   ├── order.hpp       # Order models and enums
        │   ├── position.hpp    # Position model
        │   ├── asset.hpp       # Asset model
        │   ├── clock.hpp       # Market clock model
        │   ├── calendar.hpp    # Calendar model
        │   ├── portfolio.hpp   # Portfolio history model
        │   ├── watchlist.hpp   # Watchlist model
        │   ├── bars.hpp        # OHLCV bar data (v2)
        │   ├── quote.hpp       # Quote data (v2)
        │   └── trade.hpp       # Trade data (v2)
        ├── rest/               # REST API components
        │   ├── client.hpp      # REST API client
        │   └── config.hpp      # Environment configuration
        └── stream/             # Streaming components
            └── streaming.hpp   # WebSocket streaming (placeholder)
```

## Usage

Include the umbrella header to get everything:

```cpp
#include <alpaca/markets/markets.hpp>
```

Or include only what you need:

```cpp
#include <alpaca/markets/client.hpp>
#include <alpaca/markets/config.hpp>
```

## Namespace

All types are in the `alpaca::markets` namespace:

```cpp
alpaca::markets::Client
alpaca::markets::Environment
alpaca::markets::Account
alpaca::markets::Order
// etc.
```

Streaming types are in `alpaca::markets::stream`:

```cpp
alpaca::markets::stream::Handler
alpaca::markets::stream::MessageGenerator
```

## Selective Builds

Each subdirectory under `markets/` has its own `Makefile` for building only that module:

```bash
# Build only models
cd include/alpaca/markets/models && make build

# Build only REST client
cd include/alpaca/markets/rest && make build

# Build only streaming
cd include/alpaca/markets/stream && make build
```

## Make Targets

From this directory or the project root:

- `make build` - Build the library
- `make models` - Build only the models module (from repo root)
- `make rest` - Build only the REST module (from repo root)
- `make stream` - Build only the streaming module (from repo root)
- `make lint` - Check header formatting
