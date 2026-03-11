#!/usr/bin/env python3
"""AXIOM Standard GUI - Qt edition."""

from pathlib import Path
import queue
import os
import shlex
import subprocess
import sys

from gui.python.gui_helpers import CppEngineInterface


NO_OUTPUT_TEXT = "(no output)"
EXECUTION_FAILED_TEXT = "Execution failed"
APP_ICON_NAME = "axiom_mark.svg"

try:
    from PySide6.QtCore import QTimer, Qt
    from PySide6.QtGui import QColor, QIcon, QPainter, QPainterPath, QPen, QPixmap
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
        app_icon = self._resolve_icon(APP_ICON_NAME)
        if app_icon:
            self.setWindowIcon(app_icon)
        self.setWindowTitle("AXIOM Standard GUI - Surgical Harmonic")
        self.resize(1180, 760)
        self.engine_exe = self._locate_engine()
        self.engine = CppEngineInterface(self.engine_exe)
        self._ui_tasks = queue.Queue()
        self._build_ui()
        self._start_ui_task_drain()

    def _start_ui_task_drain(self):
        self._ui_task_timer = QTimer(self)
        self._ui_task_timer.timeout.connect(self._drain_ui_tasks)
        self._ui_task_timer.start(16)

    def _run_on_ui(self, fn):
        self._ui_tasks.put(fn)

    def _drain_ui_tasks(self):
        while True:
            try:
                fn = self._ui_tasks.get_nowait()
            except queue.Empty:
                break
            fn()

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
        brand_icon = self._resolve_icon(APP_ICON_NAME)
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
        mode_lbl = QLabel("Simple mode")
        mode_lbl.setObjectName("Hint")
        hh.addWidget(mode_lbl)
        outer.addWidget(header)

        split = QSplitter(Qt.Orientation.Horizontal)
        outer.addWidget(split, 1)

        left = QFrame()
        left.setObjectName("Card")
        lv = QVBoxLayout(left)
        lv.setContentsMargins(8, 8, 8, 8)
        expr_hdr = QLabel("Expression")
        expr_hdr.setObjectName("PanelHeader")
        lv.addWidget(expr_hdr)
        self.input = QLineEdit()
        self.input.setObjectName("CommandPalette")
        self.input.setPlaceholderText("2+3*4")
        lv.addWidget(self.input)
        self.run_btn = QPushButton("Execute")
        lv.addWidget(self.run_btn)
        out_hdr = QLabel("Output")
        out_hdr.setObjectName("PanelHeader")
        lv.addWidget(out_hdr)
        self.output = QPlainTextEdit()
        self.output.setObjectName("CommandConsole")
        self.output.setReadOnly(True)
        lv.addWidget(self.output, 1)
        split.addWidget(left)

        right = QFrame()
        right.setObjectName("Card")
        rv = QVBoxLayout(right)
        rv.setContentsMargins(8, 8, 8, 8)
        quick_hdr = QLabel("Quick Actions")
        quick_hdr.setObjectName("PanelHeader")
        rv.addWidget(quick_hdr)
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
            icon = QIcon(str(icon_path))
            if not icon.isNull():
                return icon
        if icon_name == APP_ICON_NAME:
            return self._build_fallback_axiom_icon()
        return None

    def _build_fallback_axiom_icon(self):
        pixmap = QPixmap(64, 64)
        pixmap.fill(Qt.GlobalColor.transparent)

        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)

        painter.setPen(QPen(QColor("#7ad3ff"), 3))
        painter.setBrush(QColor("#0a1224"))
        painter.drawRoundedRect(6, 6, 52, 52, 12, 12)

        painter.setPen(QPen(QColor("#ffb454"), 4, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin))
        painter.drawLine(18, 46, 30, 16)
        painter.drawLine(30, 16, 34, 16)
        painter.drawLine(34, 16, 46, 46)
        painter.setPen(QPen(QColor("#ffb454"), 3.4, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap))
        painter.drawLine(24, 34, 40, 34)

        painter.setPen(QPen(QColor("#95e6cb"), 2.2, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap))
        wave = QPainterPath()
        wave.moveTo(14, 42)
        wave.cubicTo(17, 37, 20, 37, 23, 42)
        wave.cubicTo(26, 47, 29, 47, 32, 42)
        wave.cubicTo(35, 37, 38, 37, 41, 42)
        wave.cubicTo(44, 47, 47, 47, 50, 42)
        painter.drawPath(wave)

        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QColor("#7ad3ff"))
        painter.drawEllipse(44, 15, 6, 6)

        painter.end()
        return QIcon(pixmap)

    def _prefill(self, expression):
        self.input.setText(expression)

    def _append_result_lines(self, result_text: str):
        text = str(result_text)
        lines = text.splitlines() if text else [NO_OUTPUT_TEXT]
        if not lines:
            self.output.appendPlainText(NO_OUTPUT_TEXT)
            return
        for line in lines:
            self.output.appendPlainText(line if line else " ")

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
                    self._append_result_lines(result.get("result", NO_OUTPUT_TEXT))
                else:
                    self._append_result_lines(result.get("error", EXECUTION_FAILED_TEXT))
            self._run_on_ui(apply_result)

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
