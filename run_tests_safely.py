import subprocess
import sys

try:
    print("Starting tests...")
    result = subprocess.run(
        r".\build\run_tests.exe",
        capture_output=True,
        text=True,
        timeout=10
    )
    print("EXIT CODE:", result.returncode)
    print("STDOUT LEN:", len(result.stdout))
    print("STDERR LEN:", len(result.stderr))
    if len(result.stdout) > 0:
        print("STDOUT:")
        print(result.stdout[:1000])
    if len(result.stderr) > 0:
        print("STDERR:")
        print(result.stderr[:1000])
except Exception as e:
    print("EXCEPTION:", str(e))
