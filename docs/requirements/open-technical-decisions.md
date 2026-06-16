# Open Technical Decisions

A living registry of engineering decisions that are deliberately unresolved or
rest on assumptions worth revisiting — the technical counterpart to
`open-ux-questions.md`. Nothing here is settled; each entry notes an interim
default so implementation isn't blocked. When one is decided, fold the outcome
into the relevant ADR/spec and mark the entry **Resolved** (with the date and
where it landed).

Each has a stable ID (`TD-n`). "Owner" names an ADR when one already frames the
decision; standalone entries have no ADR yet.

| ID | Decision | Status | Owner |
| -- | -------- | ------ | ----- |
| TD-1 | Battery gauge accuracy | Open | — |
| TD-2 | Audio down-mix method | Open | — |
| TD-3 | Audio quality ceiling (16 kHz / 16-bit) | Open | — |
| TD-4 | Sleep depth (light vs deep) | Open | ADR 0006 |
| TD-5 | Low-power treatment during long REC/PLAY | Open | ADR 0006 |

---

## TD-1 — Battery gauge accuracy

**Status:** Open · **Related:** `time-power-spec.md` (Battery)

The voltage→percent curve is a rough Li-ion approximation snapped to 5% — fine
for a gauge, not fuel-gauge accurate. The ×2 divider ratio also assumes this
board's specific resistor divider. Revisit if the gauge proves too coarse, or if
the hardware divider differs.

**Interim default:** keep the reference curve and ×2 ratio.

## TD-2 — Audio down-mix method

**Status:** Open · **Related:** `recording-playback-spec.md`

Capture keeps the **left** channel only. Averaging L+R is moot while the codec is
mono (both slots carry the same mic) but becomes a real choice if multi-mic
hardware (e.g. an ES7210) is ever used.

**Interim default:** left-channel-only (matches the mono hardware).

## TD-3 — Audio quality ceiling

**Status:** Open · **Related:** `recording-playback-spec.md`

Fixed 16 kHz / 16-bit mono — adequate for voice and speech-to-text. Raising it
costs storage, and BLE sync time later (`../adr/0007-connectivity-and-sync-seam.md`).
Revisit only if a use case needs higher fidelity.

**Interim default:** 16 kHz / 16-bit mono.

## TD-4 — Sleep depth: light vs deep

**Status:** Open · **Owner:** `../adr/0006-power-and-sleep-model.md`

v1 uses light-sleep after 120 s to measure real battery life; deep sleep is the
fallback if life is poor. The trigger (120 s inactivity, buttons-only wake) is
settled; only the depth is open. Decide after measurement.

**Interim default:** light-sleep (v1).

## TD-5 — Low-power treatment during long REC/PLAY

**Status:** Open · **Owner:** `../adr/0006-power-and-sleep-model.md`

Active states currently run at full power. Whether long recording/playback
sessions warrant any power reduction is unknown until measured on hardware.

**Interim default:** full power while active.
