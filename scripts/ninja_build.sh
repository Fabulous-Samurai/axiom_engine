#!/bin/bash
echo "🚀 AXIOM ENGINE v3.0 - NINJA BUILD SYSTEM"
echo "========================================"

# Clean any existing builds
rm -rf ninja-build

# Create fresh ninja build directory
mkdir ninja-build
cd ninja-build

echo "🏗️ Configuring NINJA build with maximum optimization..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

echo "🏎️ Building AXIOM Engine with NINJA parallel compilation..."
ninja -j $(nproc)

echo "✅ AXIOM Engine v3.0 build complete!"
echo "📍 Executable: ninja-build/axiom"
echo "🧪 Test suite: ninja-build/run_tests"

cd ..