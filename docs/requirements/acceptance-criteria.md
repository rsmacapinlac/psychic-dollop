# Acceptance Criteria

This document defines the initial implementation acceptance criteria for scribr
v1. It is intentionally explicit about confidence level: some criteria are final
because the requirements/ADRs decide them; some are provisional hardware
validation gates; some are blocked by open UX/technical decisions.

## Status labels

| Status | Meaning |
| ------ | ------- |
| **Final** | Requirement/ADR is decided; implementation should satisfy this unless the source requirement changes. |
| **Provisional** | Target is testable, but may be tuned after hardware measurement. Record results and update the requirement if changed. |
| **Blocked** | Acceptance depends on an open UX/technical decision. Use the documented interim default only for prototypes/spikes. |

## State machine & buttons

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-SM-01 | Final | Device boots directly into **IDLE**; no splash/boot screen is shown in v1. | `state-machine.md`, `ui-screens.md` | Power-cycle device; first rendered usable screen is IDLE. |
| AC-SM-02 | Final | From IDLE, `PWR double` starts RECORDING with elapsed time at 0. | `state-machine.md` | Manual/button test. |
| AC-SM-03 | Final | From IDLE, `BOOT double` enters RECORDINGS_LIST with selection index 0. | `state-machine.md` | Manual/button test with non-empty and empty list. |
| AC-SM-04 | Final | During RECORDING, `PWR short` saves the note and returns to IDLE. | `state-machine.md` | Record test note; verify file/meta committed. |
| AC-SM-05 | Final | During RECORDING, `BOOT short` cancels/discards the take and returns to IDLE. | `state-machine.md` | Cancel recording; verify no committed note remains. |
| AC-SM-06 | Final | In non-empty RECORDINGS_LIST, `BOOT short` advances selection and wraps at the end. | `state-machine.md` | Manual/button test with >=4 notes. |
| AC-SM-07 | Final | In non-empty RECORDINGS_LIST, `PWR short` starts PLAYBACK of selected note. | `state-machine.md` | Manual/button/audio test. |
| AC-SM-08 | Final | In non-empty RECORDINGS_LIST, `PWR long` enters DELETE_CONFIRM. | `state-machine.md`, UX-1 interim | Manual/button test. |
| AC-SM-09 | Final | In empty RECORDINGS_LIST, only `BOOT long` exits to IDLE. | `state-machine.md` | Manual/button test with zero notes. |
| AC-SM-10 | Final | In DELETE_CONFIRM, `PWR short` deletes selected note and returns to RECORDINGS_LIST. | `state-machine.md` | Verify `.wav` and `.meta` removed. |
| AC-SM-11 | Final | In DELETE_CONFIRM, `BOOT short` cancels delete and returns to RECORDINGS_LIST. | `state-machine.md` | Verify files remain. |
| AC-SM-12 | Final | In PLAYBACK, `PWR short` stops playback and returns to IDLE. | `state-machine.md` | Manual/audio test. |
| AC-SM-13 | Final | PLAYBACK auto-finishes to IDLE when selected note reaches end. | `state-machine.md` | Play short note to completion. |
| AC-SM-14 | Blocked | Global `PWR long` escape behavior is not implemented beyond documented state-specific behavior unless UX-1 is resolved. | UX-1 | Code review + manual tests. |
| AC-SM-15 | Provisional | OneButton timing is explicitly configured: long press 600 ms, double-click 200 ms, debounce/short handling per ADR 0009. | `state-machine.md`, ADR 0009 | Instrumented button timing test. |

## Display & screens

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-UI-01 | Final | Core screens render: IDLE, RECORDING, RECORDINGS_LIST, empty list, PLAYBACK, DELETE_CONFIRM. | `ui-screens.md` | Visual comparison on panel against Claude Design. |
| AC-UI-02 | Final | Condition screens render: SLEEP, LOW BATTERY, CHARGING, NO SD CARD, STORAGE FULL, TIME NOT SET. | `ui-screens.md`, ADR 0008 | Force each condition and compare on panel. |
| AC-UI-03 | Final | Happy-path and condition screens use the canonical Claude Design layouts; prototype code is not source of truth where design differs. | `ui-screens.md` | Design review. |
| AC-UI-04 | Final | Full refresh is used on screen changes; partial refresh is used for isolated live timer/progress updates. | ADR 0001 | Code review + visual test. |
| AC-UI-05 | Provisional | Timer updates during RECORDING/PLAYBACK occur once per second without visible layout glitches. | `non-functional.md`, ADR 0001 | 5-minute recording/playback observation. |
| AC-UI-06 | Provisional | Periodic cleanup full-refresh cadence for long REC/PLAY is measured/tuned and documented. | ADR 0001 | Hardware ghosting test; record chosen cadence. |
| AC-UI-07 | Blocked | RECORDING live treatment follows current record-dot/doubled-rule/big-timer design unless UX-4 is resolved. | UX-4 | Visual review. |
| AC-UI-08 | Blocked | Full date display on PLAYBACK/DELETE_CONFIRM uses current full date unless it clips on glass; fallback is blocked by UX-6. | UX-6 | Hardware visual test with longest date string. |
| AC-UI-09 | Blocked | LOW BATTERY/CHARGING advisory screens use keypress dismiss unless UX-7 resolves auto-return. | UX-7 | Manual condition test. |

## Recording & playback

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-AUD-01 | Final | Recordings are stored as WAV RIFF/WAVE PCM, 44-byte header, mono, 16 kHz, 16-bit signed little-endian. | `recording-playback-spec.md` | Inspect file header and PCM data. |
| AC-AUD-02 | Final | Capture opens codec as 16 kHz, 2-channel, 16-bit, input gain 45.0, then stores left channel only as mono. | `recording-playback-spec.md`, TD-2 interim | Code review + capture test. |
| AC-AUD-03 | Final | WAV header is backfilled after stream length is known. | `recording-playback-spec.md` | Record file; verify RIFF/data sizes match actual file length. |
| AC-AUD-04 | Final | Recordings shorter than approximately 1000 bytes are discarded. | `recording-playback-spec.md` | Create sub-threshold take; verify no committed note. |
| AC-AUD-05 | Final | Playback rejects files of 44 bytes or less. | `recording-playback-spec.md` | Place header-only WAV; attempt playback. |
| AC-AUD-06 | Final | Playback skips WAV header, reads mono PCM, duplicates samples to stereo for codec output. | `recording-playback-spec.md` | Code review + audible playback. |
| AC-AUD-07 | Provisional | Recorded voice is intelligible with no obvious dropouts, clipping, or repeated buffer underruns in a 30-second test. | `non-functional.md` | Hardware listening test + serial diagnostics. |
| AC-AUD-08 | Provisional | A 10-minute recording completes without crash, file corruption, or audio-buffer overrun. | `non-functional.md` | Long-record stress test. |
| AC-AUD-09 | Blocked | Audio remains fixed at 16 kHz/16-bit unless TD-3 is resolved differently. | TD-3 | Code review. |

## Storage & metadata

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-STO-01 | Final | SD card is the source of truth; the list is built by scanning `/recordings`, not from a fixed RAM recording array. | ADR 0004 | Code review; reboot with preloaded card. |
| AC-STO-02 | Final | Each note is a directory `/recordings/<YYYYMMDDTHHMMSS>/` containing `session.wav` and `session.meta`. | ADR 0004 | Record note; inspect SD card. |
| AC-STO-03 | Final | The recording directory name is the UTC capture time (`%Y%m%dT%H%M%S`); an `unset-NNN` fallback is used only when the clock is not set. No separate index/counter file is kept. | ADR 0004 | Record with clock set and unset; inspect SD card. |
| AC-STO-04 | Final | Creation time comes from the directory name; `session.meta` contains at least `duration_sec=`. | ADR 0004 | Inspect directory name and metadata file. |
| AC-STO-05 | Final | RECORDINGS_LIST sorts notes newest first by metadata creation time. | ADR 0004 | Preload known timestamps; verify order. |
| AC-STO-06 | Final | Deleting a note removes both `.wav` and `.meta`; list updates and selection clamps. | ADR 0004, `state-machine.md` | Delete first/middle/last note. |
| AC-STO-07 | Provisional | Hundreds of notes can be scanned/listed without exhausting RAM; UI still pages the 3-row window. | `non-functional.md`, ADR 0004 | Preload large card test. |
| AC-STO-08 | Provisional | Interrupted/corrupt/incomplete notes do not crash list scan. Recovery behavior is logged and deterministic. | `non-functional.md`, ADR 0004 | Fault-injection with orphan/corrupt files. |

## Boot configuration, RTC, and time

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-TIME-01 | Final | Firmware reads `/scribr.cfg` from SD root at boot if available. | `boot-configuration.md` | Boot with config; verify parsed values. |
| AC-TIME-02 | Final | Missing/unreadable `/scribr.cfg` is not fatal; safe defaults are used. | `boot-configuration.md` | Boot without file. |
| AC-TIME-03 | Final | `local_offset_min` controls display conversion only; stored metadata remains UTC. | `boot-configuration.md`, ADR 0005 | Set offset; record note; inspect display and `.meta`. |
| AC-TIME-04 | Final | Invalid `local_offset_min` is ignored independently and defaults to 0 for that boot. | `boot-configuration.md` | Boot malformed config. |
| AC-TIME-05 | Final | Valid `rtc_set_utc` can set system clock and RTC at boot. | `boot-configuration.md`, ADR 0005 | Boot with known timestamp; read RTC/system time. |
| AC-TIME-06 | Final | `rtc_set_utc` is not repeatedly applied as stale time on every boot; successful consumption is marked or otherwise remembered. | `boot-configuration.md` | Boot twice with same config; verify no stale reset. |
| AC-TIME-07 | Final | RTC validity checks reject OS bit/stopped oscillator and invalid/suspicious dates. | ADR 0005, `time-power-spec.md` | Simulate/force invalid RTC. |
| AC-TIME-08 | Final | If RTC is invalid and no valid bootstrap exists, TIME NOT SET is surfaced, recording is allowed, and bogus timestamps are not written. | ADR 0005, ADR 0008, `state-machine.md` | Boot invalid RTC; record note; inspect metadata. |
| AC-TIME-09 | Final | Future app path can read/update `/scribr.cfg` or call an API that persists changes back to that file. | `boot-configuration.md`, ADR 0007 | Interface/API review; v1 may stub. |

## Battery, charging, and power

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-PWR-01 | Final | Battery ADC reads GPIO4 using ADC1_CH3 / 11 dB attenuation, averages 16 samples, 2 ms apart, and applies ×2 divider. | `time-power-spec.md` | Code review + measured voltage comparison. |
| AC-PWR-02 | Final | Battery percent uses documented piecewise curve and snaps to nearest 5%. | `time-power-spec.md` | Unit/calculation tests. |
| AC-PWR-03 | Final | LOW BATTERY triggers at ≤15% and recovers at ≥20%, checked every 30 s. | `time-power-spec.md`, ADR 0008 | Simulated ADC test. |
| AC-PWR-04 | Final | LOW BATTERY is full-screen, non-blocking, dismissible, and does not abort active recording. | ADR 0008, `state-machine.md` | Force during recording. |
| AC-PWR-05 | Final | CHARGING is full-screen advisory and dismissible. | ADR 0008, `state-machine.md` | Charger connect test. |
| AC-PWR-06 | Final | VBAT latch is asserted deliberately and never dropped unintentionally. | ADR 0006, `time-power-spec.md` | Code review + hardware power test. |
| AC-PWR-07 | Provisional | Battery gauge accuracy is measured against actual pack voltage and result recorded. | TD-1 | Bench measurement. |

## Sleep/wake

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-SLP-01 | Final | Only IDLE enters sleep. Active states do not sleep due to the 120 s idle timer. | ADR 0006, `state-machine.md` | Wait tests in each state. |
| AC-SLP-02 | Final | After 120 s of IDLE inactivity, SLEEP screen is drawn before entering sleep. | ADR 0006, ADR 0008 | Idle timeout test. |
| AC-SLP-03 | Final | Either button wakes device back to IDLE. | ADR 0006, `state-machine.md` | Sleep/wake button test. |
| AC-SLP-04 | Provisional | v1 uses light sleep unless TD-4 is changed after battery measurement. | ADR 0006, TD-4 | Code review + current measurement. |
| AC-SLP-05 | Provisional | Idle sleep current draw and wake latency are measured and recorded. | `non-functional.md`, TD-4 | Bench measurement. |

## Edge/error conditions

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-EDGE-01 | Final | NO SD CARD appears when card is absent/unreadable and blocks record/list actions. | ADR 0008, `state-machine.md` | Boot/remove SD test. |
| AC-EDGE-02 | Final | NO SD CARD exits to IDLE via `BOOT hold`. | `state-machine.md` | Manual/button test. |
| AC-EDGE-03 | Final | STORAGE FULL blocks new recording; if raised during recording, in-progress note is stopped and kept. | ADR 0008, `state-machine.md` | Fill-card test. |
| AC-EDGE-04 | Final | STORAGE FULL `PWR OK` returns to IDLE. | `state-machine.md` | Manual/button test. |
| AC-EDGE-05 | Final | TIME NOT SET `PWR continue` returns to IDLE. | `state-machine.md` | Invalid RTC test. |
| AC-EDGE-06 | Provisional | Condition priority is deterministic when multiple conditions are present; priority table should be added before implementation finalization. | Gap identified | Multi-fault tests after priority is specified. |

## Connectivity / sync / config seam

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-SYNC-01 | Final | v1 ships no BLE/WiFi sync feature. | ADR 0007 | Code review. |
| AC-SYNC-02 | Final | A seam/interface exists for future note enumeration, metadata access, byte reads, and progress reporting. | ADR 0007 | Interface review / compile test. |
| AC-SYNC-03 | Final | The seam/interface leaves room for future `/scribr.cfg` read/update or equivalent config API persisted to that file. | ADR 0007, `boot-configuration.md` | Interface review. |
| AC-SYNC-04 | Final | Storage and state-machine layers remain transport-agnostic. | ADR 0007 | Architecture/code review. |

## System/integration validation

| ID | Status | Criterion | Source | Verification |
| -- | ------ | --------- | ------ | ------------ |
| AC-INT-01 | Provisional | Full sketch compiles for `esp32s3` profile with pinned dependencies. | `sketch.yaml` | `arduino-cli compile --profile esp32s3 scribr`. |
| AC-INT-02 | Provisional | Button input remains responsive while display refresh is in progress. | `non-functional.md`, ADR 0002 | Press during full/partial refresh. |
| AC-INT-03 | Provisional | Display refresh during REC/PLAY does not cause audible playback glitches or recording buffer underruns in normal operation. | ADR 0002, `non-functional.md` | 5-minute REC/PLAY stress test. |
| AC-INT-04 | Provisional | 30-minute recording stress test completes without crash, corrupt file, or runaway memory use. | `non-functional.md` | Long-run hardware test. |
| AC-INT-05 | Provisional | Repeated sleep/wake cycles do not power off unexpectedly or lose required state. | ADR 0006 | 20-cycle sleep/wake test. |

## Items that still need refinement

Before implementation is considered fully ready, define or refine:

1. Condition priority table for simultaneous faults, especially NO SD + TIME NOT SET + LOW BATTERY.
2. Exact storage recovery policy for orphan/corrupt `.wav`/`.meta` pairs.
3. Exact `/scribr.cfg` mark-applied strategy for `rtc_set_utc`.
4. Concrete sync/config seam interface signatures.
5. Battery-life target that determines whether TD-4 moves from light sleep to deep sleep.
6. Hardware-tuned display ghosting cleanup cadence.
