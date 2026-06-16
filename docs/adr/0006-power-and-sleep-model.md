# 0006 — Power & Sleep Model

**Status:** Accepted (sleep **depth** provisional, pending battery testing)

## Context

Battery life is a core product goal. The board stays powered only while software
holds the **VBAT latch** (GPIO17; HIGH holds power, LOW releases it); rails for
e-paper (GPIO6) and audio (GPIO42) are independently gated active-low enables.
The reference firmware deep-sleeps after 120 s of inactivity and wakes via EXT1
on the two buttons. The user wants to
**measure real battery life with light-sleep first**, then decide whether deep
sleep is warranted — so the inactivity *trigger* is settled but the sleep
*depth* is not.

## Decision

- **Inactivity timeout: 120 s**, tracked from the last input/activity and reset
  by any button or state activity. Only **IDLE** sleeps; active states (REC,
  PLAY, LIST, DELETE) hold the device awake.
- **v1 sleep depth: light-sleep.** On timeout: show a brief sleep screen, quiesce
  what's safe (audio rail off, mute), keep the VBAT latch held and the e-paper
  image (zero-power retention), then enter light-sleep. Any button wakes the
  device back to **IDLE**.
- **Pluggable depth.** Wrap sleep entry/exit behind a small `power` seam so deep
  sleep is a drop-in alternative. Critically, **do not rely on deep-sleep restart
  semantics** anywhere else: keep model state in RAM and treat wake as "resume to
  IDLE," which holds for both light and (with state restore) deep sleep.
- **Wake source:** the two buttons (EXT1 / GPIO wake, any-low). No timed/RTC wake
  in v1.
- **Latch discipline:** the firmware must never drop the VBAT latch
  unintentionally; releasing it is an explicit power-off action only.

## Consequences

- We can measure light-sleep battery life on real hardware before committing to
  deep sleep's bigger savings and bigger complexity (cold-boot restore, peripheral
  re-init).
- Because nothing depends on a deep-sleep restart, switching depth later is
  localized to the `power` seam.
- Light-sleep retains RAM/peripherals, so wake-to-IDLE is instant and simple.

## Alternatives

- **Deep sleep now** — deferred: best battery, but adds restart/restore
  complexity before we know we need it.
- **Tunable timeout (not 120 s)** — not for v1; keep 120 s constant for a clean
  battery measurement, revisit alongside the depth decision.
- **Stay always awake** — rejected: contradicts the core battery goal.

## Open

Tracked centrally in `../requirements/open-technical-decisions.md`:

- Final sleep **depth** after battery measurement — **TD-4**.
- Whether long REC/PLAY sessions need any low-power treatment — **TD-5**.
