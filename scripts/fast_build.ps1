#!/usr/bin/env powershell
# AXIOM Engine v3.0 - Ultra-Fast Parallel Build Script
# Optimized for maximum compilation speed

param(
    [string]$BuildType = "Release",
    [string]$Generator = "Ninja",
    [switch]$Clean,
    [switch]$Verbose,
    [switch]$Benchmark
)

Write-Host "🚀 AXIOM Engine v3.0 - Ultra-Fast Parallel Build" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

# Detect CPU cores for maximum parallelism
$CPU_COUNT = (Get-CimInstance -ClassName Win32_ComputerSystem).NumberOfLogicalProcessors
Write-Host "🔥 Detected CPU cores: $CPU_COUNT" -ForegroundColor Green

# Set build directory
$BUILD_DIR = "build-fast"

# Clean if requested
if ($Clean) {
    Write-Host "🧹 Cleaning previous build..." -ForegroundColor Yellow
    if (Test-Path $BUILD_DIR) {
        Remove-Item -Recurse -Force $BUILD_DIR
    }
}

# Create build directory
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
}

# Check for Ninja
$UseNinja = $false
try {
    ninja --version | Out-Null
    $UseNinja = $true
    Write-Host "⚡ Ninja generator available - using for maximum speed" -ForegroundColor Green
} catch {
    Write-Host "⚠️ Ninja not found - using default generator" -ForegroundColor Yellow
}

# Configure build
Write-Host "⚙️ Configuring build..." -ForegroundColor Blue
$ConfigArgs = @(
    "-S", ".",
    "-B", $BUILD_DIR,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DENABLE_FAST_BUILD=ON",
    "-DENABLE_PARALLEL_BUILD=ON", 
    "-DENABLE_ADVANCED_FEATURES=ON",
    "-DENABLE_LTO=ON",
    "-DCMAKE_BUILD_PARALLEL_LEVEL=$CPU_COUNT"
)

if ($UseNinja) {
    $ConfigArgs += "-G", "Ninja"
}

# Add verbose flag if requested
if ($Verbose) {
    $ConfigArgs += "--verbose"
}

$ConfigTime = Measure-Command {
    & cmake $ConfigArgs
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Configuration completed in $($ConfigTime.TotalSeconds) seconds" -ForegroundColor Green

# Build
Write-Host "🔨 Building with maximum parallelism..." -ForegroundColor Blue
$BuildArgs = @(
    "--build", $BUILD_DIR,
    "--parallel", $CPU_COUNT,
    "--config", $BuildType
)

if ($Verbose) {
    $BuildArgs += "--verbose"
}

$BuildTime = Measure-Command {
    & cmake $BuildArgs
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Build completed in $($BuildTime.TotalSeconds) seconds" -ForegroundColor Green

# Show build summary
$TotalTime = $ConfigTime.Add($BuildTime)
Write-Host ""
Write-Host "📊 BUILD SUMMARY" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Cyan
Write-Host "Configuration: $($ConfigTime.TotalSeconds) seconds" -ForegroundColor White
Write-Host "Compilation:   $($BuildTime.TotalSeconds) seconds" -ForegroundColor White
Write-Host "Total Time:    $($TotalTime.TotalSeconds) seconds" -ForegroundColor Green
Write-Host "CPU Cores:     $CPU_COUNT" -ForegroundColor White
Write-Host "Generator:     $(if ($UseNinja) { 'Ninja (Fast)' } else { 'Default' })" -ForegroundColor White
Write-Host "Build Type:    $BuildType" -ForegroundColor White

# Performance classification
if ($BuildTime.TotalSeconds -lt 30) {
    Write-Host "🏎️ SENNA SPEED: Lightning fast build! (<30s)" -ForegroundColor Green
} elseif ($BuildTime.TotalSeconds -lt 60) {
    Write-Host "🏁 Formula 1 Speed: Very fast! (<60s)" -ForegroundColor Yellow  
} elseif ($BuildTime.TotalSeconds -lt 120) {
    Write-Host "🚗 Racing Speed: Fast! (<2m)" -ForegroundColor Orange
} else {
    Write-Host "🐌 Consider using Ninja generator or SSD storage" -ForegroundColor Red
}

# Run benchmark if requested
if ($Benchmark) {
    Write-Host ""
    Write-Host "🏁 Running performance benchmark..." -ForegroundColor Blue
    $ExePath = Join-Path $BUILD_DIR "axiom.exe"
    if (Test-Path $ExePath) {
        & $ExePath --benchmark
    } else {
        Write-Host "⚠️ Executable not found for benchmark" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Target AXIOM Engine is ready for enterprise computing!" -ForegroundColor Green
Write-Host "Run: $BUILD_DIR/axiom.exe" -ForegroundColor Cyan