"""Quick test of persistent subprocess performance."""
import sys
import time
from pathlib import Path

# Add current directory to path
sys.path.insert(0, str(Path(__file__).parent))

from gui_helpers import CppEngineInterface

def test_persistent_mode():
    """Test the persistent subprocess implementation."""
    
    # Find the C++ engine
    repo_root = Path(__file__).resolve().parents[2]
    possible_paths = [
        repo_root / "ninja-build" / "axiom.exe",
        repo_root / "ninja-build" / "giga_test_suite.exe",  # Fallback for testing
        repo_root / "build" / "axiom.exe",
    ]
    
    engine_path = None
    for path in possible_paths:
        if path.exists():
            engine_path = str(path)
            print(f"✅ Found engine: {path.name}")
            break
    
    if not engine_path:
        print("❌ No C++ engine found. Please build it first:")
        print("   cd ninja-build && ninja")
        return
    
    print("\n🚀 Initializing persistent subprocess...")
    engine = CppEngineInterface(engine_path)
    
    if not engine.process or engine.process.poll() is not None:
        print("⚠️  Persistent mode failed to start, using fallback")
    else:
        print("✅ Persistent subprocess active!")
    
    # Test commands
    test_commands = [
        "2+2",
        "5*7", 
        "sqrt(144)",
        "sin(pi/2)",
        "factorial(5)",
    ]
    
    print(f"\n📊 Testing {len(test_commands)} commands...\n")
    
    total_time = 0
    for i, cmd in enumerate(test_commands, 1):
        start = time.time()
        result = engine.execute_command(cmd)
        elapsed_ms = (time.time() - start) * 1000
        total_time += elapsed_ms
        
        if result["success"]:
            mode = "🟢 PERSISTENT" if result.get("persistent") else "🔵 FALLBACK"
            speed = "⚡ SENNA" if elapsed_ms < 20 else "🏎️  F1" if elapsed_ms < 50 else "🐌 SLOW"
            print(f"{i}. {cmd:15s} = {result['result']:10s} | \033[1m{elapsed_ms:5.1f}ms\033[0m | {speed} | {mode}")
        else:
            print(f"{i}. {cmd:15s} = ERROR: {result.get('error', 'Unknown')}")
    
    avg_time = total_time / len(test_commands)
    print(f"\n📈 Average: \033[1m{avg_time:.1f}ms\033[0m per command")
    
    if avg_time < 20:
        print("🎉 EXCELLENT! Sub-20ms performance (cold-start eliminated)")
    elif avg_time < 50:
        print("✅ GOOD! Under 50ms (faster than before)")
    else:
        print("⚠️  Still has overhead (check if persistent mode is active)")
    
    # Cleanup
    engine.close()
    print("\n✅ Test complete!")


if __name__ == "__main__":
    test_persistent_mode()
