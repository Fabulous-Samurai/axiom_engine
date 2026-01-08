@echo off
echo ============================================================
echo   AXIOM Engine - Build and Test
echo ============================================================
echo.

cd /d %~dp0\..\..

echo [1/3] Building...
cd ninja-build
ninja axiom
if errorlevel 1 (
    echo BUILD FAILED!
    exit /b 1
)
echo BUILD SUCCESS!
echo.

cd ..
echo [2/3] Testing log2 function...
echo log2(8) | ninja-build\axiom.exe --interactive
echo.

echo [3/3] Running comprehensive test suite...
python tests\performance\comprehensive_test_suite.py

echo.
echo ============================================================
echo   Test Complete!
echo ============================================================
pause
