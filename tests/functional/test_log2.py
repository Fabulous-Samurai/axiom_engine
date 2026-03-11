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


engine = CppEngineInterface(resolve_axiom_exe())

print("Testing log2 function...")
result = engine.execute_command("log2(8)")
print(f"log2(8) = {result}")

result2 = engine.execute_command("log2(16)")
print(f"log2(16) = {result2}")

result3 = engine.execute_command("log2(1024)")
print(f"log2(1024) = {result3}")

engine.close()
