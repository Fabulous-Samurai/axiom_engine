"""P3 — Two-tier variable store for AXIOM PRO workspace.

Tier-1 (count < TIER2_THRESHOLD): plain Python dict — zero overhead compared to
current behaviour.

Tier-2 (count >= TIER2_THRESHOLD): SQLite WAL-mode temp file on disk, allowing
the workspace to grow beyond available RAM (designed for NVMe-backed sessions
with billions of variables).

The store exposes the minimal dict-like interface used by AxiomQtWindow and
WorkspaceTableModel so every call-site continues to work unchanged:
  __setitem__, get, pop, __contains__, __len__, keys(), clear()

When in Tier-2, keys() is capped at TABLE_VIEW_CAP to keep the Qt table
responsive.  All variables remain fully accessible via get() regardless of scale.
"""

import os
import sqlite3
import tempfile
from typing import Any


class LargeScaleVarStore:
    """Transparent two-tier variable store (dict → SQLite WAL at 10M entries).

    Tier-1  < TIER2_THRESHOLD:  plain dict, O(1) everything.
    Tier-2 >= TIER2_THRESHOLD:  SQLite WAL temp file.
      - Writes are batched in a small in-memory buffer (WRITE_BUF_CAP entries)
        before being committed to SQLite in a single transaction.
      - Reads check the write buffer first, then SQLite.
      - keys() is capped at TABLE_VIEW_CAP for UI responsiveness; the full
        dataset is always reachable via get() or all_keys().
    """

    TIER2_THRESHOLD = 10_000_000   # escalate to SQLite at 10M variables
    WRITE_BUF_CAP   = 2_000        # flush write buffer every 2 K entries
    TABLE_VIEW_CAP  = 100_000      # max rows surfaced to WorkspaceTableModel

    # ─────────────────────────────── construction ────────────────────────────

    def __init__(self) -> None:
        self._tier1: dict = {}
        self._conn: sqlite3.Connection | None = None
        self._db_path: str = ""
        self._sql_count: int = 0     # live row count in SQLite (excl. write buf)
        self._write_buf: dict = {}   # pending writes batched before next commit

    # ─────────────────────────────── public dict interface ───────────────────

    def __setitem__(self, key: str, entry: Any) -> None:
        if self._conn is None:
            self._tier1[key] = entry
            if len(self._tier1) >= self.TIER2_THRESHOLD:
                self._escalate_to_sqlite()
        else:
            if key not in self._write_buf:
                # Delete stale SQLite row if it exists
                cur = self._conn.execute("DELETE FROM vars WHERE key=?", (key,))
                self._sql_count -= cur.rowcount   # 1 if deleted, 0 if new key
            self._write_buf[key] = entry
            if len(self._write_buf) >= self.WRITE_BUF_CAP:
                self._flush_write_buf()

    def get(self, key: str, default: Any = None) -> Any:
        if self._conn is None:
            return self._tier1.get(key, default)
        if key in self._write_buf:
            return self._write_buf[key]
        row = self._conn.execute(
            "SELECT type_col,size_col,value_col FROM vars WHERE key=?", (key,)
        ).fetchone()
        if row is None:
            return default
        return _entry_from_row(row)

    def pop(self, key: str, *args: Any) -> Any:
        default = args[0] if args else None
        if self._conn is None:
            return self._tier1.pop(key, default)
        if key in self._write_buf:
            return self._write_buf.pop(key)
        row = self._conn.execute(
            "SELECT type_col,size_col,value_col FROM vars WHERE key=?", (key,)
        ).fetchone()
        if row is None:
            return default
        self._conn.execute("DELETE FROM vars WHERE key=?", (key,))
        self._sql_count -= 1
        return _entry_from_row(row)

    def __contains__(self, key: object) -> bool:
        if self._conn is None:
            return key in self._tier1
        if key in self._write_buf:
            return True
        row = self._conn.execute(
            "SELECT 1 FROM vars WHERE key=?", (key,)
        ).fetchone()
        return row is not None

    def __len__(self) -> int:
        if self._conn is None:
            return len(self._tier1)
        return self._sql_count + len(self._write_buf)

    def keys(self):
        """Return keys for UI display, capped at TABLE_VIEW_CAP."""
        if self._conn is None:
            return self._tier1.keys()
        self._flush_write_buf()
        rows = self._conn.execute(
            f"SELECT key FROM vars ORDER BY rowid LIMIT {self.TABLE_VIEW_CAP}"
        ).fetchall()
        return [r[0] for r in rows]

    def all_keys(self):
        """Generator yielding ALL keys in insertion order (bypasses UI cap)."""
        if self._conn is None:
            yield from self._tier1
            return
        self._flush_write_buf()
        for (k,) in self._conn.execute("SELECT key FROM vars ORDER BY rowid"):
            yield k

    def clear(self) -> None:
        self._tier1.clear()
        self._write_buf.clear()
        if self._conn is not None:
            self._conn.execute("DELETE FROM vars")
            self._conn.commit()
            self._sql_count = 0

    def close(self) -> None:
        """Release the SQLite connection and remove the temp file on shutdown."""
        if self._conn is not None:
            try:
                self._flush_write_buf()
            finally:
                self._conn.close()
                self._conn = None
            try:
                if self._db_path:
                    os.unlink(self._db_path)
            except OSError:
                pass

    # ─────────────────────────────── internal ────────────────────────────────

    def _escalate_to_sqlite(self) -> None:
        """Migrate all tier-1 entries to a SQLite WAL temp file."""
        fd, self._db_path = tempfile.mkstemp(prefix="axiom_vars_", suffix=".db")
        os.close(fd)
        self._conn = sqlite3.connect(self._db_path, check_same_thread=False)
        self._conn.execute("PRAGMA journal_mode=WAL")
        self._conn.execute("PRAGMA synchronous=NORMAL")
        self._conn.execute(
            "CREATE TABLE vars "
            "(key TEXT PRIMARY KEY, type_col TEXT, size_col TEXT, value_col TEXT)"
        )
        rows = [(k, e.type, e.size, e.value) for k, e in self._tier1.items()]
        self._conn.executemany(
            "INSERT OR REPLACE INTO vars VALUES (?,?,?,?)", rows
        )
        self._conn.commit()
        self._sql_count = len(rows)
        self._tier1.clear()   # free RAM — data is now on disk

    def _flush_write_buf(self) -> None:
        if not self._write_buf or self._conn is None:
            return
        rows = [(k, e.type, e.size, e.value) for k, e in self._write_buf.items()]
        self._conn.executemany(
            "INSERT OR REPLACE INTO vars VALUES (?,?,?,?)", rows
        )
        self._conn.commit()
        self._sql_count += len(rows)
        self._write_buf.clear()


def _entry_from_row(row: tuple) -> Any:
    """Reconstruct a VarEntry from a (type_col, size_col, value_col) SQLite row."""
    # Late import avoids circular dependency; VarEntry lives in axiom_qt_gui.
    from gui.qt.axiom_qt_gui import VarEntry
    return VarEntry(row[0], row[1], row[2])
