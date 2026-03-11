#!/usr/bin/env python3
"""Download SonarQube file components and save as JSON.

Usage:
  python scripts/sonar_download.py --url https://sonar.example.com \
      --token YOUR_TOKEN --project your.project.key --out sonar_files.json

The script lists file components for the given project and fetches each
file's raw content via the SonarQube Web API, saving a JSON array of
objects {"key","path","language","content"}.
"""
from __future__ import annotations

import argparse
import json
import sys
from typing import Any, Dict, List

import requests


def auth_tuple(token: str):
    # Sonar accepts token as HTTP Basic username with empty password
    return (token, "")


def list_components(base_url: str, auth: tuple, project: str, page_size: int = 500, org: str | None = None) -> List[Dict[str, Any]]:
    comps: List[Dict[str, Any]] = []
    page = 1
    while True:
        url = f"{base_url.rstrip('/')}/api/components/search"
        params: Dict[str, object] = {"component": project, "qualifiers": "FIL", "ps": page_size, "p": page}
        if org:
            params["organization"] = org
        r = requests.get(url, params=params, auth=auth, timeout=30)
        r.raise_for_status()
        data = r.json()
        batch = data.get("components") or []
        if not batch:
            break
        comps.extend(batch)
        total = int(data.get("paging", {}).get("total", len(comps)))
        if len(comps) >= total:
            break
        page += 1
    return comps


def fetch_raw_source(base_url: str, auth: tuple, component_key: str, org: str | None = None) -> str:
    url = f"{base_url.rstrip('/')}/api/sources/raw"
    params: Dict[str, object] = {"key": component_key}
    if org:
        params["organization"] = org
    r = requests.get(url, params=params, auth=auth, timeout=30)
    # raw endpoint returns text (not JSON)
    if r.status_code == 200:
        return r.text
    r.raise_for_status()
    return ""


def main(argv: List[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Download SonarQube project files as JSON")
    p.add_argument("--url", required=True, help="SonarQube base URL, e.g. https://sonar.example.com")
    p.add_argument("--token", required=True, help="SonarQube API token")
    p.add_argument("--project", required=True, help="SonarQube project key (component)")
    p.add_argument("--out", default="sonar_files.json", help="Output JSON filename")
    p.add_argument("--org", default=None, help="SonarCloud organization (org slug), e.g. fabulous-samurai")
    p.add_argument("--ps", type=int, default=500, help="Page size for component listing")
    args = p.parse_args(argv)

    auth = auth_tuple(args.token)

    try:
        print(f"Listing file components for project {args.project}...")
        components = list_components(args.url, auth, args.project, page_size=args.ps, org=args.org)
        print(f"Found {len(components)} file components")
    except requests.HTTPError as e:
        print("Failed to list components:", e, file=sys.stderr)
        return 2

    results: List[Dict[str, Any]] = []
    for i, comp in enumerate(components, start=1):
        key = comp.get("key")
        path = comp.get("path") or comp.get("qualifier") or comp.get("longName")
        language = comp.get("language")
        print(f"[{i}/{len(components)}] Fetching {key} ({path})...")
        try:
            content = fetch_raw_source(args.url, auth, key, org=args.org)
        except requests.HTTPError as e:
            print(f"  -> error fetching {key}: {e}", file=sys.stderr)
            content = ""
        results.append({"key": key, "path": path, "language": language, "content": content})

    print(f"Writing output to {args.out}...")
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2, ensure_ascii=False)

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
