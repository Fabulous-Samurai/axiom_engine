param(
    [switch]$WithGui
)

$ErrorActionPreference = "Stop"

Write-Host "[AXIOM] Windows setup starting..." -ForegroundColor Cyan

if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) { throw "Required command not found: cmake" }
if (-not (Get-Command "ninja" -ErrorAction SilentlyContinue)) { throw "Required command not found: ninja" }
if (-not (Get-Command "python" -ErrorAction SilentlyContinue)) { throw "Required command not found: python" }

if (-not (Test-Path ".venv")) {
    python -m venv .venv
}

& .\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements-optional.txt

if ($WithGui) {
    python -m pip install PySide6
}

cmake --preset default-ninja
cmake --build build --config Release

if (Test-Path "build\run_tests.exe") {
    & .\build\run_tests.exe
}

Write-Host "[AXIOM] Windows setup completed." -ForegroundColor Green
