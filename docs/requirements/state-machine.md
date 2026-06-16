# State Machine

The canonical behavioural spec for scribr: the five core states, their
transitions, the condition/interrupt screens, and what each button does in each.
This was previously captured only in the design chat and split across two
prototypes that had drifted; this document is the single source of truth. Screen
*layout* lives in `ui-screens.md`.

## Buttons

Two physical buttons, active-low with internal pull-ups (see `../reference/hardware-pinout.md`):

| Name | GPIO | Role |
| ---- | ---- | ---- |
| BOOT | 0    | cycles / advances / cancels (left slot in hints) |
| PWR  | 18   | confirms / activates (right slot in hints)       |

Hint position carries the button — left hint = BOOT, right hint = PWR — so
on-screen hints never print the button names, only the action and a modifier.

Press detection (from the device spec):

| Event  | Definition |
| ------ | ---------- |
| short  | release before the long threshold, not part of a double |
| long   | held past **600 ms** |
| double | two presses within a **200 ms** window |

The reference firmware uses a **350 ms** short-press debounce. Single/long/double
detection is delegated to the **OneButton** library, configured to these
thresholds — see `../adr/0009-button-input-handling.md`.

## States

Five logical states form the navigable flow below. Separately, a set of
**condition screens** (low battery, no SD, sleep, …) are raised by hardware
events rather than button navigation — see "Condition & interrupt screens".

`LIST` has a distinct **empty** presentation, so there are six core screens (see
`ui-screens.md`).

| State          | Activity level    | Purpose |
| -------------- | ----------------- | ------- |
| IDLE           | light sleep target | Standby home: battery, free storage, note count. |
| RECORDING      | fully active      | Capturing audio; live elapsed timer. |
| RECORDINGS_LIST| fully active      | Browse stored notes, one selected at a time. |
| PLAYBACK       | fully active      | Play the selected note; elapsed/total + progress bar. |
| DELETE_CONFIRM | fully active      | Guard a destructive delete. |

## Transition table

| From | Button + press | To / effect |
| ---- | -------------- | ----------- |
| **IDLE** | PWR double | RECORDING (start capture, elapsed = 0) |
| **IDLE** | BOOT double | RECORDINGS_LIST (listIndex = 0) |
| **IDLE** | (120 s idle) | enter sleep (see Power, below) |
| **RECORDING** | PWR short | **save** note → IDLE |
| **RECORDING** | BOOT short | **cancel** (discard take) → IDLE |
| **RECORDINGS_LIST** (non-empty) | BOOT short | advance selection (wraps) |
| **RECORDINGS_LIST** (non-empty) | PWR short | PLAYBACK of selected (elapsed = 0) |
| **RECORDINGS_LIST** (non-empty) | PWR long | DELETE_CONFIRM |
| **RECORDINGS_LIST** (empty) | BOOT long | exit → IDLE |
| **DELETE_CONFIRM** | PWR short | delete selected → RECORDINGS_LIST |
| **DELETE_CONFIRM** | BOOT short | cancel → RECORDINGS_LIST |
| **PLAYBACK** | PWR short | stop → IDLE |
| **PLAYBACK** | playback reaches end | auto-finish → IDLE |

### Global rule

The original spec proposed a global escape hatch: **PWR long-press returns to
IDLE from any state except RECORDING** (RECORDING excepted so a long PWR can't
abandon a take; in RECORDINGS_LIST the state-specific `PWR long → DELETE_CONFIRM`
would override it).

**This is currently an open UX question, not settled behaviour** — tracked as
**UX-1** in `open-ux-questions.md`. The three sources disagree on how far it
reaches, and we've deliberately left it unresolved until the interaction is
decided.

## Resolved divergences (canon for the firmware)

Three sources disagreed; the firmware follows the column marked **Canon**.

| Behaviour | Original spec | JSX prototype | C++ prototype | **Canon** |
| --------- | ------------- | ------------- | ------------- | --------- |
| Save during RECORDING | BOOT short | PWR short | PWR short | **PWR short = save; BOOT short = cancel/discard** |
| Global PWR-long → IDLE | yes (except REC) | yes (PLAY/DELETE) | partial (missing) | **Open UX question** (UX-1) |
| PLAYBACK + BOOT short | → IDLE | → IDLE | no-op | **No-op** — PLAYBACK hint shows BOOT as `-`; only PWR stops |

The "Canon" column reflects the final on-screen hints the user approved in
design, with behaviour made to match the hints. Implementations that change a
hint must change the matching transition here too.

## Empty-list behaviour

- Entering RECORDINGS_LIST with zero notes shows the empty screen; only
  `BOOT long → IDLE` is offered.
- Deleting the **last** remaining note returns to RECORDINGS_LIST in its empty
  presentation (not to IDLE). Deleting with notes remaining keeps the list and
  clamps the selection to a valid index.

## Condition & interrupt screens

These screens are raised by **hardware/system conditions**, not by navigating the
flow above. They are interrupts: an advisory one overlays and returns you where
you were; a blocking one prevents the action and drops to IDLE. Layouts are in
the design (`ui-screens.md`); rationale in `../adr/0008-edge-and-error-state-ui.md`.

| Screen | Raised when | Kind | Action → result |
| ------ | ----------- | ---- | --------------- |
| LOW BATTERY | charge crosses ≤ 15% | advisory (non-blocking) | PWR dismiss → return to prior screen; recording continues |
| CHARGING | charger connected | advisory | PWR dismiss → return to prior screen |
| NO SD CARD | card absent / unreadable | blocking | BOOT `exit (hold)` → IDLE |
| STORAGE FULL | no space to record | blocking | PWR `OK` → IDLE (in-progress note stopped and kept) |
| TIME NOT SET | RTC stopped / never set | mild | PWR `continue` → IDLE; recording allowed, timestamps unset |
| SLEEP | 120 s inactivity (from IDLE) | transient | any button → wake to IDLE |

Notes:

- **Advisory screens don't abort the current activity** — dismissing LOW BATTERY
  during RECORDING returns to the live recording, not IDLE.
- LOW BATTERY / CHARGING currently require a keypress to dismiss. Whether they
  should **auto-return** after a moment instead is **UX-7** in
  `open-ux-questions.md`.
- These interrupts are independent of the core transition table and the UX-1
  global-rule question.

## Power interaction

scribr **boots directly into IDLE** — there is no splash/boot screen in v1 (see
`ui-screens.md`). IDLE is the only state that sleeps: after **120 s** of
inactivity the device enters light-sleep (v1; see
`../adr/0006-power-and-sleep-model.md`), drawing the SLEEP screen first. Any
button wakes it back to IDLE. Activity in any active state resets the inactivity
timer. The exact sleep depth (light vs deep) is provisional pending battery
testing.

See also `ui-screens.md`, `time-power-spec.md`, and
`../adr/0006-power-and-sleep-model.md`.
