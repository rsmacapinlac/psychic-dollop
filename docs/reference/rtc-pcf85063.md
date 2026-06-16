# PCF85063 RTC — Register Reference

Reimplementation reference for the on-board **PCF85063** real-time clock. The
reference firmware used a board I²C BSP that is **not** vendored into scribr, so
the raw register protocol is captured here. For the timekeeping *policy* (UTC
storage, configurable local offset, NTP, validity handling) see
`../requirements/time-power-spec.md` and `../adr/0005-time-model.md`.

- **Bus / address:** I²C (`I2C_NUM_0`) at **0x51** (see `hardware-pinout.md`).
- **Encoding:** all time registers are **BCD**.

Time is one 7-byte block starting at `0x04`:

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

See also `hardware-pinout.md` (I²C bus) and `../requirements/time-power-spec.md`.
