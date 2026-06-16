# scribr — Requirements

scribr is custom firmware for a small, battery-powered, two-button, 1-bit
e-paper **voice-note recorder**, built on the Waveshare ESP32-S3 1.54" e-Paper
board (`S3_ePaper_1_54`, ESP32-S3-PICO-1). It records mono voice notes to an SD
card, timestamps them from an on-board RTC, and lets you browse, play, and
delete them with two physical buttons — all rendered as static, high-contrast
1-bit screens.

The visual design is a finished handoff from Claude Design; the firmware under
`scribr/` today is a **UX-only prototype** (fake recordings, simulated timers,
no real subsystems) used to validate the screen flow on real hardware. It is
fully reworkable. These documents define what the *real* firmware must do.

## v1 scope (MVP)

A **standalone device** — that nonetheless lays the architectural foundations for a future sync path.

In scope for v1:

- The five-state recorder UX (see `state-machine.md`, `ui-screens.md`).
- Real audio capture + playback via the ES8311 codec (`recording-playback-spec.md`).
- SD-card storage with per-note metadata; the list is built by scanning the card.
- RTC timekeeping; UTC stored, local time shown via a configurable offset.
- Battery sensing with a low-battery warning.
- Light-sleep after 120 s of inactivity to conserve battery.
- The edge/error screens the hardware brief implies (no-SD, low-battery, etc.).

Explicitly **out** of v1, but designed-for:

- **BLE file sync** to a future iPhone/Android companion app. This is the
  intended connectivity direction (not a WiFi web "portal" the design tokens
  reference). v1 ships a transfer/sync *seam*, not the feature — see
  `../adr/0007-connectivity-and-sync-seam.md`.
- WiFi upload, server-side transcription, and any web UI.
- Audio (WiFi) NTP sync — the RTC is set manually in v1; the time model is built
  so NTP can slot in later (`../adr/0005-time-model.md`).

## Document map

**Product & behaviour**

- `state-machine.md` — canonical states, transitions, button semantics, global rules.
- `ui-screens.md` — pointer to the canonical screen designs in Claude Design, plus what they don't yet cover.
- `open-ux-questions.md` — central registry of deferred interaction/UX decisions (UX-n).
- `open-technical-decisions.md` — central registry of deferred engineering decisions (TD-n).
- `non-functional.md` — battery, responsiveness, durability, storage, and other NFRs.
- `acceptance-criteria.md` — pass/fail, provisional, and blocked criteria for implementation readiness.

**Subsystem specs** (obligations a compatible implementation must honor)

- `recording-playback-spec.md` — audio format, capture, and playback rules.
- `time-power-spec.md` — RTC, battery, power rails, and sleep specifics.
- `boot-configuration.md` — SD-card boot config for local offset and manual RTC bootstrap.

**Board reference** — descriptive board/panel facts, not requirements; in `../reference/`:

- `../reference/hardware-pinout.md` — complete GPIO map for the board.
- `../reference/device-rendering-constraints.md` — what the 1-bit GFX panel can actually render.
- `../reference/rtc-pcf85063.md` — PCF85063 RTC register protocol.
- `../reference/audio-codec-es8311.md` — ES8311 mono codec hardware facts.

**Decisions** — architecture decision records live in `../adr/`; start at
`../adr/README.md`.

## Status of the requirements

The hardware/subsystem specs were extracted from the reference firmware and are
considered stable. The behaviour docs (`state-machine.md`, `ui-screens.md`)
reconcile three sources that had drifted — the user's original state-machine
spec, the interactive prototype (`scribr_app.jsx`), and the on-device prototype
(`screens.cpp`) — and record the resolved canonical behaviour. The edge-state
screens still need a 1-bit design pass.
