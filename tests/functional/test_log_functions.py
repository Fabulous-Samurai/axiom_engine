import subprocess
from pathlib import Path


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

proc = subprocess.Popen(
    [resolve_axiom_exe(), "--interactive"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    bufsize=0
)

if proc.stdin is None or proc.stdout is None:
    raise RuntimeError("Failed to open subprocess pipes for axiom CLI")

test_cases = [
    ("log(100)", "should be 2"),
    ("ln(2.71828)", "should be ~1"),
    ("lg(8)", "should be 3 (log base 2)"),
    ("log2(8)", "should be 3"),
    ("log2(16)", "should be 4"),
]

for cmd, desc in test_cases:
    print(f"\nTest: {cmd} ({desc})")
    proc.stdin.write(cmd + "\n")
    proc.stdin.flush()
    
    output = []
    while True:
        line = proc.stdout.readline()
        print(f"  Raw: '{line.rstrip()}'")
        if "__END__" in line:
            break
        if line.strip() and "Error" not in line:
            output.append(line.strip())
    
    if output:
        print(f"  ✅ Result: {output[0]}")
    else:
        print("  ❌ No result!")

proc.terminate()
