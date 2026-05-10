# Security Policy

`alpaca-markets-cpp` is a third-party C++ client for the Alpaca Trading
API and Market Data API. It signs every request with an account-bound
API key + secret pair (`APCA_API_KEY_ID` / `APCA_API_SECRET_KEY` or the
`ALPACA_MARKETS_*` aliases), so a vulnerability that mishandles
credentials or leaks request material could put live trading capital
at risk. This file is the canonical contact path for reporting one.

## Supported Versions

Security fixes are made on the latest published `vX.Y.Z` tag. Older
tags are not back-patched — bump your `FetchContent_Declare(... GIT_TAG ...)`
pin or your `find_package(alpaca-markets-cpp X.Y.Z REQUIRED)` constraint
to the latest minor on the same major as part of the upgrade.

The pre-Reddimus `v0.0.2` / `v0.0.3` tags inherited from upstream
`marpaia/alpaca-trade-api-cpp` (archived 2020) are **not** supported.
Reddimus's first release is `v0.0.1` (2026-01-17).

| Version          | Supported          |
| ---------------- | ------------------ |
| latest tag       | :white_check_mark: |
| pre-Reddimus tag | :x:                |
| older            | :x:                |

## Reporting a Vulnerability

**Do not open a public issue.** Use GitHub's [private vulnerability
reporting](https://github.com/Reddimus/alpaca-markets-cpp/security/advisories/new)
flow, which delivers the report to the maintainer privately and tracks
coordinated disclosure.

When reporting, please include:

- Affected version (tag or commit SHA)
- A reproduction — minimal code or test case
- Impact (credential leak / request forgery / DoS / something else)
- Whether you've notified anyone else (e.g. Alpaca directly)

You can expect:

- Acknowledgement within **3 business days**
- An initial assessment + severity rating within **7 business days**
- A fix on a new `vX.Y.Z+1` tag, or a clear timeline if the fix is
  larger

## Out of Scope

- Bugs against `alpaca.markets` itself — those go to Alpaca's own
  vulnerability program, not this client library.
- Operational issues (rate-limit handling, network blips) — file a
  regular issue.
- Theoretical issues against dependencies — report them upstream
  (`openssl`, `cpp-httplib`, `rapidjson`, `googletest`). We pin via
  FetchContent and bump on credible advisories.
