#!/usr/bin/env python3
"""datasheet_fetch — resolve and download instrument datasheets/manuals.

Vendor programming manuals are the authoritative SCPI source; this tool covers
the datasheet-retrieval fallback described in the development plan by querying
the official Digi-Key and Mouser REST APIs (no scraping). Stdlib only.

Auth via environment (never commit keys):
  MOUSER_API_KEY          Mouser Search API key
  DIGIKEY_CLIENT_ID       Digi-Key OAuth2 client id
  DIGIKEY_CLIENT_SECRET   Digi-Key OAuth2 client secret

Usage:
  datasheet_fetch.py resolve  <model> [--vendor V] [--dry-run]
  datasheet_fetch.py download <model> --vendor V [--url URL] [--dry-run]

--dry-run prints the intended API calls and exits 0 without network or keys,
so the tool is exercisable in CI. `download` upserts datasheets/index.json.
"""

import argparse
import hashlib
import json
import os
import sys
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DATASHEETS_DIR = REPO_ROOT / "datasheets"
INDEX_PATH = DATASHEETS_DIR / "index.json"

MOUSER_SEARCH_URL = "https://api.mouser.com/api/v1/search/partnumber?apiKey={key}"
DIGIKEY_TOKEN_URL = "https://api.digikey.com/v1/oauth2/token"
DIGIKEY_SEARCH_URL = "https://api.digikey.com/products/v4/search/keyword"


# --------------------------- helpers ---------------------------

def _log(msg):
    print(msg, file=sys.stderr)


def _http_post_json(url, payload, headers):
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.loads(resp.read().decode("utf-8"))


def _load_index():
    if INDEX_PATH.exists():
        return json.loads(INDEX_PATH.read_text())
    return {"schemaVersion": "1.0.0", "documents": []}


def _save_index(index):
    INDEX_PATH.write_text(json.dumps(index, indent=2) + "\n")


def _upsert_document(index, record):
    for i, doc in enumerate(index["documents"]):
        if doc.get("model") == record["model"] and doc.get("docType") == record.get("docType"):
            index["documents"][i] = {**doc, **record}
            return
    index["documents"].append(record)


# --------------------------- providers ---------------------------

def mouser_resolve(model, dry_run):
    key = os.environ.get("MOUSER_API_KEY", "")
    url = MOUSER_SEARCH_URL.format(key="<MOUSER_API_KEY>" if dry_run else key)
    payload = {"SearchByPartRequest": {"mouserPartNumber": model}}
    if dry_run:
        _log(f"[dry-run] POST {url}")
        _log(f"[dry-run] body {json.dumps(payload)}")
        return None
    if not key:
        _log("MOUSER_API_KEY not set")
        return None
    result = _http_post_json(url, payload, {"Content-Type": "application/json"})
    for part in result.get("SearchResults", {}).get("Parts", []):
        if part.get("DataSheetUrl"):
            return part["DataSheetUrl"]
    return None


def digikey_token(dry_run):
    cid = os.environ.get("DIGIKEY_CLIENT_ID", "")
    secret = os.environ.get("DIGIKEY_CLIENT_SECRET", "")
    if dry_run:
        _log(f"[dry-run] POST {DIGIKEY_TOKEN_URL} (client_credentials)")
        return None
    if not cid or not secret:
        _log("DIGIKEY_CLIENT_ID / DIGIKEY_CLIENT_SECRET not set")
        return None
    body = f"client_id={cid}&client_secret={secret}&grant_type=client_credentials"
    req = urllib.request.Request(
        DIGIKEY_TOKEN_URL, data=body.encode(),
        headers={"Content-Type": "application/x-www-form-urlencoded"}, method="POST")
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.loads(resp.read().decode()).get("access_token")


def digikey_resolve(model, dry_run):
    cid = os.environ.get("DIGIKEY_CLIENT_ID", "")
    payload = {"Keywords": model, "Limit": 5}
    if dry_run:
        _log(f"[dry-run] POST {DIGIKEY_SEARCH_URL}")
        _log(f"[dry-run] body {json.dumps(payload)}")
        return None
    token = digikey_token(dry_run)
    if not token:
        return None
    headers = {
        "Authorization": f"Bearer {token}",
        "X-DIGIKEY-Client-Id": cid,
        "Content-Type": "application/json",
    }
    result = _http_post_json(DIGIKEY_SEARCH_URL, payload, headers)
    for product in result.get("Products", []):
        if product.get("DatasheetUrl"):
            return product["DatasheetUrl"]
    return None


def resolve_url(model, vendor, dry_run):
    """Try Mouser then Digi-Key; return the first datasheet URL found."""
    url = mouser_resolve(model, dry_run)
    if url:
        return url, "mouser"
    url = digikey_resolve(model, dry_run)
    if url:
        return url, "digikey"
    return None, None


# --------------------------- commands ---------------------------

def cmd_resolve(args):
    url, src = resolve_url(args.model, args.vendor, args.dry_run)
    if args.dry_run:
        print("(dry-run) would query Mouser then Digi-Key; no URL fetched.")
        return 0
    if url:
        print(f"{url}  (via {src})")
        return 0
    _log(f"No datasheet found for {args.model}")
    return 1


def cmd_download(args):
    vendor = args.vendor
    url = args.url
    if not url:
        url, _ = resolve_url(args.model, vendor, args.dry_run)

    dest_dir = DATASHEETS_DIR / vendor / args.model
    dest = dest_dir / f"{args.model}.pdf"

    if args.dry_run:
        print(f"(dry-run) would download {url or '<resolved-url>'} -> {dest}")
        print("(dry-run) would upsert datasheets/index.json")
        return 0

    if not url:
        _log(f"No datasheet URL for {args.model}")
        return 1

    dest_dir.mkdir(parents=True, exist_ok=True)
    _log(f"Downloading {url} -> {dest}")
    with urllib.request.urlopen(url, timeout=60) as resp:
        data = resp.read()
    dest.write_bytes(data)
    sha = hashlib.sha256(data).hexdigest()

    index = _load_index()
    _upsert_document(index, {
        "model": args.model,
        "manufacturer": vendor,
        "docType": "datasheet",
        "title": f"{args.model} datasheet",
        "path": str(dest.relative_to(REPO_ROOT)),
        "sourceUrl": url,
        "version": "",
        "sha256": sha,
        "retrieved": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    })
    _save_index(index)
    print(f"Saved {dest} ({len(data)} bytes, sha256={sha[:12]}…)")
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description="Resolve/download instrument datasheets.")
    p.add_argument("--dry-run", action="store_true", help="print intended calls; no network/keys")
    sub = p.add_subparsers(dest="cmd", required=True)

    r = sub.add_parser("resolve", help="print the datasheet URL for a model")
    r.add_argument("model")
    r.add_argument("--vendor", default="")
    r.set_defaults(func=cmd_resolve)

    d = sub.add_parser("download", help="download the datasheet and update index.json")
    d.add_argument("model")
    d.add_argument("--vendor", required=True)
    d.add_argument("--url", default="", help="explicit URL (skips resolution)")
    d.set_defaults(func=cmd_download)

    args = p.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
