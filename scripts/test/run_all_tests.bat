@echo off
echo ============================================================
echo   AXIOM ENGINE - COMPLETE TEST SUITE
echo ============================================================
echo.

cd /d %~dp0\..\..

echo [Step 1/5] Building project...
cd ninja-build
ninja axiom
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)
cd ..
echo Build: SUCCESS
echo.

echo [Step 2/5] Running comprehensive test suite...
python tests\performance\comprehensive_test_suite.py > test_results_comprehensive.txt 2>&1
type test_results_comprehensive.txt
echo.

echo [Step 3/5] Running performance test...
python tests\performance\quick_perf_test.py > test_results_performance.txt 2>&1
type test_results_performance.txt
echo.

echo [Step 4/5] Testing log2 function...
python tests\functional\test_log2.py > test_results_log2.txt 2>&1
type test_results_log2.txt
echo.

echo [Step 5/5] Testing persistent subprocess...
python tests\functional\test_persistent.py > test_results_persistent.txt 2>&1
type test_results_persistent.txt
echo.

echo ============================================================
echo   ALL TESTS COMPLETE!
echo ============================================================
echo.
echo Test results saved to:
echo   - test_results_comprehensive.txt
echo   - test_results_performance.txt
echo   - test_results_log2.txt
echo   - test_results_persistent.txt
echo.
pause
