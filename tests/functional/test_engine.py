#!/usr/bin/env python3
"""Test axiom.exe execution"""
import sys
from pathlib import Path

repo_root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(repo_root / "gui" / "python"))

from gui_helpers import CppEngineInterface


def resolve_axiom_exe() -> str:
    candidates = [
        repo_root / "build" / "axiom.exe",
        repo_root / "ninja-build" / "axiom.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)
    raise FileNotFoundError("axiom.exe not found in build/ or ninja-build/")


# Test the engine
engine = CppEngineInterface(resolve_axiom_exe())

tests = [
    "2+2",
    "5*3",
    "10/2",
    "sqrt(16)",
    "sin(0)",
]

print("Testing axiom.exe with single-command mode:")
print("=" * 50)

for test in tests:
    result = engine.execute_command(test)
    if result["success"]:
        print(f"✓ {test} = {result['result']} ({result['execution_time']}ms)")
    else:
        print(f"✗ {test} failed: {result.get('error', 'Unknown error')}")

print("\nEngine test complete!")
