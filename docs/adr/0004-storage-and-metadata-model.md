# 0004 — Storage & Metadata Model

**Status:** Accepted

## Context

The prototype holds up to 16 recordings in a RAM array with drop-oldest. A real
device stores notes on the SD card (SD_MMC, 1-bit mode; pins in
`../reference/hardware-pinout.md`) and can accumulate hundreds. Each note needs a creation
timestamp the WAV format can't carry. The RECORDINGS_LIST shows a 3-row window
and must scale, and a future BLE sync (ADR 0007) needs a stable on-disk layout.

## Decision

- **SD card is the source of truth.** No fixed RAM cap; the in-RAM model holds
  only what the current screen needs.
- **Files:** audio as `/notes/note_%03d.wav` (zero-padded), mono 16 kHz/16-bit
  PCM per `../requirements/recording-playback-spec.md`. Write the 44-byte header last (backfill).
- **Metadata sidecar:** one `.meta` file per note (e.g. `/notes/note_%03d.meta`)
  holding at least `created_utc=` in ISO-8601 UTC, plus duration. The sidecar,
  not the filename, is the authoritative timestamp/duration source.
- **List building:** on entering RECORDINGS_LIST, **scan `/notes`**, sort by
  creation time (newest first), and page the 3-row window over the result. Read
  full metadata lazily for the rows actually shown.
- **Durability:** a recording is committed only once its header is backfilled and
  its `.meta` is written; a power loss or cancel mid-capture must not leave a note
  that breaks the scan. Captures under ~1000 bytes are discarded.
- **Naming/index:** next index derives from scanning existing files (highest +1),
  so no separate counter file can desync.

## Consequences

- Scales to hundreds of notes; the list is honest about what's on the card.
- Slightly more SD I/O on list entry (a directory scan) — acceptable; cache the
  sorted index for the session and invalidate on record/delete.
- The sidecar format is a stable contract for BLE sync to read later.
- Delete removes both the `.wav` and its `.meta`.

## Alternatives

- **Keep the 16-item RAM cap** — rejected: loses notes, doesn't match an
  SD-backed device.
- **Single index/manifest file** instead of per-note sidecars — rejected for v1:
  a manifest can desync from the actual files and is a single corruption point; a
  scan is self-healing. Revisit if scan latency becomes a problem.
- **Embed timestamp in the filename only** — rejected: brittle, and WAV already
  can't hold duration cleanly.
