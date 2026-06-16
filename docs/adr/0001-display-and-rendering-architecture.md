# 0001 — Display & Rendering Architecture

**Status:** Accepted (ratifies the prototype's approach)

## Context

The panel is a 200×200 1-bit e-paper. A full refresh busy-waits ~300–500 ms and
flashes black→white; a partial refresh is fast and flash-free but accumulates
ghosting. The UI is composed of static frames with a few live fields (REC/PLAY
timers, progress bar). We draw with Adafruit GFX, whose canvas polarity (bit 1 =
ink) is the inverse of the panel framebuffer (bit 1 = white).

The existing prototype already solved this well, and the current code is
otherwise reworkable; we want to lock the parts worth keeping.

## Decision

- Draw every screen into a single shared `GFXcanvas1(200,200)`; screens compose
  into it and never flush themselves. A `display` service owns flushing.
- Two flush modes: `flushFull()` (black-flash, re-bases partial mode, clears
  ghosting) on **screen changes**; `flush()` (partial, no flash) for **isolated
  live fields**.
- Blit canvas→panel with a bitwise polarity inversion (`dst = ~src`); identical
  geometry (25 bytes/row, MSB-first) makes this a straight byte copy.
- Keep live-updating fields spatially isolated so a partial refresh touches only
  their zone (`../reference/device-rendering-constraints.md`).
- Add a **periodic cleanup full-refresh** during long REC/PLAY sessions to clear
  accumulated partial-refresh ghosting (flagged in design, not yet built).

## Consequences

- Pixel-faithful to the approved design; `screens.cpp` is the layout reference.
- Screens stay pure (model → canvas), testable without hardware.
- The refresh must run off the input-servicing path — see ADR 0002.
- The cleanup-refresh cadence (e.g. every 30–60 s or on stop) is a tunable to
  validate on hardware.

## Alternatives

- **Full refresh always** — rejected: unbearable flashing on every timer tick.
- **Per-region GFX without a full canvas** — rejected: more bookkeeping, loses the
  single-surface simplicity, easy to desync zones.
