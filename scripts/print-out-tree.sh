#!/usr/bin/env bash
set -euo pipefail

root="${1:-out}"

if [ ! -d "$root" ]; then
  echo "Directory not found: $root" >&2
  exit 1
fi

echo "[info] Tree for: $root"

if command -v tree >/dev/null 2>&1; then
  tree -a "$root"
  exit 0
fi

if command -v python3 >/dev/null 2>&1; then
  python3 - <<'PY'
import os
import sys

root = sys.argv[1]
for base, dirs, files in os.walk(root):
    rel = os.path.relpath(base, root)
    level = 0 if rel == '.' else rel.count(os.sep) + 1
    indent = '  ' * level
    print(f"{indent}{os.path.basename(base)}/")
    for name in sorted(files):
        print(f"{indent}  {name}")
PY
  exit 0
fi

find "$root" -print
