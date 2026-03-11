# Test Runner Scripts

Automated test execution scripts for the AXIOM Engine.

## Available Scripts

### run_all_tests.bat (Windows)

Comprehensive test suite runner - builds and runs all tests.

### Usage:

```cmd
scripts\test\run_all_tests.bat
```

### Test Sequence:

1. Build project (Ninja)
2. Comprehensive test suite (41 tests)
3. Performance tests (10 tests)
4. log2 function tests
5. Persistent subprocess tests

### Output:

- Console output for each test
- Saved to `test_results_*.txt` files

### build_and_test.bat (Windows)

Build verification with quick tests.

### Usage:

```cmd
scripts\test\build_and_test.bat
```

### Test Sequence:

1. Clean build
2. Quick smoke tests
3. Basic functionality verification

## Test Results

Results are saved to the project root:

- `test_results_comprehensive.txt` - Full test suite
- `test_results_performance.txt` - Performance benchmarks
- `test_results_log2.txt` - log2 function tests
- `test_results_persistent.txt` - Subprocess tests

## Creating Custom Test Scripts

### Windows Batch Template

```batch
@echo off
cd /d %~dp0\..\..

echo Running custom tests...
python tests\performance\my_test.py

if errorlevel 1 (
    echo FAILED
    exit /b 1
)
echo SUCCESS
```

### PowerShell Template

```powershell
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot\..\.."

Write-Host "Running custom tests..." -ForegroundColor Cyan
python tests\performance\my_test.py

if ($LASTEXITCODE -ne 0) {
    Write-Host "FAILED" -ForegroundColor Red
    exit 1
}
Write-Host "SUCCESS" -ForegroundColor Green
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Run Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:

      - uses: actions/checkout@v2
      - name: Build and Test

        run: scripts\test\run_all_tests.bat
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    stages {
        stage('Test') {
            steps {
                bat 'scripts\\test\\run_all_tests.bat'
            }
        }
    }
}
```

## Test Configuration

### Environment Variables

```batch
set AXIOM_CPP_ENGINE=ninja-build\axiom.exe
set AXIOM_TIMEOUT=3000
set AXIOM_VERBOSE=1
```

### Python Path Setup

Tests automatically add `gui/python` to path:
```python
sys.path.insert(0, str(Path(__file__).parent.parent / "gui/python"))
```

## Troubleshooting

### Tests Not Found

```batch

# Check test files exist

dir tests\performance\*.py
dir tests\functional\*.py
```

### Build Failures

```batch

# Manual build first

cd ninja-build
ninja axiom
cd ..
```

### Python Import Errors

```batch

# Check Python path

python -c "import sys; print('\n'.join(sys.path))"

# Install dependencies

pip install -r requirements.txt
```

### Timeout Issues

```batch

# Increase timeout in test files

# Edit: gui_helpers.py

# Change: timeout=3 to timeout=10

```

## Performance Monitoring

### Continuous Performance Testing

```batch

# Run every hour

:loop
    scripts\test\run_all_tests.bat >> performance_log.txt
    timeout /t 3600 /nobreak
    goto loop
```

### Performance Regression Detection

```batch

# Compare with baseline

python tests\performance\quick_perf_test.py > current.txt
fc baseline.txt current.txt
```

## Best Practices

1. **Run Before Commit**: Always run tests before pushing
2. **Check All Results**: Review all test output files
3. **Performance Baseline**: Keep baseline results for comparison
4. **Clean Environment**: Run from fresh build occasionally
5. **Version Control**: Commit test results for history

## Related Documentation

- **Test Categories**: [tests/README.md](../../tests/README.md)
- **Performance Tests**: [tests/performance/README.md](../../tests/performance/README.md)
- **Build Scripts**: [scripts/build/README.md](../build/README.md)

