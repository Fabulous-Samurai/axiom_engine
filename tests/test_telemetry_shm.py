"""
tests/test_telemetry_shm.py
Verifies the Python SHM telemetry bridge:
  - struct layout (sizeof, field offsets)
  - graceful connect / is_connected / close lifecycle
  - live read-back via a simulated C++ writer using ctypes in-process
"""
import ctypes
import mmap
import sys
import pytest

# ---------------------------------------------------------------------------
# Bring the module under test onto the path
# ---------------------------------------------------------------------------
from gui.qt.telemetry_reader import AxiomTelemetry, TelemetryShmReader

_SHM_NAME_WIN   = "Local\\axiom_telemetry_shm"
_EXPECTED_BYTES = 64


# ---------------------------------------------------------------------------
# 1. Struct layout
# ---------------------------------------------------------------------------
class TestAxiomTelemetryLayout:
    def test_sizeof(self):
        assert ctypes.sizeof(AxiomTelemetry) == _EXPECTED_BYTES, (
            f"Expected 64 bytes, got {ctypes.sizeof(AxiomTelemetry)}"
        )

    def test_field_offsets(self):
        expected = [
            ("fast_path_ns",      0,  8),
            ("ipc_latency_ns",    8,  8),
            ("transfer_ns",      16,  8),
            ("last_eval_ms",     24,  8),
            ("block_memo_hits",  32,  8),
            ("block_memo_misses",40,  8),
            ("write_seq",        48,  8),
            ("_pad",             56,  8),
        ]
        for name, exp_offset, exp_size in expected:
            desc = getattr(AxiomTelemetry, name)
            assert desc.offset == exp_offset, (
                f"{name}: expected offset {exp_offset}, got {desc.offset}"
            )
            assert desc.size == exp_size, (
                f"{name}: expected size {exp_size}, got {desc.size}"
            )

    def test_pack_no_extra_padding(self):
        """_pack_ = 1 must suppress any implicit padding."""
        total = sum(
            getattr(AxiomTelemetry, field[0]).size
            for field in AxiomTelemetry._fields_
        )
        assert total == _EXPECTED_BYTES


# ---------------------------------------------------------------------------
# 2. Reader lifecycle
# ---------------------------------------------------------------------------
class TestTelemetryShmReaderLifecycle:
    def test_connects_on_windows(self):
        """On Windows mmap creates the region if absent; must connect."""
        if sys.platform != "win32":
            pytest.skip("Windows-only test")
        reader = TelemetryShmReader()
        assert reader.is_connected()
        assert reader.snapshot is not None
        reader.close()

    def test_snapshot_is_none_after_close(self):
        if sys.platform != "win32":
            pytest.skip("Windows-only test")
        reader = TelemetryShmReader()
        reader.close()
        assert reader.snapshot is None
        assert not reader.is_connected()

    def test_double_close_safe(self):
        reader = TelemetryShmReader()
        reader.close()
        reader.close()  # must not raise

    def test_try_reconnect_returns_bool(self):
        reader = TelemetryShmReader()
        result = reader.try_reconnect()
        assert isinstance(result, bool)
        reader.close()


# ---------------------------------------------------------------------------
# 3. Live read-back: write into the SHM region via a second mmap handle,
#    confirm the reader sees the new values without any copy or IPC.
# ---------------------------------------------------------------------------
class TestTelemetryShmLiveReadback:
    @pytest.mark.skipif(sys.platform != "win32", reason="Windows named-shm only")
    def test_zero_copy_readback(self):
        """
        Open a second mmap handle to the same Windows named mapping and write
        known values to it directly (simulating what the C++ writer would do).
        Confirm the reader's snapshot reflects them immediately.
        """
        reader = TelemetryShmReader()
        assert reader.is_connected(), "Could not connect to SHM region"

        # Independent writer handle to the same region
        writer_map = mmap.mmap(
            -1,
            _EXPECTED_BYTES,
            tagname=_SHM_NAME_WIN,
            access=mmap.ACCESS_WRITE,
        )
        writer_tel = AxiomTelemetry.from_buffer(writer_map)

        # Write known sentinel values
        writer_tel.fast_path_ns      = 4.25
        writer_tel.ipc_latency_ns    = 37.75
        writer_tel.transfer_ns       = 15.5
        writer_tel.last_eval_ms      = 2.1
        writer_tel.block_memo_hits   = 99
        writer_tel.block_memo_misses = 7
        writer_tel.write_seq         = 42

        # Reader snapshot must reflect the writes immediately (same process,
        # same physical pages — no cache-miss on x86-64 TSO memory model).
        snap = reader.snapshot
        assert snap is not None
        assert snap.fast_path_ns      == pytest.approx(4.25)
        assert snap.ipc_latency_ns    == pytest.approx(37.75)
        assert snap.transfer_ns       == pytest.approx(15.5)
        assert snap.last_eval_ms      == pytest.approx(2.1)
        assert snap.block_memo_hits   == 99
        assert snap.block_memo_misses == 7
        assert snap.write_seq         == 42

        del writer_tel  # release from_buffer pin before closing the mmap
        writer_map.close()
        reader.close()

    @pytest.mark.skipif(sys.platform != "win32", reason="Windows named-shm only")
    def test_memo_hit_rate_calculation(self):
        """
        Verify that when SHM has authoritative counters the hit-rate formula
        matches the expected value.
        """
        writer_map = mmap.mmap(
            -1,
            _EXPECTED_BYTES,
            tagname=_SHM_NAME_WIN,
            access=mmap.ACCESS_WRITE,
        )
        writer_tel = AxiomTelemetry.from_buffer(writer_map)
        writer_tel.block_memo_hits   = 75
        writer_tel.block_memo_misses = 25   # 75 / 100 = 75%

        reader = TelemetryShmReader()
        snap = reader.snapshot
        assert snap is not None
        total = snap.block_memo_hits + snap.block_memo_misses
        rate  = (snap.block_memo_hits / total * 100.0) if total else 0.0
        assert rate == pytest.approx(75.0)

        del writer_tel  # release from_buffer pin before closing the mmap
        writer_map.close()
        reader.close()
