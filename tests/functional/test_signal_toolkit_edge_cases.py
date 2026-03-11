#!/usr/bin/env python3
"""
Edge-case tests for SignalProcessingToolkit
Non-interactive (matplotlib Agg, messagebox patched)
"""
import sys
from pathlib import Path
import pytest

import matplotlib
matplotlib.use('Agg')

project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / 'tools' / 'analysis'))

# Patch messagebox to avoid GUI blocking
from tkinter import messagebox
messagebox.showerror = lambda *args, **kwargs: None
messagebox.showwarning = lambda *args, **kwargs: None
messagebox.showinfo = lambda *args, **kwargs: None

from signal_processing_toolkit import SignalProcessingToolkit
import numpy as np


@pytest.fixture
def toolkit():
    """Provide a fresh toolkit instance per test for deterministic behavior."""
    return SignalProcessingToolkit()


def test_noise_determinism(toolkit):
    """Noise should be reproducible due to seeded RNG"""
    signals1 = toolkit.create_test_signals()
    noise1 = signals1['noise']
    signals2 = toolkit.create_test_signals()
    noise2 = signals2['noise']
    assert np.allclose(noise1, noise2), "Noise should be deterministic with seed"
    print("✅ Deterministic noise generation")


def test_spectrogram_missing_signal(toolkit):
    """Graceful handling when spectrogram called on missing signal"""
    toolkit.spectrogram_analysis("not_there")
    print("✅ Missing signal handled in spectrogram analysis")


def test_apply_filter_without_design(toolkit):
    """apply_filter should not crash when no filter is designed"""
    toolkit.create_test_signals()
    result = toolkit.apply_filter('sine_wave')  # filter_coeffs is None -> should exit gracefully
    assert result is None, "apply_filter should return None when no filter is designed"
    print("✅ apply_filter safe without prior design")


def test_psd_missing_signal(toolkit):
    """PSD analysis should handle missing signal gracefully"""
    toolkit.psd_analysis("not_there")
    print("✅ PSD handles missing signal")


def main():
    toolkit = SignalProcessingToolkit()
    tests = [
        test_noise_determinism,
        test_spectrogram_missing_signal,
        test_apply_filter_without_design,
        test_psd_missing_signal,
    ]

    failures = 0
    for t in tests:
        try:
            t(toolkit)
        except AssertionError as e:
            failures += 1
            print(f"❌ {t.__name__}: {e}")
        except Exception as e:
            failures += 1
            print(f"❌ {t.__name__}: Unexpected error: {e}")

    if failures:
        print(f"\n⚠️ {failures} edge case(s) failed")
        sys.exit(1)

    print("\n🎉 All edge cases passed")
    sys.exit(0)


if __name__ == "__main__":
    main()
