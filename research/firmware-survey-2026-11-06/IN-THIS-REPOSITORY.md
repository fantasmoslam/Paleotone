# What is committed here, and what is not

This folder is the Felucca / FM-1 / SMK-37 firmware survey of November 6, 2026.
Start with `Felucca-Firmware-Survey-2026-11-06.md`; every candidate is in
`deliverables/Repository-Register.md`.

The full source pack is `Felucca-Firmware-Survey-Source-Pack-2026-11-06.zip`
(109,242,925 bytes, SHA-256
`37355e9ba472788ae71f67efb5dd5cb614ba5efd4c4f1a9cfbeeabcc434e1d40`). Only part of it is committed.

## Committed

- The report, the repository register and every file in `deliverables/`
- The collection, screening, ancestry and report scripts
- `audit/*.json`: source references, blob status, coverage and review records
- `sources/*.json`: the discovery catalogues and search results

## Left out, kept only in the zip

| Path in the pack | Size | Why |
| --- | --- | --- |
| `corpus/blobs/` | 278 MB, 11,619 files | Copies of other projects' source code under their own licences, some with no licence. Committing them would redistribute that code from this repository. |
| `audit/files/` | 30 MB | Copies of other projects' READMEs |
| `audit/trees/` | 31 MB | Raw repository tree listings, kept for reproducibility only |
| `sources/` documents | under 1 MB | Saved copies of other projects' pages: the awesome-fm-1 list, the resource hub, the SMK-37 hub, the 2bbb collection README and the RemoteFM1 Android and iOS READMEs |

`verify_survey.py` and `extract_repo.py` need `corpus/blobs/`. To run them, unzip the full pack and run
them there. The links in the report point at the original repositories, pinned to the surveyed commits.
