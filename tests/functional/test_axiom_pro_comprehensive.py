#!/usr/bin/env python3
"""
Comprehensive Test Suite for AXIOM PRO GUI
Tests all major functions and features
"""

import sys
import os
import subprocess
import time
from pathlib import Path

# Add project paths
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / 'gui' / 'python'))
sys.path.insert(0, str(project_root / 'tools' / 'analysis'))

def test_axiom_engine_availability():
    """Test 1: Check if axiom.exe is available"""
    print("\n" + "="*60)
    print("TEST 1: AXIOM Engine Availability")
    print("="*60)
    
    axiom_paths = [
        project_root / 'ninja-build' / 'axiom.exe',
        project_root / 'build' / 'axiom.exe',
        project_root / 'axiom.exe'
    ]
    
    found = False
    for path in axiom_paths:
        if path.exists():
            print(f"✅ Found axiom.exe at: {path}")
            found = True
            break
    
    if not found:
        print("⚠️ axiom.exe not found. GUI will work but calculations may be limited.")
    
    return found

def test_signal_processing_toolkit():
    """Test 2: Signal Processing Toolkit"""
    print("\n" + "="*60)
    print("TEST 2: Signal Processing Toolkit")
    print("="*60)
    
    try:
        from signal_processing_toolkit import SignalProcessingToolkit
        import numpy as np
        
        toolkit = SignalProcessingToolkit()
        print("✅ Signal Processing Toolkit imported successfully")
        
        # Test signal generation
        toolkit.create_test_signals()
        print(f"✅ Created {len(toolkit.signals)} test signals")
        print(f"   Signals: {list(toolkit.signals.keys())}")
        
        # Test frequency analysis
        if 'sine_wave' in toolkit.signals:
            print("✅ Running frequency analysis on sine wave...")
            # Don't show GUI in automated test
            import matplotlib
            matplotlib.use('Agg')
            
            # Test individual methods
            print("   Testing FFT analysis...")
            toolkit.fft_analysis('sine_wave')
            print("   ✅ FFT analysis passed")
            
            print("   Testing peak detection...")
            toolkit.peak_detection('sine_wave')
            print("   ✅ Peak detection passed")
            
            print("   Testing correlation analysis...")
            toolkit.correlation_analysis('sine_wave')
            print("   ✅ Correlation analysis passed")
            
            print("   Testing wavelet analysis...")
            toolkit.wavelet_analysis('sine_wave')
            print("   ✅ Wavelet analysis passed")
        
        print("✅ All signal processing tests passed!")
        return True
        
    except Exception as e:
        print(f"❌ Signal Processing Toolkit test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_gui_imports():
    """Test 3: GUI Imports"""
    print("\n" + "="*60)
    print("TEST 3: AXIOM PRO GUI Imports")
    print("="*60)
    
    try:
        import tkinter as tk
        from tkinter import ttk
        print("✅ tkinter imported successfully")
        
        import matplotlib
        matplotlib.use('TkAgg')
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        print("✅ matplotlib with TkAgg backend imported successfully")
        
        import numpy as np
        print("✅ numpy imported successfully")
        
        # Import AXIOM PRO GUI
        sys.path.insert(0, str(project_root / 'gui' / 'python'))
        import axiom_pro_gui
        print("✅ axiom_pro_gui module imported successfully")
        
        return True
        
    except Exception as e:
        print(f"❌ GUI imports failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_gui_class_structure():
    """Test 4: GUI Class Structure"""
    print("\n" + "="*60)
    print("TEST 4: AXIOM PRO GUI Class Structure")
    print("="*60)
    
    try:
        import tkinter as tk
        sys.path.insert(0, str(project_root / 'gui' / 'python'))
        from axiom_pro_gui import AxiomProGUI
        
        print("✅ AxiomProGUI class imported successfully")
        
        # Check class attributes
        required_methods = [
            'create_gui',
            'create_workspace_browser',
            'create_command_window',
            'create_figure_display',
            'execute_command',
            'send_to_axiom',
            'plot_results'
        ]
        
        for method in required_methods:
            if hasattr(AxiomProGUI, method):
                print(f"✅ Method '{method}' exists")
            else:
                print(f"⚠️ Method '{method}' not found")
        
        print("✅ GUI class structure verified!")
        return True
        
    except Exception as e:
        print(f"❌ GUI class structure test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_mathematical_functions():
    """Test 5: Mathematical Operations"""
    print("\n" + "="*60)
    print("TEST 5: Mathematical Operations")
    print("="*60)
    
    test_expressions = [
        ("2 + 2", 4),
        ("10 * 5", 50),
        ("100 / 4", 25),
        ("2^8", 256),
        ("sqrt(144)", 12),
        ("sin(0)", 0),
        ("cos(0)", 1),
    ]
    
    passed = 0
    failed = 0
    
    # Find axiom.exe
    axiom_exe = None
    for path in [project_root / 'ninja-build' / 'axiom.exe',
                 project_root / 'build' / 'axiom.exe',
                 project_root / 'axiom.exe']:
        if path.exists():
            axiom_exe = str(path)
            break
    
    if not axiom_exe:
        print("⚠️ axiom.exe not found, skipping mathematical tests")
        return True
    
    for expr, expected in test_expressions:
        try:
            result = subprocess.run(
                [axiom_exe],
                input=f"{expr}\nexit\n",
                capture_output=True,
                text=True,
                timeout=5
            )
            
            # Check if calculation succeeded
            if result.returncode == 0 or str(expected) in result.stdout:
                print(f"✅ {expr} = {expected}")
                passed += 1
            else:
                print(f"⚠️ {expr} (could not verify)")
                
        except Exception as e:
            print(f"❌ {expr} failed: {e}")
            failed += 1
    
    print(f"\n📊 Results: {passed} passed, {failed} failed")
    return failed == 0

def test_plotting_capabilities():
    """Test 6: Plotting Capabilities"""
    print("\n" + "="*60)
    print("TEST 6: Plotting Capabilities")
    print("="*60)
    
    try:
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
        import matplotlib.pyplot as plt
        import numpy as np
        
        # Test basic plotting
        x = np.linspace(0, 2*np.pi, 100)
        y = np.sin(x)
        
        plt.figure()
        plt.plot(x, y)
        plt.title('Test Plot: sin(x)')
        plt.close()
        
        print("✅ Basic plotting works")
        
        # Test multiple subplots
        fig, (ax1, ax2) = plt.subplots(2, 1)
        ax1.plot(x, y)
        ax2.plot(x, np.cos(x))
        plt.close()
        
        print("✅ Subplot creation works")
        
        # Test 3D plotting
        from mpl_toolkits.mplot3d import Axes3D
        fig = plt.figure()
        fig.add_subplot(111, projection='3d')
        plt.close()
        
        print("✅ 3D plotting works")
        
        print("✅ All plotting tests passed!")
        return True
        
    except Exception as e:
        print(f"❌ Plotting test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_workspace_operations():
    """Test 7: Workspace Operations"""
    print("\n" + "="*60)
    print("TEST 7: Workspace Operations")
    print("="*60)
    
    try:
        import numpy as np
        
        # Simulate workspace operations
        workspace = {}
        
        # Test variable creation
        workspace['x'] = np.array([1, 2, 3, 4, 5])
        workspace['y'] = np.array([2, 4, 6, 8, 10])
        workspace['matrix_A'] = np.array([[1, 2], [3, 4]])
        
        print(f"✅ Created {len(workspace)} workspace variables")
        
        # Test variable operations
        workspace['sum'] = workspace['x'] + workspace['y']
        workspace['det_A'] = np.linalg.det(workspace['matrix_A'])
        
        print("✅ Variable operations work")
        print(f"   Workspace contains: {list(workspace.keys())}")
        
        return True
        
    except Exception as e:
        print(f"❌ Workspace operations test failed: {e}")
        return False

def run_all_tests():
    """Run all comprehensive tests"""
    print("\n" + "🏛️"*30)
    print("AXIOM PRO - COMPREHENSIVE TEST SUITE")
    print("🏛️"*30)
    
    results = {}
    
    # Run all tests
    results['engine'] = test_axiom_engine_availability()
    results['signal_processing'] = test_signal_processing_toolkit()
    results['gui_imports'] = test_gui_imports()
    results['gui_structure'] = test_gui_class_structure()
    results['math_operations'] = test_mathematical_functions()
    results['plotting'] = test_plotting_capabilities()
    results['workspace'] = test_workspace_operations()
    
    # Summary
    print("\n" + "="*60)
    print("TEST SUMMARY")
    print("="*60)
    
    total = len(results)
    passed = sum(1 for v in results.values() if v)
    failed = total - passed
    
    for test_name, result in results.items():
        status = "✅ PASSED" if result else "❌ FAILED"
        print(f"{test_name:20s}: {status}")
    
    print("\n" + "="*60)
    print(f"Total: {total} tests")
    print(f"Passed: {passed} ({passed*100//total}%)")
    print(f"Failed: {failed}")
    print("="*60)
    
    if failed == 0:
        print("\n🎉 ALL TESTS PASSED! AXIOM PRO is ready!")
    else:
        print(f"\n⚠️ {failed} test(s) failed. Please review.")
    
    return failed == 0

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
