# UI Screens

The canonical screen designs for scribr live in **Claude Design**, not in this
repo. This document is a pointer and an index of what the design covers and what
it doesn't yet — it intentionally does **not** re-describe screen layouts,
fields, copy, or type sizing, so there is a single source of truth that can't
drift out of sync.

## Source of truth

- **Design handoff (v1):**
  <https://api.anthropic.com/v1/design/h/CiTHvH_CHYLqw_RM_rVFpA>
  - `Scribr screens.html` — the static gallery of the five happy-path states plus
    the empty-list variant. Primary reference for the core flow.
  - `Scribr edge screens.html` — the static gallery of the edge/error states
    (SLEEP, LOW BATTERY, CHARGING, NO SD CARD, STORAGE FULL, TIME NOT SET).
  - `Scribr.html` — the interactive two-button prototype wiring all of the above.
- **Status: v1, evolving.** Treat the current design as v1; it will continue to
  be tweaked. When anything disagrees, **the live design wins** over any
  description elsewhere.
- **The handoff URL is per-export and goes stale** — re-exporting the design
  mints a new hash and the old link 404s. Update the link above whenever the
  design is re-exported.
- The on-device prototype `scribr/src/app/screens.cpp` renders these screens
  pixel-faithfully and is the reference implementation for layout.

## How to read it alongside the rest of the docs

- **Behaviour** — what each button does, transitions, the global rule:
  `state-machine.md`.
- **Renderability** — fonts, discrete sizes, line-width budget, ASCII-only,
  partial vs full refresh zones: `../reference/device-rendering-constraints.md`.

## Coverage

The design now covers the five happy-path states, the empty-list variant, and
the edge/error states (sleep, low-battery, charging, no-SD, storage-full,
time-not-set). Design principles for the edge set — reuse the existing chrome,
blocking conditions get their own screen while advisory ones don't — are recorded
in `../adr/0008-edge-and-error-state-ui.md`.

Deferred visual/interaction calls that remain open are in `open-ux-questions.md`
(**UX-4** RECORDING "live" treatment, **UX-6** long-date display on
PLAYBACK/DELETE). Resolved: **UX-2** (edge screens designed) and **UX-3**
(battery is full-screen only, no badge).

> **No boot/splash screen for v1.** The previous boot frame used a `logo_bitmap.h`
> that still rendered the old "pala NOTE" wordmark; the bitmap has been dropped.
> A boot/splash screen is deferred until a scribr-branded asset exists.

See also `state-machine.md`, `../reference/device-rendering-constraints.md`, and
`../adr/README.md`.
