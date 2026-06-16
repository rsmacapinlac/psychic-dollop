# Boot Configuration

scribr reads a small user-editable configuration file from the SD card at boot.
This is the v1 mechanism for configuring local time display and for manually
bootstrapping the RTC without adding an on-device settings UI. In a future
companion-app release, the app should be able to read and update this same file
through the sync/config seam, rather than introducing a second configuration
source.

## File location

The boot configuration file lives at the SD-card root:

```text
/scribr.cfg
```

If the file is missing, unreadable, or contains invalid keys, the firmware must
fall back to safe defaults and surface any relevant condition screen rather than
crashing or inventing values.

## Format

Plain UTF-8 / ASCII text, one `key=value` pair per line.

- Blank lines are ignored.
- Lines beginning with `#` are comments.
- Unknown keys are ignored, but should be logged over serial when possible.
- Invalid values are ignored independently; one bad key must not invalidate the
  whole file.

Example:

```ini
# scribr boot configuration
# Local display offset from UTC, in minutes. Example: -300 = UTC-5.
local_offset_min=120

# Optional one-shot manual RTC bootstrap. UTC only.
# Remove, comment, or let firmware mark this applied after a successful set.
rtc_set_utc=2026-06-15T09:32:00Z
```

## Keys

| Key | Required | Meaning |
| --- | -------- | ------- |
| `local_offset_min` | No | Signed integer minutes added to UTC for display only. Storage remains UTC. Default: `0` if missing/invalid. |
| `rtc_set_utc` | No | Optional manual UTC timestamp used to set the RTC/system clock. Format: `YYYY-MM-DDTHH:MM:SSZ`. |

## Behaviour

### Local offset

- `local_offset_min` is read on every boot after SD mount.
- It affects display labels only.
- It must never alter stored `created_utc` metadata.
- If missing or invalid, firmware uses `0` minutes and should log the fallback.

### RTC bootstrap

`rtc_set_utc` exists to solve the v1 clock-setting problem without adding a
settings screen.

- The value is UTC only, using the same ISO-8601 format as note metadata.
- If present and valid, the firmware may use it to set the system clock and RTC.
- This is a **manual bootstrap**, not an ongoing time source. It must not silently
  reset a valid RTC to a stale timestamp on every boot.
- After a successful set, the firmware should prevent repeated application by
  marking the value applied, commenting/removing the key, or otherwise recording
  that this exact value has already been consumed.
- If the RTC is invalid and no valid `rtc_set_utc` is provided, the firmware must
  follow the TIME NOT SET behaviour: recording is allowed, bogus timestamps are
  not written, and the TIME NOT SET condition screen is surfaced.

## Future companion-app interaction

The future BLE companion app should treat `/scribr.cfg` as the device's stable
configuration file for settings that are safe to expose to users, including at
least:

- `local_offset_min`
- RTC/time bootstrap or replacement time-set mechanism

The app may update `/scribr.cfg` directly through a future config/transfer seam,
or use an equivalent device-side API that persists changes back to this file.
Either way, `/scribr.cfg` remains the on-device source of truth for these v1-era
settings unless a later ADR supersedes it.

App/device updates must avoid corrupting the file:

- write changes atomically where possible, for example temp file + rename;
- preserve unknown keys/comments where practical;
- validate values before committing them;
- avoid racing with boot-time reads or other storage operations.

## SD-card failure interaction

Because the config file lives on the SD card, SD failure can also prevent reading
the local offset or manual RTC bootstrap.

- Missing/unreadable config alone is not a fatal error.
- Missing/unreadable SD card is handled by the NO SD CARD condition flow.
- If SD is unavailable, local display offset falls back to `0` for that boot.

## Security / validation

- Clamp `local_offset_min` to a reasonable range, recommended `-1440` to `+1440`.
- Reject malformed timestamps.
- Reject timestamps that fail the same sanity checks as the RTC model
  (`2024–2099`, epoch `>= 1700000000`).
- Never write a bogus or local-time timestamp into note metadata.

See also `time-power-spec.md`, `open-ux-questions.md` UX-5, and
`../adr/0005-time-model.md`.
