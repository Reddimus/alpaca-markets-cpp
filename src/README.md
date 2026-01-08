# Source Directory

This directory contains the implementation files for the Alpaca Markets C++ SDK.

## Directory Structure

```
src/
├── detail/              # Internal implementation details
│   └── json.hpp         # JSON parsing macros using RapidJSON
├── models/              # Model implementations
│   ├── status.cpp       # Status class and action status string conversions
│   ├── account.cpp      # Account model JSON deserialization
│   ├── order.cpp        # Order model and enum string conversions
│   ├── position.cpp     # Position model JSON deserialization
│   ├── asset.cpp        # Asset model JSON deserialization
│   ├── clock.cpp        # Clock model JSON deserialization
│   ├── calendar.cpp     # Calendar date model JSON deserialization
│   ├── portfolio.cpp    # Portfolio history JSON deserialization
│   ├── watchlist.cpp    # Watchlist model JSON deserialization
│   ├── bars.cpp         # Bar data (OHLCV) JSON deserialization (v2 format)
│   ├── quote.cpp        # Quote data JSON deserialization (v2 format)
│   └── trade.cpp        # Trade data JSON deserialization (v2 format)
├── rest/                # REST API implementations
│   ├── client.cpp       # REST API client implementation
│   └── config.cpp       # Environment configuration parsing
└── stream/              # Streaming implementations
    └── streaming.cpp    # WebSocket streaming message handling
```

## Selective Builds

Each subdirectory has its own `Makefile` for building only that module:

```bash
# Build only models
cd src/models && make build
# or from repo root:
make models

# Build only REST client
cd src/rest && make build
# or from repo root:
make rest

# Build only streaming
cd src/stream && make build
# or from repo root:
make stream
```

## Dependencies

- **RapidJSON**: JSON parsing (fetched via CMake FetchContent)
- **cpp-httplib**: HTTP/HTTPS client (fetched via CMake FetchContent)
- **OpenSSL**: TLS support for HTTPS

## Make Targets

From this directory or the project root:

- `make build` - Build the library
- `make models` - Build only the models module
- `make rest` - Build only the REST module
- `make stream` - Build only the streaming module
- `make lint` - Run clang-tidy on source files (if available)
