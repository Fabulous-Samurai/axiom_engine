#!/usr/bin/env python3
"""AXIOM PRO GUI entrypoint (Qt-only)."""

from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))


def main():
    try:
        from gui.qt.axiom_qt_gui import main as qt_main
    except ImportError as exc:
        print("Failed to launch Qt PRO GUI. Ensure PySide6 is installed.")
        print("Install with one of:")
        print("  pip install PySide6")
        print("  C:/msys64/usr/bin/pacman.exe -S mingw-w64-x86_64-pyside6")
        print(f"Import error: {exc}")
        return 1
    return qt_main()


if __name__ == "__main__":
    raise SystemExit(main())
