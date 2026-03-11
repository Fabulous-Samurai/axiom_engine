#!/usr/bin/env python3
"""Sonar coverage analysis for files_with_issues.json

Computes per-file metrics:
 - total_lines
 - issues_count
 - issue_lines_count (unique line numbers from issues)
 - issues_per_kloc
 - percent_lines_with_issues

Also aggregates by top-level folder and by language and writes results to
`output/coverage_analysis.json` and `output/coverage_summary.txt`.
"""
from __future__ import annotations

import json
import math
import os
from collections import defaultdict
from typing import Dict, List, Any


def analyze_file(entry: Dict[str, Any]) -> Dict[str, Any]:
    content = entry.get("content") or ""
    lines = content.splitlines()
    total_lines = max(len(lines), 0)
    issues = entry.get("issues") or []
    issues_count = len(issues)
    issue_lines = set()
    for it in issues:
        # Sonar issue line may be 'line' or 'textRange' etc.
        ln = it.get("line")
        if not ln:
            tr = it.get("textRange")
            if tr and isinstance(tr, dict):
                ln = tr.get("startLine")
        try:
            if ln and int(ln) > 0:
                issue_lines.add(int(ln))
        except Exception:
            pass
    issue_lines_count = len(issue_lines)
    kloc = total_lines / 1000.0 if total_lines > 0 else 0.0
    issues_per_kloc = (issues_count / kloc) if kloc > 0 else math.inf if issues_count > 0 else 0.0
    percent_lines_with_issues = (issue_lines_count / total_lines * 100.0) if total_lines > 0 else 0.0

    return {
        "key": entry.get("key"),
        "path": entry.get("path"),
        "language": entry.get("language"),
        "total_lines": total_lines,
        "issues_count": issues_count,
        "issue_lines_count": issue_lines_count,
        "issues_per_kloc": issues_per_kloc,
        "percent_lines_with_issues": percent_lines_with_issues,
    }


def main() -> int:
    infile = os.path.join("output", "files_with_issues.json")
    if not os.path.exists(infile):
        print(f"Input not found: {infile}")
        return 2
    with open(infile, "r", encoding="utf-8") as f:
        files = json.load(f)

    per_file = []
    by_folder = defaultdict(list)
    by_lang = defaultdict(list)

    for entry in files:
        report = analyze_file(entry)
        per_file.append(report)
        path = report.get("path") or ""
        top = path.split("/", 1)[0] if path else "."
        top = top or "."
        by_folder[top].append(report)
        by_lang[report.get("language") or "unknown"].append(report)

    # sort per-file by issues_count desc
    per_file_sorted = sorted(per_file, key=lambda x: x["issues_count"], reverse=True)

    summary: Dict[str, Any] = {
        "total_files": len(per_file),
        "total_issues": sum(f["issues_count"] for f in per_file),
        "top_files_by_issues": per_file_sorted[:20],
        "by_folder_counts": {k: len(v) for k, v in by_folder.items()},
        "by_lang_counts": {k: len(v) for k, v in by_lang.items()},
    }

    out_json = os.path.join("output", "coverage_analysis.json")
    with open(out_json, "w", encoding="utf-8") as f:
        json.dump({"per_file": per_file, "summary": summary}, f, indent=2, ensure_ascii=False)

    # write short human summary
    out_txt = os.path.join("output", "coverage_summary.txt")
    with open(out_txt, "w", encoding="utf-8") as f:
        f.write(f"Total files: {summary['total_files']}\n")
        f.write(f"Total issues: {summary['total_issues']}\n\n")
        f.write("Top files by issue count:\n")
        for item in summary["top_files_by_issues"][:20]:
            f.write(f" - {item['path']}: {item['issues_count']} issues, {item['total_lines']} lines, {item['percent_lines_with_issues']:.2f}% lines with issues\n")
        f.write("\nBy folder counts:\n")
        for k, v in sorted(summary["by_folder_counts"].items(), key=lambda x: -x[1])[:50]:
            f.write(f" - {k}: {v} files\n")
        f.write("\nBy language counts:\n")
        for k, v in sorted(summary["by_lang_counts"].items(), key=lambda x: -x[1]):
            f.write(f" - {k}: {v} files\n")

    print(f"Wrote {out_json} and {out_txt}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
