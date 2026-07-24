# datasheet_fetch

Resolve and download instrument datasheets/manuals via the official **Digi-Key**
and **Mouser** REST APIs (no scraping). This is the datasheet-retrieval fallback
from the development plan — vendor programming manuals remain the authoritative
SCPI source; this tool automates locating and archiving the PDFs and recording
them in [`datasheets/index.json`](../../datasheets/index.json).

Stdlib-only Python 3 (no third-party packages).

## Setup

Provide API credentials via environment variables (never commit keys):

```sh
export MOUSER_API_KEY=...            # Mouser Search API key
export DIGIKEY_CLIENT_ID=...         # Digi-Key OAuth2 client id
export DIGIKEY_CLIENT_SECRET=...     # Digi-Key OAuth2 client secret
```

Register for keys at the vendors' developer portals. Respect each API's Terms of
Service and rate limits.

## Usage

```sh
# Print the datasheet URL for a model (tries Mouser, then Digi-Key)
python3 datasheet_fetch.py resolve DSOX2012A

# Download the PDF into datasheets/<vendor>/<model>/ and update index.json
python3 datasheet_fetch.py download DSOX2012A --vendor Keysight

# Provide an explicit URL (skips API resolution)
python3 datasheet_fetch.py download MDO34 --vendor Tektronix --url https://.../mdo34.pdf

# Offline preview — prints intended API calls, needs no keys/network (used in CI)
python3 datasheet_fetch.py --dry-run resolve DSOX2012A
```

`download` computes the PDF's sha256 and upserts a record
(model, manufacturer, path, sourceUrl, sha256, retrieved) into
`datasheets/index.json`.

## Notes

- `--dry-run` exits 0 with no keys set, so the tool is exercisable in CI.
- Bench oscilloscopes are often better served by the manufacturer's own
  document portal than by component distributors; use `--url` when you already
  have the manufacturer's programming-guide link.
