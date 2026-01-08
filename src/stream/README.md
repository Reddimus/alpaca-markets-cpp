# Streaming Implementation

This directory contains the streaming/WebSocket implementation for the Alpaca Markets C++ SDK.

## Message Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Handler as Stream Handler
    participant WS as WebSocket Client
    participant API as Alpaca Stream

    Note over WS: ⚠️ WebSocket client<br/>not included

    App->>Handler: Generate auth message
    Handler-->>App: JSON auth payload
    App->>WS: Send auth
    WS->>API: WSS connect + auth
    API-->>WS: {"stream":"authorization","data":{"status":"authorized"}}
    WS-->>App: Auth response
    App->>Handler: Parse auth reply
    Handler-->>App: Authorized ✓

    App->>Handler: Generate subscribe message
    Handler-->>App: JSON subscribe payload
    App->>WS: Send subscribe
    WS->>API: Subscribe trade_updates
    API-->>WS: {"stream":"trade_updates","data":{...}}
    WS-->>App: Trade update
    App->>Handler: Parse trade update
    Handler-->>App: TradeUpdate object
```

## Files

| File            | Description                                                       |
| --------------- | ----------------------------------------------------------------- |
| streaming.cpp   | WebSocket message generation and reply parsing for trading stream |

## Building

Build only the streaming module:

```bash
make build
# or from the repo root:
make stream
```

## Make Targets

| Target | Description                      |
| ------ | -------------------------------- |
| build  | Build only the streaming module  |
| clean  | Clean the build directory        |
| lint   | Lint streaming source files      |
| help   | Show available targets           |

## Current Status

⚠️ **Placeholder Implementation**: The streaming module currently provides message generation and parsing utilities, but does not include a full WebSocket client. A WebSocket library (e.g., Boost.Beast, libwebsockets) would be needed for actual streaming connections.

## Supported Streams

The message generator and parser support:

- `trade_updates` - Order fill/cancel/reject notifications
- `account_updates` - Account balance changes

## Stream Endpoint

Trading stream connects to: `wss://{trading-host}/stream`

- Paper: `wss://paper-api.alpaca.markets/stream`
- Live: `wss://api.alpaca.markets/stream`

## Dependencies

- RapidJSON (JSON parsing/generation)
