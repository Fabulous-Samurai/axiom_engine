"""
gui/qt/vulkan_widget.py
────────────────────────
AXIOM Vulkan Expressway — 3-D Surface Viewport

Primary render path : PySide6 QOpenGLWidget (hardware iGPU / dGPU, always available)
Upgrade/future path : axiom_vulkan.dll offscreen Vulkan renderer (via vulkan_bridge)

Usage
-----
    from gui.qt.vulkan_widget import VulkanViewport
    dlg = VulkanViewport(parent, expression="sin(sqrt(x**2+y**2)-t)")
    dlg.show()
"""

from __future__ import annotations

import ctypes
import sys
from pathlib import Path

import numpy as np

from PySide6.QtCore import Qt, QTimer, QPoint, Signal
from PySide6.QtGui import (
    QColor, QFont, QPainter, QMatrix4x4, QVector3D,
    QSurfaceFormat,
)
from PySide6.QtOpenGL import (
    QOpenGLBuffer,
    QOpenGLShader,
    QOpenGLShaderProgram,
    QOpenGLVertexArrayObject,
)
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QComboBox, QDialog, QHBoxLayout, QLabel,
    QLineEdit, QPushButton, QSlider, QVBoxLayout, QWidget,
)

from gui.vulkan.mesh_builder import orbit_mvp
from gui.vulkan.sandbox import InputSandbox, SandboxResult

# ── GL constants (ES2 / GL 3.3) that Qt's functions() wrapper exposes ────────
_GL_DEPTH_TEST            = 0x0B71
_GL_CULL_FACE             = 0x0B44
_GL_BACK                  = 0x0405
_GL_COLOR_BUFFER_BIT      = 0x4000
_GL_DEPTH_BUFFER_BIT      = 0x0100
_GL_TRIANGLES             = 0x0004
_GL_UNSIGNED_INT          = 0x1405
_GL_FLOAT                 = 0x1406
_GL_LEQUAL                = 0x0203
_GL_ARRAY_BUFFER          = 0x8892
_GL_ELEMENT_ARRAY_BUFFER  = 0x8893
_GL_DYNAMIC_DRAW          = 0x88E8

# ctypes prototype for raw glBufferData — bypasses PySide6’s 32-bit pointer
# range check which rejects valid 64-bit heap addresses on Windows x64.
_GLBufferDataFn = ctypes.CFUNCTYPE(
    None,             # void return
    ctypes.c_uint,    # GLenum  target
    ctypes.c_int64,   # GLsizeiptr  size
    ctypes.c_void_p,  # const GLvoid*  data
    ctypes.c_uint,    # GLenum  usage
)
# ctypes prototype for raw glVertexAttribPointer — PySide6 6.10 rejects
# integer VBO byte-offsets (even 0) as the ptr argument on Windows x64.
_GLVertexAttribPointerFn = ctypes.CFUNCTYPE(
    None,             # void return
    ctypes.c_uint,    # GLuint  index
    ctypes.c_int,     # GLint   size
    ctypes.c_uint,    # GLenum  type
    ctypes.c_uint8,   # GLboolean normalized
    ctypes.c_int,     # GLsizei stride
    ctypes.c_void_p,  # const GLvoid* pointer (byte offset into VBO)
)

# ctypes prototype for raw glDrawElements — PySide6 6.10 also rejects the
# EBO byte-offset (indices=0) passed as the void* ptr argument.
_GLDrawElementsFn = ctypes.CFUNCTYPE(
    None,             # void return
    ctypes.c_uint,    # GLenum  mode
    ctypes.c_int,     # GLsizei count
    ctypes.c_uint,    # GLenum  type
    ctypes.c_void_p,  # const GLvoid* indices (EBO byte offset)
)

# ctypes prototype for raw glBufferSubData — reuses existing GPU allocation
# without reallocation, eliminating pipeline stalls on same-size uploads.
_GLBufferSubDataFn = ctypes.CFUNCTYPE(
    None,             # void return
    ctypes.c_uint,    # GLenum    target
    ctypes.c_int64,   # GLintptr  offset
    ctypes.c_int64,   # GLsizeiptr size
    ctypes.c_void_p,  # const GLvoid* data
)
# ── GLSL 330 core shaders ──────────────────────────────────────────────────────
_VERT_SRC = """
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;
out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vWorldPos   = aPos;
    vNormal     = aNormal;
    vColor      = aColor;
}
"""

_FRAG_SRC = """
#version 330 core
in  vec3 vColor;
in  vec3 vNormal;
in  vec3 vWorldPos;

uniform vec3  uLightDir;
uniform vec3  uViewPos;
uniform float uAmbient;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.45;
    vec3  lit  = (uAmbient + diff + spec) * vColor;
    FragColor  = vec4(clamp(lit, 0.0, 1.0), 1.0);
}
"""


# ══════════════════════════════════════════════════════════════════════════════
# GPU Surface Renderer — QOpenGLWidget
# ══════════════════════════════════════════════════════════════════════════════
class _SurfaceGLRenderer(QOpenGLWidget):
    """Hardware-accelerated Phong-lit surface widget.

    Camera: orbit (left-drag azimuth/elevation) + scroll wheel zoom.
    Upload mesh at any time via :py:meth:`submit_mesh`.
    """

    fps_updated = Signal(float)
    gl_ready = Signal(str)   # emits OpenGL renderer string when context is ready

    def __init__(self, parent: QWidget | None = None) -> None:
        fmt = QSurfaceFormat()
        fmt.setVersion(3, 3)
        fmt.setProfile(QSurfaceFormat.OpenGLContextProfile.CoreProfile)
        fmt.setDepthBufferSize(24)
        fmt.setSamples(4)
        QSurfaceFormat.setDefaultFormat(fmt)
        super().__init__(parent)

        self._vao: QOpenGLVertexArrayObject | None = None
        self._vbo: QOpenGLBuffer | None = None
        self._ebo: QOpenGLBuffer | None = None
        self._prog: QOpenGLShaderProgram | None = None
        self._index_count: int = 0
        # 2-slot ring buffer — numpy dtype conversion happens in submit_mesh
        # (CPU side, outside paintGL) so the GL upload critical path is a
        # pure SubData pointer copy with no dtype overhead.
        self._mesh_q: list = [None, None]   # slot → (verts_f32, idx_u32) | None
        self._q_write: int = 0              # next write slot; read = 1 - _q_write
        # Raw ctypes wrappers — set up in initializeGL.
        self._raw_buf_data = None
        self._raw_buf_sub = None
        self._raw_vap = None
        self._raw_draw = None
        # GPU buffer sizes (bytes) for SubData vs Data decision.
        self._vbo_cap = 0
        self._ebo_cap = 0
        # Whether VAP+EnableVertexAttrib has been recorded into the VAO yet.
        self._vao_configured = False
        # Cached uniform locations (int) — set after shader link in initializeGL.
        self._u_mvp: int = -1
        self._u_light: int = -1
        self._u_vpos: int = -1
        self._u_amb: int = -1

        # Camera state
        self._azimuth: float = 45.0
        self._elevation: float = 30.0
        self._distance: float = 8.0
        self._drag_start: QPoint | None = None
        self._drag_az0: float = 0.0
        self._drag_el0: float = 0.0

        self._light_dir = np.array([0.577, 0.577, 0.577], dtype=np.float32)
        self._eye = np.array([4.0, 3.0, 4.0], dtype=np.float32)

        # FPS tracking
        self._frame_count: int = 0
        self._fps_timer = QTimer()
        self._fps_timer.timeout.connect(self._report_fps)
        self._fps_timer.start(1000)

    # ── OpenGL lifecycle ──────────────────────────────────────────────────────
    def initializeGL(self) -> None:
        gl = self.context().functions()
        gl.glEnable(_GL_DEPTH_TEST)
        gl.glClearColor(0.06, 0.08, 0.12, 1.0)

        # Compile shader programme
        self._prog = QOpenGLShaderProgram(self)
        ok1 = self._prog.addShaderFromSourceCode(
            QOpenGLShader.ShaderTypeBit.Vertex, _VERT_SRC.strip())
        ok2 = self._prog.addShaderFromSourceCode(
            QOpenGLShader.ShaderTypeBit.Fragment, _FRAG_SRC.strip())
        if not ok1 or not ok2 or not self._prog.link():
            print(f"[VulkanExpressway] Shader link error: {self._prog.log()}")
            return

        # Cache uniform locations as integers — PySide6 6.10 has a runtime bug
        # where setUniformValue(bytes_name, value) raises ValueError even when
        # the signature matches.  The integer-location overload is always safe.
        self._u_mvp   = self._prog.uniformLocation(b"uMVP")
        self._u_light = self._prog.uniformLocation(b"uLightDir")
        self._u_vpos  = self._prog.uniformLocation(b"uViewPos")
        self._u_amb   = self._prog.uniformLocation(b"uAmbient")

        # VAO — records all vertex attribute bindings
        self._vao = QOpenGLVertexArrayObject()
        self._vao.create()

        # VBO (vertex data)
        self._vbo = QOpenGLBuffer(QOpenGLBuffer.Type.VertexBuffer)
        self._vbo.create()
        self._vbo.setUsagePattern(QOpenGLBuffer.UsagePattern.DynamicDraw)

        # EBO (index data)
        self._ebo = QOpenGLBuffer(QOpenGLBuffer.Type.IndexBuffer)
        self._ebo.create()
        self._ebo.setUsagePattern(QOpenGLBuffer.UsagePattern.DynamicDraw)

        # Cache raw GL function pointers via WGL/GLX proc address.
        # PySide6 6.x rejects 64-bit heap pointers and integer VBO offsets,
        # so we call the driver functions directly through ctypes.
        try:
            fn_addr = int(self.context().getProcAddress(b"glBufferData"))
            if fn_addr:
                self._raw_buf_data = _GLBufferDataFn(fn_addr)
        except Exception as exc:
            print(f"[VulkanExpressway] getProcAddress(glBufferData) failed: {exc}")
        try:
            fn_addr = int(self.context().getProcAddress(b"glVertexAttribPointer"))
            if fn_addr:
                self._raw_vap = _GLVertexAttribPointerFn(fn_addr)
        except Exception as exc:
            print(f"[VulkanExpressway] getProcAddress(glVertexAttribPointer) failed: {exc}")
        try:
            fn_addr = int(self.context().getProcAddress(b"glDrawElements"))
            if fn_addr:
                self._raw_draw = _GLDrawElementsFn(fn_addr)
        except Exception as exc:
            print(f"[VulkanExpressway] getProcAddress(glDrawElements) failed: {exc}")
        try:
            fn_addr = int(self.context().getProcAddress(b"glBufferSubData"))
            if fn_addr:
                self._raw_buf_sub = _GLBufferSubDataFn(fn_addr)
        except Exception as exc:
            print(f"[VulkanExpressway] getProcAddress(glBufferSubData) failed: {exc}")

        # Pre-allocate VBO/EBO to the maximum sandbox resolution (MAX_N=192)
        # so every mesh update uses glBufferSubData — no GPU memory realloc,
        # no pipeline stall, regardless of the grid resolution chosen.
        # Both buffers together are < 2 MB, trivially small for any GPU.
        if self._raw_buf_data is not None:
            _MAX_VBO = 192 * 192 * 9 * 4   # 1 327 104 bytes
            _MAX_EBO = 191 * 191 * 6 * 4   #   877 416 bytes
            self._vbo.bind()
            self._raw_buf_data(_GL_ARRAY_BUFFER, _MAX_VBO, None, _GL_DYNAMIC_DRAW)
            self._ebo.bind()
            self._raw_buf_data(_GL_ELEMENT_ARRAY_BUFFER, _MAX_EBO, None, _GL_DYNAMIC_DRAW)
            self._vbo_cap = _MAX_VBO
            self._ebo_cap = _MAX_EBO

        # Report the actual hardware renderer string for the GPU status label
        try:
            renderer_raw = gl.glGetString(0x1F01)  # GL_RENDERER
            if renderer_raw:
                name = (
                    renderer_raw.decode("utf-8", errors="replace")
                    if isinstance(renderer_raw, bytes)
                    else str(renderer_raw)
                )
                self.gl_ready.emit(name)
        except Exception:
            pass

    def resizeGL(self, w: int, h: int) -> None:
        gl = self.context().functions()
        gl.glViewport(0, 0, w, h)

    def paintGL(self) -> None:
        gl = self.context().functions()
        gl.glClear(_GL_COLOR_BUFFER_BIT | _GL_DEPTH_BUFFER_BIT)

        # Ring buffer: upload from the most recently completed write slot.
        # read_slot = 1 − _q_write always points to the last written slot.
        read_slot = 1 - self._q_write
        pair = self._mesh_q[read_slot]
        if pair is not None and self._vao is not None:
            try:
                self._upload_now(*pair)
            except Exception as exc:
                print(f"[VulkanExpressway] _upload_now failed: {exc}")
            finally:
                self._mesh_q[read_slot] = None  # consume; don’t re-upload same data

        if self._index_count == 0 or self._prog is None or self._vao is None or self._ebo is None or self._raw_draw is None:
            return

        w, h = self.width(), self.height()
        mvp, eye = orbit_mvp(
            self._azimuth, self._elevation, self._distance, w, h)
        self._eye = eye

        self._prog.bind()
        flat = mvp.flatten().tolist()
        qmvp = QMatrix4x4(
            flat[0], flat[1], flat[2], flat[3],
            flat[4], flat[5], flat[6], flat[7],
            flat[8], flat[9], flat[10], flat[11],
            flat[12], flat[13], flat[14], flat[15],
        )
        # Use integer-location overloads — the bytes-name overloads raise
        # ValueError at runtime in PySide6 6.10 even on matching signatures.
        self._prog.setUniformValue(self._u_mvp,   qmvp)
        self._prog.setUniformValue(self._u_light, QVector3D(*self._light_dir.tolist()))
        self._prog.setUniformValue(self._u_vpos,  QVector3D(*eye.tolist()))
        self._prog.setUniformValue(self._u_amb,   0.15)

        # VAO restores vertex attribute state + EBO binding
        binder = self._vao.bind()  # QOpenGLVertexArrayObject.Binder
        self._ebo.bind()
        # glDrawElements via raw ctypes — PySide6 6.10 rejects EBO offset 0
        # as a void* argument even when the signature matches.
        self._raw_draw(_GL_TRIANGLES, self._index_count, _GL_UNSIGNED_INT, None)
        self._ebo.release()
        del binder
        self._prog.release()

        self._frame_count += 1

    # ── Mesh upload ───────────────────────────────────────────────────────────
    def submit_mesh(self, verts: np.ndarray, indices: np.ndarray) -> None:
        """Queue a mesh for upload on the next paintGL call.

        numpy dtype conversion is done here (CPU side, outside paintGL) so
        the GPU upload critical path is a pure pointer copy with no overhead.
        Latest-wins: if two results arrive before paintGL runs, only the
        newest is uploaded (the older slot is overwritten).
        """
        slot = self._q_write
        self._mesh_q[slot] = (
            np.ascontiguousarray(verts,   dtype=np.float32),
            np.ascontiguousarray(indices, dtype=np.uint32),
        )
        self._q_write = 1 - slot   # advance ring pointer
        self.update()

    def _upload_now(self, verts: np.ndarray, indices: np.ndarray) -> None:
        """Must be called with GL context current.

        Both arrays are guaranteed contiguous float32 / uint32 by submit_mesh.
        """
        if self._vao is None or self._vbo is None or self._ebo is None:
            return
        if self._raw_buf_data is None:
            raise RuntimeError(
                "glBufferData function pointer not available — "
                "GL context may not be current during initializeGL"
            )

        gl = self.context().functions()
        stride = 9 * 4  # 9 floats × 4 bytes = 36

        vptr = verts.ctypes.data_as(ctypes.c_void_p)
        iptr = indices.ctypes.data_as(ctypes.c_void_p)

        # Bind VAO — records vertex attribute state + EBO binding.
        self._vao.bind()
        self._vbo.bind()

        # Pre-allocation in initializeGL guarantees the buffer is always large
        # enough; SubData reuses GPU allocation with no pipeline stall.
        if self._raw_buf_sub is not None and verts.nbytes <= self._vbo_cap:
            self._raw_buf_sub(_GL_ARRAY_BUFFER, 0, verts.nbytes, vptr)
        else:
            self._raw_buf_data(_GL_ARRAY_BUFFER, verts.nbytes, vptr, _GL_DYNAMIC_DRAW)
            self._vbo_cap = verts.nbytes

        # Record vertex attribute layout into VAO only once — layout (stride,
        # offsets, types) never changes between meshes.
        if not self._vao_configured:
            if self._raw_vap is None:
                raise RuntimeError("glVertexAttribPointer function pointer not available")
            self._raw_vap(0, 3, _GL_FLOAT, 0, stride, None)                   # position
            gl.glEnableVertexAttribArray(0)
            self._raw_vap(1, 3, _GL_FLOAT, 0, stride, ctypes.c_void_p(12))    # normal
            gl.glEnableVertexAttribArray(1)
            self._raw_vap(2, 3, _GL_FLOAT, 0, stride, ctypes.c_void_p(24))    # colour
            gl.glEnableVertexAttribArray(2)

        self._ebo.bind()
        if self._raw_buf_sub is not None and indices.nbytes <= self._ebo_cap:
            self._raw_buf_sub(_GL_ELEMENT_ARRAY_BUFFER, 0, indices.nbytes, iptr)
        else:
            self._raw_buf_data(_GL_ELEMENT_ARRAY_BUFFER, indices.nbytes, iptr, _GL_DYNAMIC_DRAW)
            self._ebo_cap = indices.nbytes

        self._vao_configured = True
        self._index_count = int(len(indices))

    # ── Mouse / wheel camera control ──────────────────────────────────────────
    def mousePressEvent(self, event) -> None:
        if event.button() == Qt.MouseButton.LeftButton:
            self._drag_start = event.pos()
            self._drag_az0 = self._azimuth
            self._drag_el0 = self._elevation

    def mouseReleaseEvent(self, event) -> None:
        if event.button() == Qt.MouseButton.LeftButton:
            self._drag_start = None

    def mouseMoveEvent(self, event) -> None:
        if self._drag_start is None:
            return
        dx = event.pos().x() - self._drag_start.x()
        dy = event.pos().y() - self._drag_start.y()
        self._azimuth = self._drag_az0 - dx * 0.4
        self._elevation = max(-89.0, min(89.0, self._drag_el0 + dy * 0.4))
        self.update()

    def wheelEvent(self, event) -> None:
        delta = event.angleDelta().y()
        self._distance = max(1.0, min(50.0, self._distance - delta * 0.01))
        self.update()

    def _report_fps(self) -> None:
        self.fps_updated.emit(float(self._frame_count))
        self._frame_count = 0


# ══════════════════════════════════════════════════════════════════════════════
# VulkanViewport — top-level dialog
# ══════════════════════════════════════════════════════════════════════════════
# Module-level cache: VulkanBridge() init is slow; check availability only once.
_VULKAN_AVAILABLE: bool | None = None

# Preset expressions
_PRESETS = [
    "sin(sqrt(x**2 + y**2) - t)",
    "sin(x) * cos(y) * cos(t)",
    "cos(r) * exp(-r / 4)",
    "sin(4*theta + t) * r * exp(-r/3)",
    "x**2 - y**2",
    "sin(x*y) / (x*y + 0.01)",
    "(x**2 + y**2 - 1)**3 - x**2 * y**3",
    "exp(-(x**2+y**2)/2) * cos(5*sqrt(x**2+y**2) - t)",
]


class VulkanViewport(QDialog):
    """AXIOM Vulkan Expressway — interactive 3-D surface viewport.

    Parameters
    ----------
    parent : QWidget, optional
    expression : str
        Initial z=f(x,y,t) expression.
    """

    def __init__(
        self,
        parent: QWidget | None = None,
        expression: str = "sin(sqrt(x**2 + y**2) - t)",
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("AXIOM Vulkan Expressway  |  3-D Surface View")
        self.setMinimumSize(800, 620)
        self.resize(1024, 680)
        self.setWindowFlags(
            Qt.WindowType.Dialog
            | Qt.WindowType.WindowMinimizeButtonHint
            | Qt.WindowType.WindowMaximizeButtonHint
            | Qt.WindowType.WindowCloseButtonHint,
        )

        self._expr = expression
        self._t: float = 0.0
        self._dt: float = 0.04
        self._N: int = 64
        self._cmap: str = "rainbow"
        self._has_t: bool = "t" in expression
        self._mesh_dirty: bool = True

        # Out-of-cap sandbox: isolated process pool + ComplexityGuard
        self._sandbox = InputSandbox()

        self._build_ui()

        # Render clock — runs at 60 fps regardless of eval state.
        # The timer also serves as the sandbox-result poller: poll() is
        # non-blocking so paintGL is never stalled by expression evaluation.
        self._anim_timer = QTimer(self)
        self._anim_timer.timeout.connect(self._tick)
        self._anim_timer.start(16)  # 16 ms ≈ 62.5 fps target

        # Kick off the first eval in the sandbox (non-blocking)
        self._sandbox.submit(self._expr, self._N, self._t, cmap=self._cmap)

    # ── UI construction ───────────────────────────────────────────────────────
    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(6)

        # ── top bar ───────────────────────────────────────────────────────────
        bar = QHBoxLayout()
        bar.setSpacing(6)

        lbl = QLabel("z(x,y,t) =")
        lbl.setFont(QFont("Consolas", 10))

        self._expr_edit = QLineEdit(self._expr)
        self._expr_edit.setFont(QFont("Consolas", 10))
        self._expr_edit.setPlaceholderText("e.g. sin(sqrt(x**2+y**2)-t)")
        self._expr_edit.returnPressed.connect(self._apply_expr)

        self._preset_combo = QComboBox()
        self._preset_combo.addItem("— presets —")
        for p in _PRESETS:
            self._preset_combo.addItem(p, p)
        self._preset_combo.currentIndexChanged.connect(self._on_preset)

        btn_eval = QPushButton("Render")
        btn_eval.setFixedWidth(72)
        btn_eval.clicked.connect(self._apply_expr)

        self._anim_chk = QCheckBox("Animate")
        self._anim_chk.setChecked("t" in self._expr)
        self._anim_chk.toggled.connect(self._on_anim_toggle)

        self._fps_lbl = QLabel("-- fps | GPU")
        self._fps_lbl.setFont(QFont("Consolas", 9))
        gpu_mode = "Vulkan" if self._vulkan_available() else "OpenGL"
        self._gpu_lbl = QLabel(f"[{gpu_mode}]")
        self._gpu_lbl.setFont(QFont("Consolas", 9))
        self._gpu_lbl.setStyleSheet("color: #95e6cb;")

        bar.addWidget(lbl)
        bar.addWidget(self._expr_edit, 2)
        bar.addWidget(self._preset_combo, 1)
        bar.addWidget(btn_eval)

        _cmap_lbl = QLabel("Color:")
        _cmap_lbl.setFont(QFont("Consolas", 9))
        self._cmap_combo = QComboBox()
        self._cmap_combo.setFixedWidth(80)
        for _cm in ("rainbow", "viridis", "plasma", "cool"):
            self._cmap_combo.addItem(_cm)
        self._cmap_combo.currentTextChanged.connect(self._on_cmap)
        bar.addWidget(_cmap_lbl)
        bar.addWidget(self._cmap_combo)

        bar.addWidget(self._anim_chk)
        bar.addStretch(1)
        bar.addWidget(self._fps_lbl)
        bar.addWidget(self._gpu_lbl)
        root.addLayout(bar)

        # ── OpenGL viewport ───────────────────────────────────────────────────
        self._gl = _SurfaceGLRenderer(self)
        self._gl.fps_updated.connect(self._on_fps)
        self._gl.gl_ready.connect(self._on_gl_ready)
        root.addWidget(self._gl, 1)

        # ── bottom controls ───────────────────────────────────────────────────
        ctrl = QHBoxLayout()
        ctrl.setSpacing(10)

        ctrl.addWidget(QLabel("Resolution:"))
        self._res_slider = QSlider(Qt.Orientation.Horizontal)
        self._res_slider.setRange(16, 128)
        self._res_slider.setValue(64)
        self._res_slider.setFixedWidth(120)
        self._res_label = QLabel("64")
        self._res_slider.valueChanged.connect(self._on_res)
        ctrl.addWidget(self._res_slider)
        ctrl.addWidget(self._res_label)

        ctrl.addWidget(QLabel("Speed:"))
        self._speed_slider = QSlider(Qt.Orientation.Horizontal)
        self._speed_slider.setRange(1, 20)
        self._speed_slider.setValue(4)
        self._speed_slider.setFixedWidth(100)
        self._speed_slider.valueChanged.connect(
            lambda v: setattr(self, "_dt", v * 0.01))
        ctrl.addWidget(self._speed_slider)

        ctrl.addStretch(1)

        btn_reset = QPushButton("Reset Camera")
        btn_reset.setFixedWidth(100)
        btn_reset.clicked.connect(self._reset_camera)
        ctrl.addWidget(btn_reset)

        btn_close = QPushButton("Close")
        btn_close.setFixedWidth(72)
        btn_close.clicked.connect(self.close)
        ctrl.addWidget(btn_close)
        root.addLayout(ctrl)

        # ── status bar ────────────────────────────────────────────────────────
        self._status = QLabel(
            "Drag to orbit  ·  Scroll to zoom  ·  Edit expression and press Enter")
        self._status.setFont(QFont("Consolas", 8))
        self._status.setStyleSheet("color: #7ad3ff;")
        root.addWidget(self._status)

    # ── slots ─────────────────────────────────────────────────────────────────
    def _vulkan_available(self) -> bool:
        global _VULKAN_AVAILABLE
        if _VULKAN_AVAILABLE is None:
            try:
                from gui.vulkan.vulkan_bridge import VulkanBridge
                _VULKAN_AVAILABLE = VulkanBridge().available
            except Exception:
                _VULKAN_AVAILABLE = False
        return _VULKAN_AVAILABLE

    def _apply_expr(self) -> None:
        self._expr = self._expr_edit.text().strip() or "0"
        self._has_t = "t" in self._expr
        self._t = 0.0
        error = self._sandbox.submit(self._expr, self._N, self._t, cmap=self._cmap)
        if error:
            self._status.setText(f"⚠  {error}")
        else:
            self._status.setText(f"Computing z = {self._expr} …")

    def _on_preset(self, idx: int) -> None:
        if idx <= 0:
            return
        expr = self._preset_combo.itemData(idx)
        if expr:
            self._expr_edit.setText(expr)
            self._apply_expr()
        self._preset_combo.setCurrentIndex(0)

    def _on_anim_toggle(self, checked: bool) -> None:
        # Timer always runs (it is also the sandbox poller).
        # We only stop/start the render clock if there's truly nothing to show.
        if not self._anim_timer.isActive():
            self._anim_timer.start(16)

    def _on_res(self, value: int) -> None:
        self._N = value
        self._res_label.setText(str(value))
        error = self._sandbox.submit(self._expr, self._N, self._t, cmap=self._cmap)
        if error:
            self._status.setText(f"⚠  {error}")

    def _on_fps(self, fps: float) -> None:
        self._fps_lbl.setText(f"{fps:.0f} fps | GPU")

    def _on_cmap(self, name: str) -> None:
        """Re-evaluate with the newly selected colour map."""
        self._cmap = name
        self._sandbox.submit(self._expr, self._N, self._t, cmap=self._cmap)

    def _on_gl_ready(self, renderer: str) -> None:
        """Update the GPU label with the true OpenGL renderer name."""
        prefix = "Vulkan" if self._vulkan_available() else "OpenGL"
        # Trim at first '/' to drop driver suffixes (e.g. "/PCIe/SSE2")
        short = renderer.split("/")[0].strip()[:32]
        self._gpu_lbl.setText(f"[{prefix} · {short}]")

    def _reset_camera(self) -> None:
        self._gl._azimuth = 45.0
        self._gl._elevation = 30.0
        self._gl._distance = 8.0
        self._gl.update()

    def _tick(self) -> None:
        # 1. Poll FIRST — consume any completed result before a new submit can
        #    overwrite self._future; this ensures no worker result is ever lost.
        result: SandboxResult | None = self._sandbox.poll()
        if result is not None:
            self._gl.submit_mesh(result.verts, result.indices)
            self._update_status(result)

        # 2. Advance time; submit next eval only when the worker is idle so the
        #    process pool is never flooded with stale in-flight frames.
        if self._has_t and self._anim_chk.isChecked():
            self._t += self._dt
            if not self._sandbox.is_evaluating:
                self._sandbox.submit(self._expr, self._N, self._t, cmap=self._cmap)

        # 3. Trigger repaint only when there is something new to show:
        #    animating (t advances each tick) or a fresh mesh just arrived.
        #    Static meshes don't need a continuous 60 fps redraw — camera
        #    drag / scroll events call self.update() directly via mouse events.
        if result is not None or (self._has_t and self._anim_chk.isChecked()):
            self._gl.update()

    def _update_status(self, result: SandboxResult) -> None:
        """Update the status bar from a SandboxResult (may be an error)."""
        if result.error:
            # Show warning but verts/indices already contain last-good fallback.
            busy = " [⧗ evaluating…]" if self._sandbox.is_evaluating else ""
            self._status.setText(f"⚠  {result.error}{busy}")
            return
        tri_count = len(result.indices) // 3
        parts = [
            f"z = {result.expr}",
            f"{result.N}×{result.N} grid",
            f"{tri_count:,} triangles",
            f"t = {self._t:.2f}",
        ]
        if result.eval_ms >= 1.0:
            parts.append(f"eval {result.eval_ms:.0f} ms")
        self._status.setText("  ·  ".join(parts))

    def closeEvent(self, event) -> None:
        self._anim_timer.stop()
        self._sandbox.close()   # gracefully shut down the worker process
        super().closeEvent(event)
