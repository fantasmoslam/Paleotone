# FM-1 / Felucca firmware survey — November 6, 2026

Start with **Felucca-Firmware-Survey-2026-11-06.md**. The complete candidate register is **deliverables/Repository-Register.md**.

This package contains a dated public-source screening survey and focused architectural review. Repository candidates include forks, hosts, libraries, games, unresolved entries and unavailable projects. They are not a count of unique playable firmwares. Documentation tags are reported features, not tested capabilities. No hardware was flashed or qualified.

The deduplicated source collection is in **corpus/blobs/**. **audit/final-source-references.json** maps every collected file to its original repository, commit and path. Retain that mapping with reused files. Original licences and notices apply; this collection grants no new licence.

Reconstruct the collected subset of one project:

```bash
python3 extract_repo.py Rastapopolix/tessera --out extracted/tessera
python3 extract_repo.py fantasmoslam/fmini-1@smk37 --out extracted/fmini-smk37
```

These are survey subsets. Build assets, submodules, generated tables or toolchains may be absent.

Verify source integrity and inventory accounting offline:

```bash
python3 verify_survey.py
```

The synthesis-source and platform-source ancestry evidence are separate. Read **deliverables/ancestry-evidence.json** alongside the conservative classifications in the repository register. File equality alone does not establish direct parentage or direction of copying. Unavailable and uncertain projects remain visible.

No tablet integration was implemented in this survey.
