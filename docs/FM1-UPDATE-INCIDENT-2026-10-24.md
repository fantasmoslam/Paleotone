# FM-1 update failure: interrupted write confirmed

Investigation date: October 24, 2026 (America/Phoenix).

## Finding

The recovery backup contains a partial installation: the first 23 application
sectors are from `532ed716c2390526e49fb9092a9dbbfe8f3f5f26`, followed by 115 sectors
from `dee5c8007ac6710009488243cbf4011f737e40dc`. The last five sectors are identical
in both packages. Every application sector matches one or both packages; none is
unexplained corruption. Both independently captured backups are byte-identical.

This mixed executable and its invalid checksums explain the nonbooting application.
The user confirmed that the installer errored or stopped before completion and that
the instrument never booted after the update. This is evidence of an interrupted
installation, not evidence that the complete new application reached boot and
crashed. The initial screen-companion webpage timeout remains a separate incident.

The cause of the transfer interruption is still unknown. There is no original
installer trace establishing whether it was a USB disconnect, host/backend error,
loader stall, power interruption, or another transport failure. The complete new
firmware's hardware boot and runtime safety remain unverified.

## Exact evidence

| Item | SHA256 |
| --- | --- |
| Failed flash backup (both copies) | `29178e3b248096b85155299b4e7169e56fbb8bfc5ce6e89e8b82c926707f677d` |
| Earlier CI package, dee5c800 | `666504401907408360b1cb15c914fbfbe9a06e9c80cbf4506950ea78c2937ce0` |
| New CI package, 532ed716 | `142dc37b7cb66c6e8a4caea77492f102cbbf65a3b04d1b5ed86d2dc1fe5a71ce` |

The CI packages were downloaded from firmware runs 40155067799 and 40156754924.
Their hashes match those runs' build logs. The failed flash was read from the
existing `backup6/flash-1.bin` and `flash-2.bin` recovery files, not from the live
instrument. The backup directly ties the new package to the interrupted write.
Raw device backups and user data are not included in this repository.

Ranges below use an exclusive end address and physical flash offsets:

| Range | Sectors | Exact match |
| --- | --- | --- |
| `0x04000 .. 0x1B000` | 23 | New package only |
| `0x1B000 .. 0x8E000` | 115 | Earlier package only |
| `0x8E000 .. 0x93000` | 5 | Both packages |

Replacing the earlier package's application prefix with the new package through
`0x1B000` reproduces the entire captured application region byte for byte.

The SFC-decrypted backup has valid app-area and app-entry metadata header CRCs,
but the content CRCs fail:

| CRC16 | Declared in new header | Actual captured content |
| --- | --- | --- |
| App-area body | 1429 | 58481 |
| app.bin | 25351 | 47272 |

Both complete packages pass their own UFW file CRCs and these decrypted content
checks. Their update-loader payloads and packaged boot heads are identical.
The staged loader body at `0xE0020` exactly matches the package loader body.
The flash update record at `0xE4F00` has a valid CRC16 (46877), type `0x5441`, and
loader offset `0xE0000`. It was not cleared as a completed install would clear it.
These facts support interruption before loader completion. They do not prove the
SPL's physical-device resume behavior after every reset condition.

The build logs show image sizes 561124 -> 561540 bytes and RAM .data+.bss
89380 -> 89396 bytes. Pool use stays 332900 of 344064 bytes. Those figures do not
establish a new memory-capacity overflow as the cause of this incident.

## Validation and changes

`tests/ldr_test.c` now interrupts the actual portable loader after 23 verified
sectors, checks the exact new-prefix/old-suffix state, confirms no finish or record
clear happened, and resumes on that mixed image. The resumed session skips
already equal sectors and preserves the boot head and user-data region.

It also accepts an optional third argument containing a captured 1 MiB failed
flash image. Replaying backup6 through this test completed to the exact new
application, with only the remaining differing application sectors rewritten and
the boot head and user-data region preserved. All loader checks passed locally.
This is a host simulation of the actual loader core, not a hardware flash test.

`tools/fm1_install.py` now writes a flushed JSON-lines session trace containing the
package SHA256, stage, requested address/length, sent replies, interruption, and
returned identity. It refuses to overwrite an existing log before touching MIDI.
A log-write failure during an active transfer disables logging without aborting
the firmware transfer. Failed final reply sends now propagate as errors instead
of being treated as a completed write. Error text warns about partial firmware
and describes same-package resume when the loader still answers.

All 31 installer checks passed locally, including the new logging and final-send
failure cases. Optional JS/package parity and stock V15 checks were skipped because
their fixtures were absent. Firmware artifacts now retain build logs, application
image, ELF symbols and disassembly to support future diagnosis.

`tools/fm1_audit_flash.py` independently parses and validates the two packages,
classifies every captured application sector, checks decrypted content CRCs, and
checks the retained update record. It only reads files and prints JSON; it has no
device access or write commands. Its output was cross-checked against a separate
JavaScript decoder in this investigation.

Example (all paths refer to existing files; one physical command line):

```text
python tools/fm1_audit_flash.py FAILED.bin EARLIER.fwsc NEW.fwsc --second-backup FAILED2.bin
```

## Current hardware and next boundary

The instrument remains on recovered SLOOP 2.5. The existing restore log was read
during this investigation and does contain `FULL RESTORE VERIFIED` with SHA256
`f673c17a56c4091cc018815c430a2a535a110e2ce0dc1f01dfa7197caeeca501`, matching the
earlier dry-run plan. This adds evidence missing from the prior handoff.

No live device commands, firmware installation, backup modification, or recovery
write was performed. No firmware runtime change is justified solely by the mixed
flash evidence. The request-queue patch is not proven to be the brick's cause and
the complete image is not proven hardware-safe. Before any next hardware test,
retain a known recovery route and use a single updater client with session logging.
If a write fails, preserve the log and package; do not call it a completed update.
