#!/usr/bin/env bash
set -euo pipefail
source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/alpaca-consumers.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
mkdir "$scratch_dir/readme"
python3 - "$source_dir/README.md" "$scratch_dir/readme" <<'PYCODE'
from pathlib import Path
import sys
for i, block in enumerate(Path(sys.argv[1]).read_text().split("```cpp\n")[1:]):
    Path(sys.argv[2], f"example{i}.cpp").write_text(block.split("```", 1)[0])
PYCODE
cmake -S "$source_dir" -B "$scratch_dir/sdk" -DCMAKE_BUILD_TYPE=Release \
    -DALPACA_MARKETS_BUILD_TESTS=OFF -DALPACA_MARKETS_BUILD_EXAMPLES=OFF "$@"
cmake --build "$scratch_dir/sdk" --parallel 4
cmake --install "$scratch_dir/sdk" --prefix "$scratch_dir/prefix"
for dependency in glaze gtest gmock httplib.h; do
    test ! -e "$scratch_dir/prefix/include/$dependency"
done
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/package" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$scratch_dir/prefix" \
    -DREADME_EXAMPLES_DIR="$scratch_dir/readme"
cmake --build "$scratch_dir/package" --parallel 4
"$scratch_dir/package/consumer"
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/fetch" \
    -DCMAKE_BUILD_TYPE=Release -DALPACA_SOURCE_DIR="$source_dir" \
    -DREADME_EXAMPLES_DIR="$scratch_dir/readme" "$@"
cmake --build "$scratch_dir/fetch" --parallel 4
"$scratch_dir/fetch/consumer"
echo 'Installed and FetchContent consumers pass'
