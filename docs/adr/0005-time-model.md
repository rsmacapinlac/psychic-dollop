# 0005 — Time Model: RTC, UTC Storage, Configurable Local

**Status:** Accepted

## Context

Notes are timestamped from the on-board **PCF85063** RTC (I²C 0x51, BCD
registers; protocol in `../requirements/time-power-spec.md`). v1 ships no WiFi, so there is no
NTP at boot — but the brief's future direction includes connectivity, and the
companion-app path could also set time. The reference firmware hard-codes a
single local offset (`LOCAL_TIME_OFFSET_MIN = 120`, UTC+2), which is wrong for
most users.

## Decision

- **Canonical time is UTC.** The system clock and the RTC chip both hold UTC and
  sync both ways. Per-note `created_utc` is stored in ISO-8601 UTC
  (`%Y-%m-%dT%H:%M:%SZ`) in the `.meta` sidecar (ADR 0004).
- **Local time is display-only**, computed from a **configurable** offset (not the
  fixed +120). The offset is a user setting read from the SD-card boot config
  (`/scribr.cfg`, key `local_offset_min`); no timezone database or DST in v1.
- **Setting the clock in v1:** manual via the SD-card boot config. An optional
  `rtc_set_utc=` value in `/scribr.cfg` can bootstrap the RTC/system clock at
  boot. This is a manual/one-shot mechanism, not an ongoing time source; repeated
  stale application must be prevented. The model is built so a later
  **NTP-over-WiFi** or **companion-app** time-set drops in without changing the
  stored format.
- **Invalid-clock handling:** if the RTC's oscillator-stopped (OS) bit is set or
  the time fails the sanity checks (year 2024–2099, `epoch ≥ 1700000000`), treat
  the clock as **unset** — still allow recording, but surface a "time not set"
  state (ADR 0008) rather than writing bogus timestamps.

## Consequences

- Timestamps are unambiguous and portable; display adapts per user without
  touching stored data.
- A new code seam for "set time" with two future providers (NTP, companion app).
- Users in seasonal DST regions must adjust the offset manually — acceptable for
  v1, revisit with connectivity.

## Future

- The v1 mechanism is the SD-card boot config (`../requirements/boot-configuration.md`),
  not an on-device settings screen. Future UX may still add companion-app or
  on-device editing; the companion app should be able to update `/scribr.cfg` or
  use a device-side API that persists changes back to that file. The firmware
  must not require the app for v1.

## Alternatives

- **Fixed UTC+2 offset** — rejected: wrong for most users; trivially avoidable.
- **Store local time** — rejected: breaks the moment the offset changes and
  complicates sync.
- **Full TZ database / DST** — rejected for v1: heavy for the value; the offset
  covers the need.
