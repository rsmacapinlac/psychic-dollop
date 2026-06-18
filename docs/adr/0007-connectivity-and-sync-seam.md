# 0007 — Connectivity & Sync Seam (BLE-First)

**Status:** Accepted

## Context

v1 is a standalone device — no network features shipped. But the product
direction is **file sync to a future iPhone/Android companion app over BLE** (the
board is BLE-capable). The design bundle's tokens also reference a WiFi web
"portal," but that is **not** the chosen direction. We want v1 to make the future
sync cheap to add without reworking storage or the state machine, while shipping
none of it now.

## Decision

- **Ship a seam, not a feature.** Define a small `Transfer`/sync interface that
  can enumerate notes (from the SD scan + `.meta`, ADR 0004), read a note's bytes,
  and report/stream progress — with **no implementation wired in for v1**.
- The seam should also leave room for future **configuration access**: a companion
  app should be able to read and update `/scribr.cfg` (or call an equivalent
  device-side config API that persists back to that file) so it can set user
  configuration such as `local_offset_min` and time/bootstrap settings.
- **BLE is the intended first implementation** (v2), reading the same on-disk
  layout. The `/recordings/<YYYYMMDDTHHMMSS>/` directory-per-recording layout
  (with `session.wav` + `session.meta`, ADR 0004) and the `/scribr.cfg`
  configuration file are treated as stable contracts for that consumer.
- **No WiFi portal.** The design's portal tokens are out of scope; if connectivity
  ever wants WiFi/NTP, it implements the same seam (and the time model already
  anticipates NTP, ADR 0005).
- Keep the storage and state-machine layers ignorant of transport: sync is a
  consumer of storage, never the other way around.

## Consequences

- v1 stays small and offline; no BLE stack, pairing UI, or radio power cost yet.
- Adding BLE later is additive: implement the seam, add any pairing/sync UI
  (new edge states, ADR 0008), and reuse the existing on-disk format.
- Committing to the `/recordings/<stamp>/` + `session.meta` contract and
  `/scribr.cfg` now avoids a storage/configuration migration when sync arrives.

## Alternatives

- **Build BLE sync in v1** — rejected: out of MVP scope, adds risk before the
  recorder itself is solid.
- **WiFi web portal** (per the design tokens) — rejected: not the product
  direction; a phone app over BLE fits a pocket device better.
- **No seam, add later** — rejected: risks a storage/state rework when sync lands;
  the seam is nearly free now.
