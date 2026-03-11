#!/usr/bin/env python3
"""Download GitHub Actions CI/CD workflow run logs.

Usage:
  # Download logs for the latest workflow run:
  python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN

  # Download logs for a specific run ID:
  python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --run-id 123456789

  # Download logs for a specific workflow file:
  python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --workflow ci.yml

  # Save the log archive to a custom path:
  python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --out ./logs

The script uses the GitHub REST API to list recent workflow runs and downloads
the log archive (ZIP) for the chosen run into the specified output directory.
Individual job logs are also extracted and saved as plain text files.
"""
from __future__ import annotations

import argparse
import io
import os
import sys
import zipfile
from typing import Any, Dict, List, Optional
from urllib.parse import urljoin

import requests

GITHUB_API = "https://api.github.com"


def _auth_headers(token: str) -> Dict[str, str]:
    return {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def list_workflow_runs(
    repo: str,
    token: str,
    workflow: Optional[str] = None,
    branch: Optional[str] = None,
    status: Optional[str] = None,
    per_page: int = 10,
) -> List[Dict[str, Any]]:
    """Return recent workflow runs for *repo*.

    Args:
        repo: ``owner/name`` repository slug.
        token: GitHub personal access token.
        workflow: Optional workflow file name (e.g. ``ci.yml``) or numeric ID.
        branch: Optional branch filter.
        status: Optional run status filter (e.g. ``completed``, ``failure``).
        per_page: Maximum number of runs to return (1-100).
    """
    if workflow:
        url = f"{GITHUB_API}/repos/{repo}/actions/workflows/{workflow}/runs"
    else:
        url = f"{GITHUB_API}/repos/{repo}/actions/runs"

    params: Dict[str, Any] = {"per_page": per_page}
    if branch:
        params["branch"] = branch
    if status:
        params["status"] = status

    r = requests.get(url, headers=_auth_headers(token), params=params, timeout=30)
    r.raise_for_status()
    return r.json().get("workflow_runs", [])


def list_jobs(repo: str, token: str, run_id: int) -> List[Dict[str, Any]]:
    """Return all jobs for the given workflow run."""
    url = f"{GITHUB_API}/repos/{repo}/actions/runs/{run_id}/jobs"
    jobs: List[Dict[str, Any]] = []
    page = 1
    while True:
        r = requests.get(
            url,
            headers=_auth_headers(token),
            params={"per_page": 100, "page": page},
            timeout=30,
        )
        r.raise_for_status()
        data = r.json()
        batch = data.get("jobs", [])
        if not batch:
            break
        jobs.extend(batch)
        if len(jobs) >= data.get("total_count", len(jobs)):
            break
        page += 1
    return jobs


def download_run_logs(repo: str, token: str, run_id: int) -> bytes:
    """Download the log archive (ZIP) for a workflow run and return raw bytes."""
    url = f"{GITHUB_API}/repos/{repo}/actions/runs/{run_id}/logs"
    r = requests.get(
        url,
        headers=_auth_headers(token),
        allow_redirects=True,
        timeout=60,
    )
    r.raise_for_status()
    return r.content


def download_job_log(repo: str, token: str, job_id: int) -> str:
    """Download the plain-text log for a single job and return it as a string."""
    url = f"{GITHUB_API}/repos/{repo}/actions/jobs/{job_id}/logs"
    r = requests.get(
        url,
        headers=_auth_headers(token),
        allow_redirects=True,
        timeout=60,
    )
    r.raise_for_status()
    return r.text


def save_run_logs(zip_bytes: bytes, out_dir: str, run_id: int) -> List[str]:
    """Extract *zip_bytes* into *out_dir* and return a list of written paths."""
    run_dir = os.path.join(out_dir, str(run_id))
    os.makedirs(run_dir, exist_ok=True)
    written: List[str] = []
    with zipfile.ZipFile(io.BytesIO(zip_bytes)) as zf:
        for name in zf.namelist():
            dest = os.path.join(run_dir, name)
            # Prevent path traversal
            abs_dest = os.path.realpath(dest)
            abs_run_dir = os.path.realpath(run_dir)
            if not abs_dest.startswith(abs_run_dir + os.sep) and abs_dest != abs_run_dir:
                print(f"  Skipping unsafe path: {name}", file=sys.stderr)
                continue
            if name.endswith("/"):
                os.makedirs(dest, exist_ok=True)
            else:
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with zf.open(name) as src, open(dest, "wb") as dst:
                    dst.write(src.read())
                written.append(dest)
    return written


def main(argv: Optional[List[str]] = None) -> int:
    p = argparse.ArgumentParser(description="Download GitHub Actions CI/CD workflow run logs")
    p.add_argument("--repo", required=True, help="Repository slug: owner/name")
    p.add_argument("--token", required=True, help="GitHub personal access token (needs actions:read)")
    p.add_argument("--run-id", type=int, default=None, help="Workflow run ID to download (default: latest)")
    p.add_argument("--workflow", default=None, help="Workflow file name or ID to filter runs, e.g. ci.yml")
    p.add_argument("--branch", default=None, help="Branch name to filter runs")
    p.add_argument("--status", default=None, help="Run status filter, e.g. completed, failure")
    p.add_argument("--out", default="cicd-logs", help="Output directory for downloaded logs (default: cicd-logs)")
    p.add_argument("--list", action="store_true", help="List recent runs and exit without downloading")
    p.add_argument("--per-page", type=int, default=10, help="Number of runs to list (default: 10)")
    args = p.parse_args(argv)

    # Validate repo slug
    if "/" not in args.repo:
        print("Error: --repo must be in owner/name format", file=sys.stderr)
        return 1

    try:
        runs = list_workflow_runs(
            args.repo,
            args.token,
            workflow=args.workflow,
            branch=args.branch,
            status=args.status,
            per_page=args.per_page,
        )
    except requests.HTTPError as e:
        print(f"Failed to list workflow runs: {e}", file=sys.stderr)
        return 2

    if not runs:
        print("No workflow runs found for the given filters.", file=sys.stderr)
        return 0

    if args.list:
        print(f"{'ID':<12} {'Status':<12} {'Conclusion':<12} {'Branch':<20} {'Event':<15} {'Created'}")
        print("-" * 85)
        for run in runs:
            print(
                f"{run['id']:<12} {run['status']:<12} {str(run.get('conclusion') or ''):<12} "
                f"{run.get('head_branch', ''):<20} {run.get('event', ''):<15} {run.get('created_at', '')}"
            )
        return 0

    if args.run_id:
        run = next((r for r in runs if r["id"] == args.run_id), None)
        if run is None:
            # Try to fetch the run directly even if it wasn't in the listing page
            try:
                url = f"{GITHUB_API}/repos/{args.repo}/actions/runs/{args.run_id}"
                r = requests.get(url, headers=_auth_headers(args.token), timeout=30)
                r.raise_for_status()
                run = r.json()
            except requests.HTTPError as e:
                print(f"Run {args.run_id} not found: {e}", file=sys.stderr)
                return 2
    else:
        run = runs[0]

    run_id: int = run["id"]
    print(
        f"Downloading logs for run #{run_id} "
        f"(status={run.get('status')}, conclusion={run.get('conclusion')}, "
        f"branch={run.get('head_branch')}, event={run.get('event')})"
    )

    try:
        jobs = list_jobs(args.repo, args.token, run_id)
    except requests.HTTPError as e:
        print(f"Warning: could not list jobs: {e}", file=sys.stderr)
        jobs = []

    if jobs:
        print(f"  {len(jobs)} job(s): {', '.join(j['name'] for j in jobs)}")

    try:
        zip_bytes = download_run_logs(args.repo, args.token, run_id)
    except requests.HTTPError as e:
        print(f"Failed to download run logs: {e}", file=sys.stderr)
        return 2

    os.makedirs(args.out, exist_ok=True)
    written = save_run_logs(zip_bytes, args.out, run_id)
    print(f"Extracted {len(written)} log file(s) to {os.path.join(args.out, str(run_id))}/")

    for path in written:
        print(f"  {path}")

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
