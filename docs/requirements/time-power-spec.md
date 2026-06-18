# Time & Power Spec

Timekeeping, battery, and power/sleep requirements for scribr, extracted from
the reference firmware, which runs on the same **Waveshare ESP32-S3 1.54" e-Paper**
board scribr targets. These are the specs a compatible implementation must honor
when these subsystems are wired into scribr.

## Time / RTC

- **RTC chip:** PCF85063 on I²C (`I2C_NUM_0`) at address **0x51**; registers are
  **BCD** (seconds 0x04 … years 0x0A).
- **Canonical time is UTC.** The system clock and the RTC chip both hold UTC and
  are synced both ways (`rtcSyncSystemFromChip`, `rtcSyncChipFromSystem`).
- **Serialized format:** ISO-8601 UTC, `%Y-%m-%dT%H:%M:%SZ` (e.g.
  `2026-06-14T09:32:00Z`) for display/logging. A note's creation time is persisted
  as its UTC recording-directory name `%Y%m%dT%H%M%S` (ADR 0004), not in the
  sidecar.
- **NTP sync:** servers `pool.ntp.org`, `time.google.com`, `time.cloudflare.com`;
  timeout **6 s** (boot retry) to **8 s**. NTP runs only when WiFi is up.
- **Local time for display only:** applied when rendering labels; **storage stays
  UTC**. scribr makes this offset configurable via the SD-card boot config
  (`/scribr.cfg`, key `local_offset_min`) — see `boot-configuration.md` and ADR
  `../adr/0005-time-model.md`.
- **Manual RTC bootstrap:** v1 has no on-device settings screen. A valid optional
  `rtc_set_utc=` entry in `/scribr.cfg` may be used at boot to set the system
  clock and RTC. This is a one-shot/manual bootstrap mechanism, not an ongoing
  time source; repeated stale application must be prevented. See
  `boot-configuration.md`.

### PCF85063 register access

The raw register protocol — BCD layout, the OS (oscillator-stopped) validity
bit, write sequence, and epoch conversion — lives in the board reference:
`../reference/rtc-pcf85063.md`.

## Battery

- **Sense pin:** GPIO4 (ADC1_CH3), attenuation `ADC_11db`.
- **Reading:** `analogReadMilliVolts`, averaged over **16 samples** (2 ms apart),
  then ×2 for the resistor divider → pack voltage. `v ≤ 0.1 V` → `-1` (invalid).
- **Voltage → percent:** piecewise-linear, snapped to the nearest **5%**:

  | Voltage | 3.20 V | 3.40 V | 3.70 V | 3.90 V | 4.20 V |
  | ------- | ------ | ------ | ------ | ------ | ------ |
  | Percent | 0%     | 25%    | 50%    | 75%    | 100%   |

  Clamps: `≥ 4.20 V → 100%`, `≤ 3.20 V → 0%` (`≥ 4.35 V` also 100%).
- **Low-battery policy:** warn at **≤ 15%**, recover at **≥ 20%** (hysteresis),
  re-checked every **30 s** (`BAT_CHECK_INTERVAL_MS`).

> Gauge accuracy (the rough 5% curve and board-specific divider) is an open
> technical decision — **TD-1** in `open-technical-decisions.md`.

## Power rails & latch

The e-paper and audio controls are **active-low** peripheral rail enables; the
GPIO map is in the board reference (`../reference/hardware-pinout.md`). The
**VBAT latch** (GPIO17) is not treated as a normal active-low rail: **HIGH holds
board power** and LOW releases the latch for intentional shutdown. The firmware
must assert the latch deliberately and never drop it unintentionally, or the
board powers off.

## Sleep

- **Trigger:** deep ("ultra") sleep after **120 s** of inactivity
  (`ULTRA_SLEEP_MS`), tracked via `lastActivityMs` / `resetActivity()`.
- **Entry sequence:** show sleep screen → stop transfer server → WiFi off →
  mute audio → keep VBAT latched → arm wake → `esp_deep_sleep_start()`.
- **Wake source:** EXT1 on **BTN_REC (GPIO0)** and **BTN_PWR (GPIO18)**,
  `ESP_EXT1_WAKEUP_ANY_LOW` (any button pressed wakes the device).

> The above describes the reference firmware's **deep** sleep. scribr v1 uses
> **light**-sleep after the same 120 s with buttons-only wake; the sleep depth is
> open — see ADR `../adr/0006-power-and-sleep-model.md` and **TD-4**.

See also `docs/requirements/recording-playback-spec.md` and
`docs/reference/device-rendering-constraints.md`.
