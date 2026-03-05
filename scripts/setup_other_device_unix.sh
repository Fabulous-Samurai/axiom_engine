#!/usr/bin/env bash
set -euo pipefail

echo "[AXIOM] Unix setup starting..."

command -v cmake >/dev/null 2>&1 || { echo "cmake not found"; exit 1; }
command -v ninja >/dev/null 2>&1 || { echo "ninja not found"; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "python3 not found"; exit 1; }

if [[ ! -d .venv ]]; then
  python3 -m venv .venv
fi

source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements-optional.txt

cmake --preset default-ninja
cmake --build build --config Release

if [[ -x build/run_tests ]]; then
  ./build/run_tests
fi

echo "[AXIOM] Unix setup completed."
