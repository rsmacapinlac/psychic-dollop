# 0004 — Storage & Metadata Model

**Status:** Accepted (amended 2026-06-18)

> **Amendment 2026-06-18 — earshot-compatible layout.** The on-disk layout now
> mirrors the [earshot](https://github.com/rsmacapinlac/earshot) project's
> filesystem-as-state model: **one directory per recording, named with the UTC
> capture time**, instead of flat `note_%03d.wav` files. The directory name is the
> authoritative timestamp; a `session.meta` sidecar carries duration. The
> rejection of "timestamp in the filename" below stands for *flat* files — the
> timestamp now names the containing **directory**, which is unambiguous and keeps
> the scan self-healing. Decision bullets and consequences below reflect the
> amended layout; the original ID-based scheme is retained only in this note for
> history.

## Context

The prototype holds up to 16 recordings in a RAM array with drop-oldest. A real
device stores notes on the SD card (SD_MMC, 1-bit mode; pins in
`../reference/hardware-pinout.md`) and can accumulate hundreds. Each note needs a creation
timestamp the WAV format can't carry. The RECORDINGS_LIST shows a 3-row window
and must scale, and a future BLE sync (ADR 0007) needs a stable on-disk layout.

## Decision

- **SD card is the source of truth.** No fixed RAM cap; the in-RAM model holds
  only what the current screen needs.
- **One directory per recording.** Each note is a directory under `/recordings/`
  named with the UTC capture time as `%Y%m%dT%H%M%S` (e.g.
  `/recordings/20260618T091500/`). If the clock is not yet set, an `unset-NNN`
  fallback name is used so capture still works; such notes sort last and display
  "time not set". The **directory name is the authoritative creation timestamp**.
- **Files inside the directory:**
  - `session.wav` — mono 16 kHz/16-bit PCM per
    `../requirements/recording-playback-spec.md`; the 44-byte header is written
    last (backfill).
  - `session.meta` — sidecar holding `duration_sec=<seconds>`. Duration lives in
    the sidecar (the WAV can't carry it cleanly and byte-derived duration drifts
    during bring-up); creation time comes from the directory name, not the meta.
- **List building:** on entering RECORDINGS_LIST, **scan `/recordings`**, sort by
  creation time (newest first), and page the 3-row window over the result. Read
  full metadata lazily for the rows actually shown.
- **Durability:** a recording is committed only once its `session.wav` header is
  backfilled and `session.meta` is written; a power loss or cancel mid-capture
  must not leave a note that breaks the scan. Captures under ~1000 bytes are
  discarded and their directory removed. The scan ignores any directory without a
  valid `session.wav` (> 44 bytes), so a partial capture is self-healing.
- **Naming:** the timestamp directory name is self-describing, so no separate
  index/counter file is kept and none can desync. (When the clock is unset, the
  `unset-NNN` fallback derives N from the highest existing `unset-*` directory.)

## Consequences

- Scales to hundreds of notes; the list is honest about what's on the card.
- Slightly more SD I/O on list entry (a directory scan) — acceptable; cache the
  sorted index for the session and invalidate on record/delete.
- The directory-per-recording layout is a stable contract for BLE sync to read
  later, and matches earshot so tooling/offload can be shared.
- Delete removes the recording's directory and its contents (`session.wav` +
  `session.meta`).

## Alternatives

- **Keep the 16-item RAM cap** — rejected: loses notes, doesn't match an
  SD-backed device.
- **Single index/manifest file** instead of per-note sidecars — rejected for v1:
  a manifest can desync from the actual files and is a single corruption point; a
  scan is self-healing. Revisit if scan latency becomes a problem.
- **Embed timestamp in the filename only** — rejected: brittle, and WAV already
  can't hold duration cleanly.
