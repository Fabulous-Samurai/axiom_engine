#!/usr/bin/env python3
"""Unit tests for scripts/download_cicd_logs.py."""
from __future__ import annotations

import io
import os
import sys
import unittest
import zipfile
from typing import Any, Dict
from unittest.mock import MagicMock, patch

# Allow import from the scripts directory regardless of working directory
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "scripts"))

from download_cicd_logs import (
    _auth_headers,
    list_jobs,
    list_workflow_runs,
    main,
    save_run_logs,
)


def _make_zip(files: Dict[str, bytes]) -> bytes:
    """Build an in-memory ZIP archive from a dict of {name: content}."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zf:
        for name, data in files.items():
            zf.writestr(name, data)
    return buf.getvalue()


class TestAuthHeaders(unittest.TestCase):
    def test_bearer_token_format(self):
        headers = _auth_headers("mytoken")
        self.assertEqual(headers["Authorization"], "Bearer mytoken")
        self.assertIn("application/vnd.github+json", headers["Accept"])
        self.assertIn("X-GitHub-Api-Version", headers)

    def test_empty_token(self):
        headers = _auth_headers("")
        self.assertEqual(headers["Authorization"], "Bearer ")


class TestListWorkflowRuns(unittest.TestCase):
    def _make_response(self, runs):
        mock_resp = MagicMock()
        mock_resp.raise_for_status = MagicMock()
        mock_resp.json.return_value = {"workflow_runs": runs}
        return mock_resp

    @patch("download_cicd_logs.requests.get")
    def test_returns_workflow_runs(self, mock_get):
        mock_get.return_value = self._make_response(
            [{"id": 1, "status": "completed", "conclusion": "success"}]
        )
        runs = list_workflow_runs("owner/repo", "token")
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0]["id"], 1)

    @patch("download_cicd_logs.requests.get")
    def test_workflow_filter_uses_workflow_url(self, mock_get):
        mock_get.return_value = self._make_response([])
        list_workflow_runs("owner/repo", "token", workflow="ci.yml")
        called_url = mock_get.call_args[0][0]
        self.assertIn("workflows/ci.yml/runs", called_url)

    @patch("download_cicd_logs.requests.get")
    def test_no_workflow_filter_uses_runs_url(self, mock_get):
        mock_get.return_value = self._make_response([])
        list_workflow_runs("owner/repo", "token")
        called_url = mock_get.call_args[0][0]
        self.assertIn("/actions/runs", called_url)
        self.assertNotIn("workflows", called_url)

    @patch("download_cicd_logs.requests.get")
    def test_branch_and_status_passed_as_params(self, mock_get):
        mock_get.return_value = self._make_response([])
        list_workflow_runs("owner/repo", "token", branch="main", status="completed")
        params = mock_get.call_args[1]["params"]
        self.assertEqual(params["branch"], "main")
        self.assertEqual(params["status"], "completed")

    @patch("download_cicd_logs.requests.get")
    def test_empty_response_returns_empty_list(self, mock_get):
        mock_get.return_value = self._make_response([])
        runs = list_workflow_runs("owner/repo", "token")
        self.assertEqual(runs, [])


class TestListJobs(unittest.TestCase):
    def _make_paged_responses(self, pages):
        """Return a sequence of mock responses, one per paginated call."""
        total = sum(len(p) for p in pages)
        responses = []
        for page in pages:
            mock_resp = MagicMock()
            mock_resp.raise_for_status = MagicMock()
            mock_resp.json.return_value = {"jobs": page, "total_count": total}
            responses.append(mock_resp)
        return responses

    @patch("download_cicd_logs.requests.get")
    def test_returns_all_jobs(self, mock_get):
        mock_get.side_effect = self._make_paged_responses(
            [[{"id": 1, "name": "Build"}, {"id": 2, "name": "Test"}]]
        )
        jobs = list_jobs("owner/repo", "token", run_id=42)
        self.assertEqual(len(jobs), 2)
        self.assertEqual(jobs[0]["name"], "Build")

    @patch("download_cicd_logs.requests.get")
    def test_empty_jobs(self, mock_get):
        mock_resp = MagicMock()
        mock_resp.raise_for_status = MagicMock()
        mock_resp.json.return_value = {"jobs": [], "total_count": 0}
        mock_get.return_value = mock_resp
        jobs = list_jobs("owner/repo", "token", run_id=99)
        self.assertEqual(jobs, [])


class TestSaveRunLogs(unittest.TestCase):
    def test_extracts_files_to_run_subdirectory(self):
        import tempfile

        zip_bytes = _make_zip(
            {
                "job1/0_Set up job.txt": b"step log line 1\nstep log line 2",
                "job1/1_Build.txt": b"build output",
            }
        )
        with tempfile.TemporaryDirectory() as tmp:
            written = save_run_logs(zip_bytes, tmp, run_id=123)
            self.assertEqual(len(written), 2)
            run_dir = os.path.join(tmp, "123")
            self.assertTrue(os.path.isdir(run_dir))
            # Check that both extracted files exist
            for path in written:
                self.assertTrue(os.path.isfile(path))

    def test_creates_output_directory(self):
        import tempfile

        zip_bytes = _make_zip({"a.txt": b"hello"})
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = os.path.join(tmp, "new_subdir")
            save_run_logs(zip_bytes, out_dir, run_id=1)
            self.assertTrue(os.path.isdir(os.path.join(out_dir, "1")))

    def test_path_traversal_is_blocked(self):
        import tempfile

        # Craft a ZIP with a path-traversal entry
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            zf.writestr("../../evil.txt", b"malicious")
        zip_bytes = buf.getvalue()

        with tempfile.TemporaryDirectory() as tmp:
            written = save_run_logs(zip_bytes, tmp, run_id=7)
            # The traversal entry must be skipped
            self.assertEqual(written, [])
            evil_path = os.path.join(os.path.dirname(tmp), "evil.txt")
            self.assertFalse(os.path.exists(evil_path))

    def test_directory_entries_are_not_written_as_files(self):
        import tempfile

        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            zf.writestr("subdir/", b"")  # directory entry
            zf.writestr("subdir/file.txt", b"content")
        zip_bytes = buf.getvalue()

        with tempfile.TemporaryDirectory() as tmp:
            written = save_run_logs(zip_bytes, tmp, run_id=5)
            self.assertEqual(len(written), 1)
            self.assertTrue(written[0].endswith("file.txt"))


class TestMain(unittest.TestCase):
    def _run_main(self, extra_args, mock_get_side_effects):
        with patch("download_cicd_logs.requests.get") as mock_get:
            mock_get.side_effect = mock_get_side_effects
            return main(["--repo", "owner/repo", "--token", "tok"] + extra_args)

    def _make_resp(self, data: Any, status_code: int = 200):
        mock_resp = MagicMock()
        mock_resp.raise_for_status = MagicMock()
        mock_resp.json.return_value = data
        mock_resp.status_code = status_code
        return mock_resp

    def test_invalid_repo_format_returns_1(self):
        ret = main(["--repo", "badrepo", "--token", "tok"])
        self.assertEqual(ret, 1)

    def test_list_flag_prints_runs_and_returns_0(self):
        runs_resp = self._make_resp({
            "workflow_runs": [
                {
                    "id": 9,
                    "status": "completed",
                    "conclusion": "success",
                    "head_branch": "master",
                    "event": "push",
                    "created_at": "2026-01-01T00:00:00Z",
                }
            ]
        })
        with patch("download_cicd_logs.requests.get", return_value=runs_resp):
            ret = main(["--repo", "owner/repo", "--token", "tok", "--list"])
        self.assertEqual(ret, 0)

    def test_no_runs_returns_0(self):
        resp = self._make_resp({"workflow_runs": []})
        with patch("download_cicd_logs.requests.get", return_value=resp):
            ret = main(["--repo", "owner/repo", "--token", "tok"])
        self.assertEqual(ret, 0)

    def test_http_error_on_list_returns_2(self):
        import requests as req

        mock_resp = MagicMock()
        mock_resp.raise_for_status.side_effect = req.HTTPError("403")
        with patch("download_cicd_logs.requests.get", return_value=mock_resp):
            ret = main(["--repo", "owner/repo", "--token", "tok"])
        self.assertEqual(ret, 2)

    def test_download_latest_run(self):
        import tempfile

        zip_bytes = _make_zip({"job/step.txt": b"log content"})

        runs_resp = self._make_resp({
            "workflow_runs": [
                {
                    "id": 42,
                    "status": "completed",
                    "conclusion": "success",
                    "head_branch": "master",
                    "event": "push",
                    "created_at": "2026-01-01T00:00:00Z",
                }
            ]
        })
        jobs_resp = self._make_resp({"jobs": [{"id": 1, "name": "Build"}], "total_count": 1})
        logs_resp = MagicMock()
        logs_resp.raise_for_status = MagicMock()
        logs_resp.content = zip_bytes

        with tempfile.TemporaryDirectory() as tmp:
            with patch(
                "download_cicd_logs.requests.get",
                side_effect=[runs_resp, jobs_resp, logs_resp],
            ):
                ret = main(["--repo", "owner/repo", "--token", "tok", "--out", tmp])
            self.assertEqual(ret, 0)
            run_dir = os.path.join(tmp, "42")
            self.assertTrue(os.path.isdir(run_dir))


if __name__ == "__main__":
    unittest.main()
