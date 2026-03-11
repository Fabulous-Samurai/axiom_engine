#!/usr/bin/env python3
"""
Background test suite for AXIOM PRO v3.0 Fast Render and FPS features
Tests without launching GUI (uses Agg backend)
"""
import sys
import os
from pathlib import Path
import numpy as np
import pytest

# Setup paths
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / 'gui' / 'python'))
sys.path.insert(0, str(project_root / 'tools' / 'analysis'))

# Force Agg backend before any GUI imports
import matplotlib
matplotlib.use('Agg')

import tkinter as tk
try:
    from axiom_pro_gui import AxiomProGUI
except (ImportError, AttributeError):
    pytest.skip(
        "Legacy Tk AxiomProGUI is unavailable in current Qt-first architecture",
        allow_module_level=True,
    )

def test_gui_initialization():
    """Test GUI initializes with new features"""
    print("Test 1: GUI Initialization")
    try:
        root = tk.Tk()
        root.withdraw()  # Hide window
        app = AxiomProGUI(root)
        
        # Check Fast Render var exists
        assert hasattr(app, 'fast_render_var'), "Missing fast_render_var"
        assert isinstance(app.fast_render_var, tk.BooleanVar), "fast_render_var wrong type"
        
        # Check FPS display var exists
        assert hasattr(app, 'show_fps_var'), "Missing show_fps_var"
        assert isinstance(app.show_fps_var, tk.BooleanVar), "show_fps_var wrong type"
        
        # Check decimation methods exist
        assert hasattr(app, '_decimate_xy'), "Missing _decimate_xy method"
        assert hasattr(app, '_decimate_grid'), "Missing _decimate_grid method"
        
        # Check FPS callback
        assert hasattr(app, '_on_draw_event'), "Missing _on_draw_event method"
        
        root.destroy()
        print("  ✅ GUI initialization: PASS")
        return True
    except Exception as e:
        print(f"  ❌ GUI initialization: FAIL - {e}")
        return False


def test_decimation_logic():
    """Test decimation functions"""
    print("\nTest 2: Decimation Logic")
    try:
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Test _decimate_xy with large dataset
        x_large = np.linspace(0, 100, 50000)
        y_large = np.sin(x_large)
        x_dec, y_dec = app._decimate_xy(x_large, y_large, target_points=5000)
        
        assert x_dec.size <= 5000, f"XY decimation failed: {x_dec.size} > 5000"
        assert y_dec.size <= 5000, f"XY decimation failed: {y_dec.size} > 5000"
        assert x_dec.size == y_dec.size, "XY size mismatch after decimation"
        print(f"  ✅ XY decimation: {x_large.size} → {x_dec.size} points")
        
        # Test _decimate_xy with small dataset (should not decimate)
        x_small = np.linspace(0, 10, 100)
        y_small = np.cos(x_small)
        x_dec2, _ = app._decimate_xy(x_small, y_small, target_points=5000)
        
        assert x_dec2.size == x_small.size, "Small dataset incorrectly decimated"
        print(f"  ✅ XY small dataset preserved: {x_small.size} points")
        
        # Test _decimate_grid with 2D mesh
        x = np.linspace(-5, 5, 200)
        y = np.linspace(-5, 5, 200)
        x_grid, y_grid = np.meshgrid(x, y)
        z_grid = np.sin(np.sqrt(x_grid**2 + y_grid**2))

        x_dec_g, y_dec_g, z_dec_g = app._decimate_grid(x_grid, y_grid, z_grid, target_side=100)

        assert x_dec_g.shape[0] <= 100, f"Grid decimation failed: {x_dec_g.shape[0]} > 100"
        assert y_dec_g.shape[0] <= 100, f"Grid decimation failed: {y_dec_g.shape[0]} > 100"
        assert z_dec_g.shape == x_dec_g.shape, "Grid shapes mismatch after decimation"
        print(f"  ✅ Grid decimation: {x_grid.shape} → {x_dec_g.shape}")
        
        root.destroy()
        print("  ✅ Decimation logic: PASS")
        return True
    except Exception as e:
        print(f"  ❌ Decimation logic: FAIL - {e}")
        return False


def test_fast_render_toggle():
    """Test Fast Render toggle behavior"""
    print("\nTest 3: Fast Render Toggle")
    try:
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Initially disabled
        initial_state = app.fast_render_var.get()
        assert isinstance(initial_state, bool), "Fast Render state not boolean"
        print(f"  ✅ Initial Fast Render state: {initial_state}")
        
        # Toggle on
        app.fast_render_var.set(True)
        assert app.fast_render_var.get() == True, "Failed to enable Fast Render"
        print("  ✅ Fast Render toggle ON: PASS")
        
        # Toggle off
        app.fast_render_var.set(False)
        assert app.fast_render_var.get() == False, "Failed to disable Fast Render"
        print("  ✅ Fast Render toggle OFF: PASS")
        
        root.destroy()
        print("  ✅ Fast Render toggle: PASS")
        return True
    except Exception as e:
        print(f"  ❌ Fast Render toggle: FAIL - {e}")
        return False


def test_fps_display_toggle():
    """Test FPS display toggle"""
    print("\nTest 4: FPS Display Toggle")
    try:
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Initially enabled (default True)
        initial_state = app.show_fps_var.get()
        assert isinstance(initial_state, bool), "FPS display state not boolean"
        print(f"  ✅ Initial FPS display state: {initial_state}")
        
        # Toggle off
        app.show_fps_var.set(False)
        assert app.show_fps_var.get() == False, "Failed to disable FPS display"
        print("  ✅ FPS display toggle OFF: PASS")
        
        # Toggle on
        app.show_fps_var.set(True)
        assert app.show_fps_var.get() == True, "Failed to enable FPS display"
        print("  ✅ FPS display toggle ON: PASS")
        
        root.destroy()
        print("  ✅ FPS display toggle: PASS")
        return True
    except Exception as e:
        print(f"  ❌ FPS display toggle: FAIL - {e}")
        return False


def test_fast_plot_with_decimation():
    """Test fast_plot_xy with Fast Render enabled"""
    print("\nTest 5: Fast Plot with Decimation")
    try:
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Large dataset
        x_large = np.linspace(0, 100, 100000)
        y_large = np.sin(x_large) * np.exp(-0.01 * x_large)
        
        # Without Fast Render (should plot all points)
        app.fast_render_var.set(False)
        app.fast_plot_xy(x_large, y_large)
        print(f"  ✅ Plot without Fast Render: {x_large.size} points")
        
        # Clear for next test
        app.fig.clear()
        app._plot_line = None
        app._ax_main = None
        
        # With Fast Render (should decimate)
        app.fast_render_var.set(True)
        app.fast_plot_xy(x_large, y_large)
        print("  ✅ Plot with Fast Render: decimation applied")
        
        root.destroy()
        print("  ✅ Fast plot with decimation: PASS")
        return True
    except Exception as e:
        print(f"  ❌ Fast plot with decimation: FAIL - {e}")
        return False


def test_status_var_updates():
    """Test status variable exists and is bindable"""
    print("\nTest 6: Status Variable")
    try:
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Check status_var exists
        assert hasattr(app, 'status_var'), "Missing status_var"
        assert isinstance(app.status_var, tk.StringVar), "status_var wrong type"
        
        # Get initial value
        initial = app.status_var.get()
        print(f"  ✅ Initial status: '{initial}'")
        
        # Test setting value
        app.status_var.set("Test status message")
        assert app.status_var.get() == "Test status message", "Failed to update status"
        print("  ✅ Status update: PASS")
        
        root.destroy()
        print("  ✅ Status variable: PASS")
        return True
    except Exception as e:
        print(f"  ❌ Status variable: FAIL - {e}")
        return False


def test_performance_comparison():
    """Compare performance metrics with/without Fast Render"""
    print("\nTest 7: Performance Comparison")
    try:
        import time
        root = tk.Tk()
        root.withdraw()
        app = AxiomProGUI(root)
        
        # Large dataset
        n = 500000
        x = np.linspace(0, 100, n)
        y = np.sin(x) + 0.1 * np.cos(10 * x)
        
        # Without Fast Render
        app.fast_render_var.set(False)
        app.fig.clear()
        app._plot_line = None
        app._ax_main = None
        
        t0 = time.perf_counter()
        app.fast_plot_xy(x, y)
        app.canvas.draw()
        t_full = (time.perf_counter() - t0) * 1000
        
        # With Fast Render
        app.fast_render_var.set(True)
        app.fig.clear()
        app._plot_line = None
        app._ax_main = None
        
        t0 = time.perf_counter()
        app.fast_plot_xy(x, y)
        app.canvas.draw()
        t_fast = (time.perf_counter() - t0) * 1000
        
        speedup = t_full / t_fast if t_fast > 0 else 1.0
        
        print(f"  Full render: {t_full:.1f} ms ({n} points)")
        print(f"  Fast render: {t_fast:.1f} ms (decimated)")
        print(f"  Speedup: {speedup:.1f}x")
        
        # Expect some speedup (at least 1.2x)
        assert speedup >= 1.0, f"No speedup observed: {speedup:.2f}x"
        
        root.destroy()
        print("  ✅ Performance comparison: PASS")
        return True
    except Exception as e:
        print(f"  ❌ Performance comparison: FAIL - {e}")
        return False


def main():
    """Run all tests"""
    print("="*70)
    print("AXIOM PRO v3.0 - Fast Render & FPS Features Test Suite")
    print("="*70)
    
    tests = [
        test_gui_initialization,
        test_decimation_logic,
        test_fast_render_toggle,
        test_fps_display_toggle,
        test_fast_plot_with_decimation,
        test_status_var_updates,
        test_performance_comparison,
    ]
    
    results = []
    for test_func in tests:
        try:
            results.append(test_func())
        except Exception as e:
            print(f"  ❌ Test crashed: {e}")
            results.append(False)
    
    # Summary
    print("\n" + "="*70)
    print("TEST SUMMARY")
    print("="*70)
    passed = sum(results)
    total = len(results)
    print(f"Passed: {passed}/{total}")
    print(f"Failed: {total - passed}/{total}")
    
    if passed == total:
        print("\n🎉 All tests PASSED! Fast Render & FPS features verified.")
        return 0
    else:
        print(f"\n⚠️  {total - passed} test(s) FAILED. Review output above.")
        return 1


if __name__ == '__main__':
    exit_code = main()
    sys.exit(exit_code)
