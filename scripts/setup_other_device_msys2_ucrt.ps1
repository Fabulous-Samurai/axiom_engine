$ErrorActionPreference = "Stop"

Write-Host "[AXIOM] MSYS2 UCRT setup starting..." -ForegroundColor Cyan

$pacman = "C:/msys64/usr/bin/pacman.exe"
if (-not (Test-Path $pacman)) {
    throw "MSYS2 pacman not found at $pacman"
}

& $pacman -S --needed --noconfirm `
    mingw-w64-ucrt-x86_64-gcc `
    mingw-w64-ucrt-x86_64-cmake `
    mingw-w64-ucrt-x86_64-ninja `
    mingw-w64-ucrt-x86_64-python `
    mingw-w64-ucrt-x86_64-pyside6

$py = "C:/msys64/ucrt64/bin/python.exe"
if (-not (Test-Path $py)) {
    throw "UCRT Python not found at $py"
}

& $py -m venv .venv
& .\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements-optional.txt

cmake --preset default-ninja
cmake --build build --config Release

if (Test-Path "build\run_tests.exe") {
    & .\build\run_tests.exe
}

Write-Host "[AXIOM] MSYS2 UCRT setup completed." -ForegroundColor Green
