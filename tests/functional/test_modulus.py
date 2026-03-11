#!/usr/bin/env python
"""Test modulus operation."""
from pathlib import Path
from gui.python.gui_helpers import CppEngineInterface


def resolve_axiom_exe() -> str:
	repo_root = Path(__file__).resolve().parents[2]
	candidates = [
		repo_root / "build" / "axiom.exe",
		repo_root / "ninja-build" / "axiom.exe",
	]
	for candidate in candidates:
		if candidate.exists():
			return str(candidate)
	raise FileNotFoundError("axiom.exe not found in build/ or ninja-build/")


engine = CppEngineInterface(resolve_axiom_exe())
print("Testing modulus operation...")

# Test with spaces
result = engine.execute_command('938247290474946 % 342423423')
print(f"Result with spaces: {result}")

# Test without spaces  
result = engine.execute_command('938247290474946%342423423')
print(f"Result without spaces: {result}")

engine.close()
