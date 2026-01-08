@echo off
echo 🚀 AXIOM ENGINE v3.0 - NINJA BUILD SYSTEM
echo ========================================

REM Clean any existing builds
if exist "ninja-build" rmdir /s /q "ninja-build"

REM Create fresh ninja build directory
mkdir ninja-build
cd ninja-build

echo 🏗️ Configuring NINJA build with maximum optimization...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

echo 🏎️ Building AXIOM Engine with NINJA parallel compilation...
ninja -j %NUMBER_OF_PROCESSORS%

echo ✅ AXIOM Engine v3.0 build complete!
echo 📍 Executable: ninja-build\axiom.exe
echo 🧪 Test suite: ninja-build\run_tests.exe

cd ..