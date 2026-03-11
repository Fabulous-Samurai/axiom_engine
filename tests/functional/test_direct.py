import subprocess
from pathlib import Path


def resolve_axiom_exe() -> str:
    """Pick the most up-to-date built binary path for local CLI testing."""
    repo_root = Path(__file__).resolve().parents[2]
    candidates = [
        repo_root / "build" / "axiom.exe",
        repo_root / "ninja-build" / "axiom.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)
    raise FileNotFoundError("axiom.exe not found in build/ or ninja-build/")


# Direct test with axiom executable
proc = subprocess.Popen(
    [resolve_axiom_exe(), "--interactive"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    bufsize=1
)

if proc.stdin is None or proc.stdout is None:
    raise RuntimeError("Failed to open subprocess pipes for axiom CLI")

commands = [
    "ln(10)",
    "log(10)",  
    "log2(8)",
    "sqrt(16)"
]

for cmd in commands:
    print(f"\nTesting: {cmd}")
    proc.stdin.write(cmd + "\n")
    proc.stdin.flush()
    
    output_lines = []
    while True:
        line = proc.stdout.readline()
        if line == "":
            break
        if "__END__" in line:
            break
        output_lines.append(line.rstrip())
    
    print(f"Result: {' '.join(output_lines)}")

proc.stdin.write("exit\n")
proc.stdin.flush()
proc.stdin.close()
proc.wait()
