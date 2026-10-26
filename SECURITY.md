# Security

Security fixes target the latest release. Upgrade the FetchContent tag or
installed alpaca_markets package; pre-fork tags and older releases are
unsupported.

Report SDK vulnerabilities with GitHub's [private vulnerability reporting](https://github.com/Reddimus/alpaca-markets-cpp/security/advisories/new).
Include the affected commit/version, a minimal reproduction, and impact.
Do not post credentials or a public issue containing exploit details.

REST authentication sends account key and secret headers; use HTTPS and
paper credentials while developing. Report service vulnerabilities to
Alpaca and dependency vulnerabilities to their maintainers (OpenSSL,
cpp-httplib, Glaze, or Google Test). Ordinary SDK bugs belong in issues.
