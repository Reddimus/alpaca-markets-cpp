# Examples Directory

This directory contains example applications demonstrating the Alpaca Markets C++ SDK.

## Examples

### view_account_info

Displays account information including buying power, cash, and equity.

```bash
./build/examples/view_account_info
```

Output:
```
Account ID: abc123
Account Status: ACTIVE
Buying Power: $100000.00
Cash: $50000.00
Equity: $100000.00
Currency: USD
```

### check_market_status

Shows whether the market is currently open and the next open/close times.

```bash
./build/examples/check_market_status
```

Output:
```
Market is currently CLOSED
Current timestamp: 2024-01-15T18:30:00Z
Next open: 2024-01-16T09:30:00-05:00
Next close: 2024-01-16T16:00:00-05:00
```

### get_latest_quote

Fetches the latest quote and trade for a given symbol using Market Data API v2.

```bash
./build/examples/get_latest_quote AAPL
```

Output:
```
Latest quote for AAPL:
  Bid: $185.50 x 300
  Ask: $185.55 x 200
  Timestamp: 2024-01-15T16:00:00Z
Latest trade for AAPL:
  Price: $185.52
  Size: 100
  Timestamp: 2024-01-15T16:00:00Z
```

## Prerequisites

Set environment variables before running (see root `README.md` for all supported names):

```bash
export APCA_API_KEY_ID="your-api-key"
export APCA_API_SECRET_KEY="your-secret-key"
# or:
export ALPACA_MARKETS_KEY_ID="your-api-key"
export ALPACA_MARKETS_SECRET_KEY="your-secret-key"
```

## Building

From the project root:

```bash
make build
```

Examples are built automatically when the main project is built.

## Make Targets

From this directory or the project root:

- `make build` - Build examples
