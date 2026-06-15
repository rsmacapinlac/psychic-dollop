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
  `2026-06-14T09:32:00Z`). Stored per note as `created_utc=` in the `.meta` file.
- **NTP sync:** servers `pool.ntp.org`, `time.google.com`, `time.cloudflare.com`;
  timeout **6 s** (boot retry) to **8 s**. NTP runs only when WiFi is up.
- **Local time for display only:** `LOCAL_TIME_OFFSET_MIN` (default **120** =
  UTC+2). Applied when rendering labels; **storage stays UTC**.

### PCF85063 register access (reimplementation reference)

The reference used a board I²C BSP that is **not** vendored into scribr, so the
register protocol is captured here. Time is one 7-byte block starting at
`0x04`, all **BCD**:

| Offset | Reg  | Field    | Read mask | Notes                          |
| ------ | ---- | -------- | --------- | ------------------------------ |
| 0      | 0x04 | seconds  | `& 0x7F`  | bit 7 = **OS** (osc stopped)   |
| 1      | 0x05 | minutes  | `& 0x7F`  |                                |
| 2      | 0x06 | hours    | `& 0x3F`  | 24-hour mode                   |
| 3      | 0x07 | days     | `& 0x3F`  | day of month                   |
| 4      | 0x08 | weekday  | `& 0x07`  | 0–6                            |
| 5      | 0x09 | months   | `& 0x1F`  | 1–12                           |
| 6      | 0x0A | years    | full byte | add 2000                       |

- **BCD:** `dec = (v>>4)*10 + (v&0x0F)`; `bcd = ((v/10)<<4) | (v%10)`.
- **Validity:** if seconds bit 7 (OS) is set, the oscillator stopped → time is
  invalid (needs re-set). Reference also range-checks year 2024–2099, month
  1–12, day 1–31, h/m/s in range.
- **Write:** same 7 registers from `0x04`, BCD, with `year - 2000`.
- **Epoch:** set `TZ=UTC0`, `tzset()`, then `mktime()`; a synced clock is
  sanity-checked as `epoch >= 1700000000`.

### Decisions to revisit
- Single fixed offset constant — **no timezone database, no DST** handling.
- Local offset must be set manually per region/season.

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

### Decisions to revisit
- The curve is a rough Li-ion approximation (5% granularity) — fine for a gauge,
  not for fuel-gauge accuracy.
- The ×2 divider ratio assumes the board's specific resistor divider.

## Power rails & latch

All rails are **active-low** enables.

| Rail            | GPIO | Notes                                          |
| --------------- | ---- | ---------------------------------------------- |
| VBAT power latch | 17  | Software hold — must be asserted to stay on    |
| E-paper rail    | 6    | Powers the display panel                       |
| Audio rail      | 42   | Powers the codec / amp                         |

The board stays on only while the **VBAT latch** is held; releasing it cuts power.

## Sleep

- **Trigger:** deep ("ultra") sleep after **120 s** of inactivity
  (`ULTRA_SLEEP_MS`), tracked via `lastActivityMs` / `resetActivity()`.
- **Entry sequence:** show sleep screen → stop transfer server → WiFi off →
  mute audio → keep VBAT latched → arm wake → `esp_deep_sleep_start()`.
- **Wake source:** EXT1 on **BTN_REC (GPIO0)** and **BTN_PWR (GPIO18)**,
  `ESP_EXT1_WAKEUP_ANY_LOW` (any button pressed wakes the device).

### Decisions to revisit
- 120 s timeout is a fixed constant.
- Only the two buttons wake the device; no timed/RTC wake is configured.

See also `docs/requirements/recording-playback-spec.md` and
`docs/device-rendering-constraints.md`.
