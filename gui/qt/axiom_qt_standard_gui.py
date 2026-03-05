#!/usr/bin/env python3
"""AXIOM Standard GUI - Qt edition."""

from pathlib import Path
import os
import shlex
import subprocess
import sys

from gui.python.gui_helpers import CppEngineInterface

try:
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QIcon
    from PySide6.QtWidgets import (
        QApplication,
        QFrame,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QPlainTextEdit,
        QPushButton,
        QSplitter,
        QVBoxLayout,
        QWidget,
    )
except ImportError:
    print("PySide6 is not installed.")
    print("Install with one of:")
    print("  pip install PySide6")
    print("  C:/msys64/usr/bin/pacman.exe -S mingw-w64-x86_64-pyside6")
    sys.exit(1)


class AxiomQtStandardWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self._icon_dir = Path(__file__).with_name("icons")
        app_icon = self._resolve_icon("axiom_mark.svg")
        if app_icon:
            self.setWindowIcon(app_icon)
        self.setWindowTitle("AXIOM Standard GUI - Surgical Harmonic")
        self.resize(1180, 760)
        self.engine_exe = self._locate_engine()
        self.engine = CppEngineInterface(self.engine_exe)
        self._build_ui()

    def _build_ui(self):
        root = QWidget()
        self.setCentralWidget(root)
        outer = QVBoxLayout(root)
        outer.setContentsMargins(8, 8, 8, 8)
        outer.setSpacing(8)

        header = QFrame()
        header.setObjectName("HeaderBar")
        hh = QHBoxLayout(header)
        hh.setContentsMargins(10, 8, 10, 8)
        brand = QLabel()
        brand.setObjectName("BrandMark")
        brand_icon = self._resolve_icon("axiom_mark.svg")
        if brand_icon:
            brand.setPixmap(brand_icon.pixmap(22, 22))

        title_box = QVBoxLayout()
        title_box.setContentsMargins(0, 0, 0, 0)
        title_box.setSpacing(1)

        title = QLabel("AXIOM STANDARD v3.0 | Surgical Harmonic")
        title.setObjectName("Title")
        subtitle = QLabel("Lean profile | deterministic execution surface")
        subtitle.setObjectName("SubTitle")
        title_box.addWidget(title)
        title_box.addWidget(subtitle)

        hh.addWidget(brand)
        hh.addLayout(title_box)
        hh.addStretch(1)
        hh.addWidget(QLabel("Simple mode"))
        outer.addWidget(header)

        split = QSplitter(Qt.Orientation.Horizontal)
        outer.addWidget(split, 1)

        left = QFrame()
        left.setObjectName("Card")
        lv = QVBoxLayout(left)
        lv.setContentsMargins(8, 8, 8, 8)
        lv.addWidget(QLabel("Expression"))
        self.input = QLineEdit()
        self.input.setPlaceholderText("2+3*4")
        lv.addWidget(self.input)
        self.run_btn = QPushButton("Execute")
        lv.addWidget(self.run_btn)
        lv.addWidget(QLabel("Output"))
        self.output = QPlainTextEdit()
        self.output.setReadOnly(True)
        lv.addWidget(self.output, 1)
        split.addWidget(left)

        right = QFrame()
        right.setObjectName("Card")
        rv = QVBoxLayout(right)
        rv.setContentsMargins(8, 8, 8, 8)
        rv.addWidget(QLabel("Quick Actions"))
        for label, expr in (
            ("Algebra", "2+3*4"),
            ("Linear", "--mode=linear solve([2,3;1,4],[5;6])"),
            ("Stats", "--mode=statistics mean([1,2,3,4])"),
        ):
            btn = QPushButton(label)
            btn.clicked.connect(lambda _=False, e=expr: self._prefill(e))
            rv.addWidget(btn)
        rv.addStretch(1)
        split.addWidget(right)
        split.setSizes([800, 320])

        self.run_btn.clicked.connect(self._run)
        self.input.returnPressed.connect(self._run)

    def _locate_engine(self):
        project_root = Path(__file__).resolve().parents[2]
        candidates = [
            project_root / "build" / "axiom.exe",
            project_root / "axiom.exe",
            project_root / "build" / "axiom",
            project_root / "axiom",
        ]
        for candidate in candidates:
            if candidate.exists():
                return str(candidate)
        return None

    def _build_engine_env(self):
        env = os.environ.copy()
        path_entries = []

        if self.engine_exe:
            engine_dir = str(Path(self.engine_exe).resolve().parent)
            path_entries.append(engine_dir)

        for p in (r"C:\msys64\mingw64\bin", r"C:\msys64\ucrt64\bin"):
            if Path(p).exists():
                path_entries.append(p)

        existing = env.get("PATH", "")
        env["PATH"] = os.pathsep.join(path_entries + [existing]) if path_entries else existing
        return env

    def _resolve_icon(self, icon_name: str):
        icon_path = self._icon_dir / icon_name
        if icon_path.exists():
            return QIcon(str(icon_path))
        return None

    def _prefill(self, expression):
        self.input.setText(expression)

    def closeEvent(self, event):
        try:
            if self.engine:
                self.engine.close()
        finally:
            super().closeEvent(event)

    def _parse_mode_and_command(self, raw: str):
        tokens = shlex.split(raw, posix=False)
        mode = None
        command_tokens = []
        i = 0
        while i < len(tokens):
            tok = tokens[i]
            if tok.startswith("--mode="):
                mode = tok.split("=", 1)[1]
            elif tok == "--mode" and i + 1 < len(tokens):
                mode = tokens[i + 1]
                i += 1
            else:
                command_tokens.append(tok)
            i += 1

        command = " ".join(command_tokens).strip()
        if mode is None:
            lowered = command.lower()
            if lowered.startswith(("derive ", "differentiate ", "diff(", "integrate(", "simplify", "expand", "factor", "limit(", "roots(", "taylor(")):
                mode = "symbolic"
            elif lowered.startswith(("mean(", "median(", "std(", "variance(", "var(")):
                mode = "statistics"
            elif lowered.startswith(("convert ", "unit(")):
                mode = "units"
            elif "[" in lowered and ";" in lowered:
                mode = "linear"
            else:
                mode = "algebraic"

        return mode, command

    def _run(self):
        raw = self.input.text().strip()
        if not raw:
            return
        self.output.appendPlainText(f">> {raw}")

        if not self.engine_exe:
            self.output.appendPlainText("Engine executable not found")
            return

        mode, command = self._parse_mode_and_command(raw)
        self.engine.set_mode(mode)
        self.run_btn.setEnabled(False)

        def on_result(result):
            def apply_result():
                self.run_btn.setEnabled(True)
                if result.get("success"):
                    self.output.appendPlainText(result.get("result", "(no output)"))
                else:
                    self.output.appendPlainText(result.get("error", "Execution failed"))
            from PySide6.QtCore import QTimer
            QTimer.singleShot(0, apply_result)

        self.engine.execute_command_async(command, on_result)


def main():
    app = QApplication(sys.argv)
    qss_path = Path(__file__).with_name("war_machine.qss")
    if qss_path.exists():
        app.setStyleSheet(qss_path.read_text(encoding="utf-8"))
    win = AxiomQtStandardWindow()
    win.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
