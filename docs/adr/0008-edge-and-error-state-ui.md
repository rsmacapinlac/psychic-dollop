# 0008 — Edge & Error-State UI

**Status:** Accepted (designed; see `../requirements/ui-screens.md` for the link)

## Context

The original design covered only the five "happy path" states. The hardware
brief and subsystem specs imply several conditions that had **no screen**: empty
battery, missing storage, an unset clock, and sleep. These needed designs
obeying the 1-bit GFX constraints (`../reference/device-rendering-constraints.md`)
and the existing header/footer/hint conventions
(`../requirements/ui-screens.md`), rather than ad-hoc text invented during
implementation.

## Decision

The edge/error screens were designed in Claude Design as a first-class set, in
the existing visual system. The realized screens:

| Screen | Trigger | Behaviour |
| ------ | ------- | --------- |
| **Sleep** | 120 s inactivity (ADR 0006) | Wordmark + "Sleeping..."; e-ink holds it at zero power. No hints. |
| **Low battery** | ≤15%, recover ≥20% (ADR/spec) | Full-screen, **non-blocking** — dismiss (PWR) and keep recording. |
| **Charging** | charger present | Full-screen; reuses the inverted block as the "energised" treatment. Dismiss (PWR). |
| **No SD / SD error** | card absent or unreadable | Blocking — can't record or list; `EXIT (hold)` on BOOT. |
| **Storage full** | insufficient space | Blocking — in-progress note stopped and kept; `OK` (PWR) → IDLE. |
| **Time not set** | RTC OS bit / sanity-check fail (ADR 0005) | Mild — recording allowed, timestamps unset; `continue` (PWR) → IDLE. |

The two principles that guided it, and bind future additions:

- **Reuse the existing header/rule/footer-hint chrome — no new visual language.**
- **Blocking conditions get their own screen; advisory conditions don't add
  chrome.** Battery in particular is full-screen only (UX-3), never a persistent
  badge, so the happy-path screens stay clean.

## Consequences

- Closes the gap between the brief's requirements and the drawn design before it
  became ad-hoc UI.
- Resolves **UX-2** (layouts) and **UX-3** (battery presentation) in
  `../requirements/open-ux-questions.md`.
- Edge-screen transitions (dismiss/OK/continue/exit) still need folding into
  `../requirements/state-machine.md`, which currently enumerates only the five
  core states.
- **No boot/splash screen for v1** — the old `logo_bitmap.h` ("pala NOTE")
  was dropped; a boot frame is deferred until a scribr-branded asset exists
  (`../requirements/ui-screens.md`).

## Alternatives

- **Handle errors with ad-hoc text** — rejected: inconsistent, and the brief
  explicitly calls for battery warnings and clear state.
- **Persistent battery badge in chrome** — rejected (UX-3): clutters the
  happy-path screens; full-screen-when-severe keeps them clean.
