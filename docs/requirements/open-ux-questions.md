# Open UX Questions

A living registry of interaction/UX decisions that are **deliberately unresolved**
— surfaced during design and requirements work but not yet settled. Nothing here
is canon; each entry notes an interim default so implementation isn't blocked.
When a question is decided, fold the outcome into the relevant requirement/ADR
and mark the entry **Resolved** (with the date and where it landed).

Each question has a stable ID (`UX-n`) so other docs can reference it.

| ID | Question | Status |
| -- | -------- | ------ |
| UX-1 | Global PWR long-press "escape hatch" | Open |
| UX-2 | Edge / error-state screen layouts | **Resolved** (2026-06-15) |
| UX-3 | Low-battery & charging: screen, badge, or both | **Resolved** (2026-06-15) |
| UX-4 | "You are live" treatment for RECORDING | Open |
| UX-5 | Setting the clock & timezone offset (no settings UI) | **Resolved** (2026-06-15) |
| UX-6 | Long date display on PLAYBACK / DELETE_CONFIRM | Open |
| UX-7 | Advisory screens: auto-return vs keypress-dismiss | Open |

---

## UX-1 — Global PWR long-press "escape hatch"

**Status:** Open · **Related:** `state-machine.md`

Whether a long PWR press should be a universal "return to IDLE" gesture, and how
far it reaches. Sources disagree: the original spec proposed it everywhere except
RECORDING; the JSX prototype implemented it only in PLAYBACK/DELETE; the C++
prototype largely omitted it. Sub-questions:

- Is a hidden, unhinted global gesture desirable at all on a two-button device?
- Should it be discoverable (shown as a hint) or intentionally invisible?
- Does it collide with per-state long-press meanings (e.g. LIST's hold-to-delete)?

**Interim default:** don't wire PWR-long beyond the `LIST → DELETE_CONFIRM` case.

## UX-2 — Edge / error-state screen layouts

**Status:** Resolved (2026-06-15) · **Related:** `ui-screens.md`,
`../adr/0008-edge-and-error-state-ui.md`

**Resolved:** the edge/error screens were designed in Claude Design — see the
`Scribr edge screens.html` gallery linked from `ui-screens.md`. Six 1-bit panels
(SLEEP, LOW BATTERY, CHARGING, NO SD CARD, STORAGE FULL, TIME NOT SET) in the
existing chrome. A boot/splash screen is **deferred** (the old logo bitmap was
dropped — see `ui-screens.md`). Layouts now live in the design, not here.

## UX-3 — Low-battery & charging: screen, badge, or both

**Status:** Resolved (2026-06-15) · **Related:**
`../adr/0008-edge-and-error-state-ui.md`, `time-power-spec.md`

**Resolved: full-screen only, no persistent badge.** Battery state surfaces as a
full-screen frame — never chrome on the happy-path screens. It appears when
charge crosses ≤15% (LOW BATTERY, non-blocking — dismiss and keep recording) or a
charger is connected (CHARGING, which reuses the system's inverted block so an
energised battery reads as "active"). Battery otherwise shows only on IDLE.

## UX-4 — "You are live" treatment for RECORDING

**Status:** Open · **Related:** `ui-screens.md`

RECORDING currently signals "live" with a filled record dot + doubled header rule
+ big timer, but no inversion. Raised repeatedly in design and never decided:
should RECORDING get a stronger treatment (e.g. a fully inverted panel) so it's
unmistakable at a glance, or is the current treatment enough?

**Interim default:** keep the current record-dot + doubled-rule + big-timer treatment.

## UX-5 — Setting the clock & timezone offset

**Status:** Resolved (2026-06-15) · **Related:** `../adr/0005-time-model.md`,
`boot-configuration.md`

**Resolved:** v1 does not add an on-device settings screen. The user configures
local display offset and may manually bootstrap the RTC through the SD-card boot
configuration file at `/scribr.cfg`:

- `local_offset_min=` — signed minutes from UTC for display only.
- `rtc_set_utc=` — optional one-shot/manual UTC timestamp for setting the RTC at
  boot.

If the RTC is invalid and no valid `rtc_set_utc` is provided, firmware follows
the TIME NOT SET condition flow: recording is allowed, bogus timestamps are not
written, and the TIME NOT SET screen is surfaced.

## UX-6 — Long date display on PLAYBACK / DELETE_CONFIRM

**Status:** Open · **Related:** `ui-screens.md`, `../reference/device-rendering-constraints.md`

The full 16-char date `YYYY-MM-DD HH:MM` at FreeSansBold 12pt sits right at the
~15-char width budget on these two screens. If it clips on real glass, what's the
preferred fix — drop the century (`26-06-14 09:32`), wrap to two lines, or use a
smaller font?

**Interim default:** keep the full date; verify on hardware before changing.

## UX-7 — Advisory screens: auto-return vs keypress-dismiss

**Status:** Open · **Related:** `state-machine.md`,
`../adr/0008-edge-and-error-state-ui.md`

The advisory condition screens (LOW BATTERY, CHARGING) are designed with a
`dismiss` keypress (PWR) that returns the user to the prior screen. Should they
instead **auto-return** after a few seconds with no keypress — so a low-battery
warning never strands the user mid-task — or is an explicit dismiss preferable so
the message can't be missed?

**Interim default:** keypress dismiss (PWR), as designed.

---

> Note: **technical** (non-UX) open decisions live in their own registry,
> `open-technical-decisions.md` (e.g. sleep depth, audio quality ceiling). This
> file is UX/interaction questions only.
