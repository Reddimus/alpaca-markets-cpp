# Contributing

See [README.md](README.md) for dependencies and configuration.

```sh
make build test lint consumers
make format
```

Use C++23, four spaces, and 120 columns, as configured in .clang-format.
Prefer explicit local types; structured bindings, lambdas, and iterators
may use auto. JSON uses private Glaze helpers in src/detail/json.hpp.
Keep regression cases for changed behavior. Local HTTP tests use dummy
credentials; live account checks require your own credentials.

Submit a focused PR with a clear problem, change, and validation.
Update CHANGELOG.md for user-visible changes. Merge only after Linux,
macOS, Windows, and documentation CI pass. Prepare versions through a PR,
then create a signed vX.Y.Z tag on verified main; release.yml publishes it.
Minor versions may change public types while the library remains 0.x.

Report vulnerabilities through [SECURITY.md](SECURITY.md). Contributions
are licensed under the repository's MIT license.
