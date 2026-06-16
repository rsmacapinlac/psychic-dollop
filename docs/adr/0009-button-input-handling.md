# 0009 — Button Input Handling via OneButton

**Status:** Accepted (ratifies the prototype's dependency)

## Context

All interaction is two buttons (BOOT/GPIO0, PWR/GPIO18), active-low with internal
pull-ups. The state machine needs **short**, **long**, and **double** press events,
distinguished per the device spec's thresholds:

| Event  | Spec |
| ------ | ---- |
| short press threshold | 350 ms |
| long press threshold  | 600 ms |
| double press window   | 200 ms |

The same physical buttons are also the EXT1/GPIO wake source out of sleep
(ADR 0006), so runtime detection only has to cover the awake states. The
prototype already uses the **OneButton** library (2.6.2) for this; the
interactive design prototype hand-rolled an equivalent detector (`makePresser`).

## Decision

Use the **OneButton** library for runtime press detection on both buttons, rather
than a hand-rolled GPIO state machine.

- One `OneButton` instance per button, constructed active-low with pull-ups.
- Map events to the state machine: `attachClick` → short, `attachDoubleClick`
  → double, `attachLongPressStart` → long (fires at the threshold, which suits the
  "act on long-press begin" semantics the state machine wants).
- Configure to the spec thresholds: double-press window via `setClickMs(200)`,
  long-press via `setPressMs(600)`, plus a sane debounce.
- Poll both instances from the core-1 loop `tick()` (ADR 0002); the existing
  1 ms loop cadence is well inside the detector's timing needs.
- Keep button handling behind the `app` layer — OneButton callbacks dispatch
  into the state machine, which owns all transitions (`../requirements/state-machine.md`).

## Consequences

- Removes the need to write and debug our own debounce/long/double timing — a
  classic source of subtle bugs — for a tiny, well-used dependency.
- Press semantics live in library config, so the spec thresholds are expressed in
  one place and easy to retune on hardware.
- The **350 ms "short press threshold"** has no direct OneButton knob: a short
  click is simply any press released before the long threshold and not followed by
  a second click within the double window. Treat 350 ms as informational (or as a
  debounce/feel target), not a hard gate, unless hardware testing says otherwise.
- Adds OneButton as a required library (already in `sketch.yaml`).

## Alternatives

- **Hand-rolled detector** (like the JSX `makePresser`) — rejected: reimplements
  what OneButton already does well; more code to verify for no benefit here.
- **Raw GPIO + manual timing** — rejected: most control, but debounce + double +
  long timing by hand is error-prone and not where the project's value lies.
