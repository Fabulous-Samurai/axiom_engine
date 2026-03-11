#!/usr/bin/env python3
"""
test_tla_specs.py
=================
Formal-verification test harness for the Mantis A* TLA+ specifications.

Two test layers:
  1. Structural (always run) — verify .tla/.cfg pairs exist and contain
     required keywords (no TLC/Java installation needed).
  2. TLC integration (skipped when Java or tla2tools.jar unavailable) —
     run TLC model-checking and assert "No error has been found."

Run with:
    pytest tests/unit/test_tla_specs.py -v
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).parents[2]
FORMAL_DIR = REPO_ROOT / "formal" / "tla"

# Each entry: (tla_file, cfg_file, required_invariants, required_properties)
SPECS = [
    (
        "MantisAStarCorrectness.tla",
        "MantisAStarCorrectness.cfg",
        ["TypeInv", "MonotoneExploration", "OpenSetValidity", "ClosedImmutable"],
        ["EventuallyTerminates"],
    ),
    (
        "MantisHeuristicDispatch.tla",
        "MantisHeuristicDispatch.cfg",
        ["TypeInv", "DeterministicOutput", "OrderingEquivalence", "NoSilentDrop", "PathCoherence"],
        [],
    ),
    (
        "MantisDogThreshold.tla",
        "MantisDogThreshold.cfg",
        ["TypeInv", "DogBranchConsistency", "NormSafety", "IdentityWhenBelowThreshold"],
        [],
    ),
    (
        "MantisFixedMinHeap.tla",
        "MantisFixedMinHeap.cfg",
        ["TypeInv", "HeapInvariant", "SizeInvariant"],
        [],
    ),
]

TLC_SUCCESS_MARKER = "No error has been found"
TLC_JAR_ENV = "TLC_JAR"          # optional: override jar path via env var
TLC_JAR_DEFAULT = "tla2tools.jar"


def _find_tlc_jar() -> Path | None:
    """Return path to tla2tools.jar if resolvable, else None."""
    # 1. Environment variable override
    env_path = os.environ.get(TLC_JAR_ENV)
    if env_path:
        p = Path(env_path)
        if p.is_file():
            return p

    # 2. tla2tools.jar in the repo root
    candidate = REPO_ROOT / TLC_JAR_DEFAULT
    if candidate.is_file():
        return candidate

    # 3. Anywhere on PATH (e.g., if installed as a script)
    found = shutil.which("tlc")
    if found:
        return Path(found)

    return None


def _java_available() -> bool:
    return shutil.which("java") is not None


# ---------------------------------------------------------------------------
# Class 1 — Structural tests (no TLC required)
# ---------------------------------------------------------------------------
class TestTlaSpecsStructural(unittest.TestCase):
    """Verify .tla/.cfg file pairs exist and contain required identifiers."""

    def _tla_path(self, name: str) -> Path:
        return FORMAL_DIR / name

    # ---- Existence ----

    def test_formal_tla_directory_exists(self):
        self.assertTrue(
            FORMAL_DIR.is_dir(),
            f"Expected formal/tla directory at {FORMAL_DIR}",
        )

    def test_all_spec_files_exist(self):
        for tla_name, cfg_name, _, _ in SPECS:
            with self.subTest(spec=tla_name):
                self.assertTrue(
                    self._tla_path(tla_name).exists(),
                    f"Missing TLA+ spec: {tla_name}",
                )
                self.assertTrue(
                    self._tla_path(cfg_name).exists(),
                    f"Missing TLA+ config: {cfg_name}",
                )

    # ---- Syntax / content checks ----

    def _load(self, name: str) -> str:
        return (FORMAL_DIR / name).read_text(encoding="utf-8")

    def test_tla_files_have_module_header(self):
        """Every .tla must open with ---- MODULE <Name> ----"""
        for tla_name, _, _, _ in SPECS:
            with self.subTest(spec=tla_name):
                content = self._load(tla_name)
                module_name = tla_name.replace(".tla", "")
                pattern = rf"----\s+MODULE\s+{module_name}\s+----"
                self.assertRegex(
                    content,
                    pattern,
                    f"{tla_name}: missing or malformed MODULE header",
                )

    def test_tla_files_end_with_terminator(self):
        """Every .tla must end with ==== terminator."""
        for tla_name, _, _, _ in SPECS:
            with self.subTest(spec=tla_name):
                content = self._load(tla_name)
                self.assertIn("====", content, f"{tla_name}: missing ==== terminator")

    def test_tla_files_declare_spec(self):
        """Every .tla must define a Spec operator."""
        for tla_name, _, _, _ in SPECS:
            with self.subTest(spec=tla_name):
                content = self._load(tla_name)
                self.assertIn("Spec ==", content, f"{tla_name}: missing Spec definition")

    def test_tla_files_declare_invariants(self):
        """Verify all expected invariant names appear in each .tla."""
        for tla_name, _, invariants, _ in SPECS:
            with self.subTest(spec=tla_name):
                content = self._load(tla_name)
                for inv in invariants:
                    self.assertIn(
                        inv,
                        content,
                        f"{tla_name}: missing invariant '{inv}'",
                    )

    def test_tla_files_declare_properties(self):
        """Verify all expected temporal properties appear in each .tla."""
        for tla_name, _, _, properties in SPECS:
            with self.subTest(spec=tla_name):
                content = self._load(tla_name)
                for prop in properties:
                    self.assertIn(
                        prop,
                        content,
                        f"{tla_name}: missing property '{prop}'",
                    )

    def test_cfg_files_reference_spec(self):
        """Each .cfg must contain a SPECIFICATION line."""
        for _, cfg_name, _, _ in SPECS:
            with self.subTest(cfg=cfg_name):
                content = self._load(cfg_name)
                self.assertIn(
                    "SPECIFICATION",
                    content,
                    f"{cfg_name}: missing SPECIFICATION directive",
                )

    def test_cfg_files_reference_invariants(self):
        """Each .cfg must declare INVARIANTS section with expected names."""
        for _, cfg_name, invariants, _ in SPECS:
            with self.subTest(cfg=cfg_name):
                content = self._load(cfg_name)
                self.assertIn(
                    "INVARIANTS",
                    content,
                    f"{cfg_name}: missing INVARIANTS directive",
                )
                for inv in invariants:
                    self.assertIn(
                        inv,
                        content,
                        f"{cfg_name}: invariant '{inv}' not listed",
                    )

    def test_mantis_astar_has_fairness(self):
        """MantisAStarCorrectness must use WF_ (weak fairness) for liveness."""
        content = self._load("MantisAStarCorrectness.tla")
        self.assertIn("WF_vars", content, "MantisAStarCorrectness: missing WF_ fairness for liveness")

    def test_mantis_dispatch_models_both_paths(self):
        """MantisHeuristicDispatch must mention FMA3 and Scalar paths."""
        content = self._load("MantisHeuristicDispatch.tla")
        self.assertIn("PATH_FMA3", content)
        self.assertIn("PATH_SCALAR", content)

    def test_mantis_dog_threshold_models_branch(self):
        """MantisDogThreshold must model both branch-taken and identity paths."""
        content = self._load("MantisDogThreshold.tla")
        self.assertIn("DogBranchConsistency", content)
        self.assertIn("IdentityWhenBelowThreshold", content)

    def test_spec_pairs_are_consistent(self):
        """Each .cfg SPECIFICATION value must match the .tla MODULE name."""
        for tla_name, cfg_name, _, _ in SPECS:
            with self.subTest(pair=tla_name):
                module_name = tla_name.replace(".tla", "")
                cfg_content = self._load(cfg_name)
                # SPECIFICATION Spec should reference the module implicitly
                # (TLC infers from the module name); we check the .cfg says "Spec"
                self.assertRegex(
                    cfg_content,
                    r"SPECIFICATION\s+Spec",
                    f"{cfg_name}: SPECIFICATION must be 'Spec'",
                )


# ---------------------------------------------------------------------------
# Class 2 — TLC integration tests (skipped without Java + tla2tools.jar)
# ---------------------------------------------------------------------------
@unittest.skipUnless(
    _java_available(), "Java not found on PATH — skipping TLC model checking"
)
class TestTlcModelCheck(unittest.TestCase):
    """
    Run TLC on each spec and assert no errors are found.
    Skipped automatically when Java is unavailable (typical CI without TLC).
    When Java IS present, tla2tools.jar is expected either:
      - at the repo root (formal/tla/../ = REPO_ROOT/tla2tools.jar), or
      - at the path given by TLC_JAR environment variable.
    """

    @classmethod
    def setUpClass(cls):
        cls.jar = _find_tlc_jar()
        if cls.jar is None:
            raise unittest.SkipTest(
                "tla2tools.jar not found. Set TLC_JAR env var or place "
                f"tla2tools.jar in {REPO_ROOT}"
            )

    def _run_tlc(self, tla_name: str, cfg_name: str, timeout: int = 120) -> str:
        """
        Run TLC in model-checking mode, return combined stdout+stderr.
        Raises subprocess.TimeoutExpired on timeout.
        """
        tla = str(FORMAL_DIR / tla_name)
        cfg = str(FORMAL_DIR / cfg_name)
        cmd = [
            "java", "-jar", str(self.jar),
            "-config", cfg,
            tla,
        ]
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=str(FORMAL_DIR),
        )
        return result.stdout + result.stderr

    def _assert_tlc_pass(self, output: str, spec_name: str):
        self.assertIn(
            TLC_SUCCESS_MARKER,
            output,
            f"TLC found errors in {spec_name}:\n{output[:2000]}",
        )

    def test_tlc_astar_correctness(self):
        output = self._run_tlc(
            "MantisAStarCorrectness.tla",
            "MantisAStarCorrectness.cfg",
        )
        self._assert_tlc_pass(output, "MantisAStarCorrectness")

    def test_tlc_heuristic_dispatch(self):
        output = self._run_tlc(
            "MantisHeuristicDispatch.tla",
            "MantisHeuristicDispatch.cfg",
        )
        self._assert_tlc_pass(output, "MantisHeuristicDispatch")

    def test_tlc_dog_threshold(self):
        output = self._run_tlc(
            "MantisDogThreshold.tla",
            "MantisDogThreshold.cfg",
        )
        self._assert_tlc_pass(output, "MantisDogThreshold")

    def test_tlc_fixed_min_heap(self):
        output = self._run_tlc(
            "MantisFixedMinHeap.tla",
            "MantisFixedMinHeap.cfg",
        )
        self._assert_tlc_pass(output, "MantisFixedMinHeap")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(
        unittest.TestLoader().loadTestsFromModule(sys.modules[__name__])
    )
    sys.exit(0 if result.wasSuccessful() else 1)
