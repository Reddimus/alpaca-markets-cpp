# Development guide

Read README.md for build, usage, supported scope, and runtime policies.
Use `make build test lint consumers` before submitting changes. CI builds
and tests Linux, macOS, and Windows; Markdown lint runs separately.

C++23, namespace alpaca::markets, four-space indentation, 120 columns.
Use explicit local types except structured bindings, lambdas, and
iterators. Keep public compatibility forwarding headers. Glaze is private;
model parsing helpers live in src/detail/json.hpp. REST uses cpp-httplib;
streaming has message helpers but no WebSocket transport. OBJECT modules
are combined into alpaca::markets: propagate their link dependencies to
the final library. Test fractional quantities and full-width integer IDs.

This repository is a fork of archived marpaia/alpaca-trade-api-cpp;
pre-fork tags are historical. Keep comments and documentation concise.
Do not include credentials in fixtures, logs, or committed configuration.
