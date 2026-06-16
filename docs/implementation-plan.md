# scribr v1 Production Firmware Implementation Plan

Status: initial execution plan, created from `~/Downloads/start-implementation-prompt.md` and the current requirements/ADR set.

## Source-of-truth inputs

- `docs/requirements/README.md`
- `docs/requirements/state-machine.md`
- `docs/requirements/ui-screens.md`
- `docs/requirements/recording-playback-spec.md`
- `docs/requirements/time-power-spec.md`
- `docs/requirements/boot-configuration.md`
- `docs/requirements/non-functional.md`
- `docs/requirements/acceptance-criteria.md`
- `docs/requirements/open-ux-questions.md`
- `docs/requirements/open-technical-decisions.md`
- `docs/adr/0001-display-and-rendering-architecture.md` through `0009-button-input-handling.md`
- `docs/reference/`

The current `scribr/` firmware is treated as a disposable UX prototype. The display/canvas/async-refresh approach and screen layout code are useful references; fake recording data, fixed RAM list model, and prototype app APIs are not production constraints.

## Proposed production architecture

Layering target: explicit hardware seams, app logic free of raw peripheral details.

```text
scribr/
  scribr.ino
  src/
    hal/
      pins.h
      power.h/.cpp              # rail control, VBAT latch discipline, sleep entry seam
      i2c_bus.h/.cpp            # shared Wire/I2C owner + mutex
      sd_bus.h/.cpp             # SD_MMC pin/bus init
      adc_battery.h/.cpp        # raw battery millivolt sampling
    drivers/
      epaper_driver_bsp.*       # existing raw panel driver
      rtc_pcf85063.h/.cpp       # raw BCD register protocol + validity
      es8311_codec.h/.cpp       # manual ES8311 register init/control
      i2s_audio_link.h/.cpp     # ESP-IDF I2S standard-mode read/write
    services/
      display.h/.cpp            # canvas owner + async full/partial flush task
      buttons.h/.cpp            # OneButton config -> typed button events
      config_store.h/.cpp       # /scribr.cfg parse/write/mark-applied
      clock.h/.cpp              # UTC system/RTC sync + local offset conversion
      battery.h/.cpp            # percent curve + low-battery hysteresis
      storage.h/.cpp            # /notes scan, metadata, note IDs, delete, free-space
      wav_recorder.h/.cpp       # WAV header/backfill, temp/commit/cancel policy
      wav_player.h/.cpp         # WAV validation/playback stop flag
      audio_service.h/.cpp      # app-facing begin/recordTo/play/stop seam
      sleep_service.h/.cpp      # light-sleep now, deep-sleep swappable later
      transfer.h                # v1 stub interface for future BLE/config sync
    app/
      model.h                   # production UI/state model, no fake cap
      state_machine.h/.cpp      # canonical state transitions and condition interrupts
      screens.h/.cpp            # pure render(model)->canvas, adapted from prototype
      app.h/.cpp                # setup/tick orchestration
```

### Key seams

- **Display seam:** keep `GFXcanvas1(200,200)`, full refresh on screen changes, partial refresh for live timer/progress fields. Refresh remains asynchronous on core 0. App never waits on panel refresh.
- **Button seam:** OneButton instances are isolated in a service and dispatch `BOOT/PWR x short/long/double` events to the state machine. Configure 600 ms long and 200 ms double windows.
- **Storage seam:** SD card is source of truth. App asks for note summaries/pages and selected note handles; it never maintains a fixed fake recording list.
- **Audio seam:** state machine calls `startRecording`, `saveRecording`, `cancelRecording`, `startPlayback`, `stopPlayback`; only audio/storage services know I2S, ES8311, WAV, temp files, and metadata commit details.
- **Clock/config seam:** `/scribr.cfg` is the v1 source for `local_offset_min` and optional one-shot `rtc_set_utc`. Future app updates must use this seam or an API that persists back to the same file.
- **Power seam:** rail/latch/sleep operations are centralized. Sleep depth is an implementation detail behind the seam; v1 default is light-sleep.
- **Condition-screen seam:** hardware/system conditions are raised separately from the five core states, with deterministic priority before final integration.

## Subsystem implementation order

Risk-first order, with compileable checkpoints:

1. **Board/pin/power audit**
   - Confirm GPIO map in `docs/reference/hardware-pinout.md` against current code.
   - VBAT latch semantics are now explicit in docs/code: GPIO17 HIGH holds board power, LOW releases it for intentional shutdown. Verify on hardware during bring-up before adding any shutdown path.
2. **Display foundation**
   - Preserve/adapt `display.cpp`, `epaper_driver_bsp.*`, and screen layout code.
   - Add render support for condition screens.
   - Keep async refresh and `busy()` behavior.
3. **Button input service**
   - Encapsulate OneButton configuration and callbacks.
   - Add serial/instrumented button timing diagnostics for hardware validation.
4. **Production app model/state-machine skeleton**
   - Implement the five core states and documented transitions without fake notes/audio.
   - Add condition-screen stack/overlay model using interim UX defaults only where documented.
5. **SD + boot config + RTC/time**
   - Mount SD_MMC in 1-bit mode.
   - Parse `/scribr.cfg` robustly.
   - Implement PCF85063 read/write/validity and UTC/local display conversion.
   - Implement `rtc_set_utc` one-shot mark-applied strategy before enabling repeated boot use.
6. **Battery + advisory plumbing**
   - GPIO4 ADC averaging, ×2 divider, reference percent curve, 15/20% hysteresis, 30 s interval.
7. **Storage + metadata**
   - Create `/notes` if needed, scan notes, derive next index, sort newest first, delete wav+meta.
   - Define deterministic corrupt/orphan recovery behavior.
8. **Audio bring-up in isolation**
   - ES8311 init, audio rail/PA control, I2S standard mode, capture/playback test sketches or guarded firmware modes.
9. **Recording/playback integration**
   - WAV temp/commit/cancel flow, metadata sidecar, duration, storage-full behavior, playback progress/stop.
10. **Sleep/wake behavior**
    - Sleep screen then light-sleep after 120 s IDLE inactivity; either button wakes to IDLE.
11. **Transfer/sync/config seam**
    - Add interfaces only; no BLE/WiFi feature in v1.
12. **Integration validation**
    - Run acceptance criteria in `docs/requirements/acceptance-criteria.md`, update results and tune provisional thresholds.

## Blocking/open decisions and implementation questions

### Existing UX questions

| ID | Implementation handling |
| --- | --- |
| UX-1 | Do not implement global PWR-long escape beyond `LIST -> DELETE_CONFIRM`. Keep app code structured so a global handler can be added later. |
| UX-4 | Use current record dot + doubled rule + big timer. Do not invent inversion or stronger live treatment. |
| UX-6 | Keep full `YYYY-MM-DD HH:MM` on PLAYBACK/DELETE_CONFIRM and verify on panel with longest strings before changing. |
| UX-7 | Use PWR keypress dismiss for LOW BATTERY/CHARGING. Do not add auto-return unless UX is resolved. |

### Existing technical decisions

| ID | Implementation handling |
| --- | --- |
| TD-1 | Use documented battery curve and ×2 divider; measure pack voltage during bring-up. |
| TD-2 | Keep left channel only while hardware is mono. |
| TD-3 | Keep 16 kHz / 16-bit mono. |
| TD-4 | Implement light-sleep behind swappable power seam; measure current. |
| TD-5 | Run active REC/PLAY at full power; collect current/thermal observations. |

### Additional details to settle before full implementation

1. **Condition priority table** for simultaneous conditions, especially NO SD + TIME NOT SET + LOW BATTERY + CHARGING.
2. **VBAT latch hardware verification**: docs/code now state GPIO17 HIGH holds power and LOW releases it; confirm this on the target board before adding any intentional shutdown behavior.
3. **`/scribr.cfg` mark-applied strategy** for `rtc_set_utc`:
   - Preferred v1 proposal: after successful RTC set, rewrite config atomically by commenting the exact consumed line as `# applied rtc_set_utc=...` while preserving unknown keys/comments where practical.
   - Fallback if atomic rename is unreliable on SD_MMC: write a sidecar such as `/scribr.cfg.applied` containing the exact timestamp and epoch of application, then ignore the matching value on later boots.
4. **Future app update semantics** for config:
   - Interface should support read whole config, validate staged updates, atomic write, and preserve unknown/comment lines where practical.
5. **Storage recovery policy**:
   - Proposed: list only notes with a valid WAV and parseable `.meta`; ignore/log orphan WAVs/metas; optionally quarantine incomplete temp files on boot.
6. **Storage-full threshold**:
   - Need a concrete minimum-free-space threshold before starting recording and behavior when free space disappears mid-recording.
7. **Display cleanup full-refresh cadence** during long REC/PLAY.
8. **Charging detection source**:
   - Requirements define CHARGING behavior, but the pin/API for charger-present detection is not in the current pinout. Need board fact or explicit omission/stub until known.

## Acceptance-criteria matrix

Use `docs/requirements/acceptance-criteria.md` as the working matrix. It already covers final/provisional/blocked criteria across:

- state machine and buttons;
- display/screens;
- recording/playback;
- storage/metadata;
- boot config, RTC, and time;
- battery/charging/power;
- sleep/wake;
- edge/error conditions;
- connectivity/sync/config seam;
- integration validation.

Implementation should add a results column or separate validation log once hardware testing starts. Immediate additions/refinements needed before final acceptance:

| Area | Needed refinement |
| --- | --- |
| Edge conditions | Deterministic condition priority table. |
| Storage | Exact orphan/corrupt/temp-file recovery policy. |
| Config | Exact `rtc_set_utc` mark-applied mechanism. |
| Sync seam | Concrete interface signatures for note enumeration, byte reads, metadata access, progress, and config read/update. |
| Power | Battery-life target that determines whether TD-4 changes from light to deep sleep. |
| Display | Hardware-tuned cleanup full-refresh cadence. |
| Charging | Concrete charger-present detection mechanism. |

## Hardware bring-up plan

1. **Power/latch safety**
   - Serial boot log first; verify VBAT latch level required to stay powered.
   - Confirm EPD/audio rail active levels with a meter before enabling audio.
2. **Display**
   - Full/partial refresh smoke test, polarity, ghosting, and async button responsiveness during refresh.
3. **Buttons**
   - Log short/long/double events with timestamps; verify thresholds on both buttons.
4. **SD_MMC**
   - Mount/unmount tests, `/scribr.cfg` read, `/notes` create/scan, free-space reporting.
5. **RTC/I2C**
   - I2C scan for 0x51 and 0x18; read RTC registers; test OS-bit invalid path; test set/readback.
6. **Battery ADC**
   - Compare ADC-derived voltage to multimeter across charge levels; record TD-1 data.
7. **Audio**
   - Codec register init smoke test; capture raw buffers; record WAV; inspect header/data; playback known WAV; watch for I2S underruns.
8. **Concurrency**
   - REC/PLAY with timer partial refreshes; skip/defer UI refresh if audio underruns appear.
9. **Sleep/wake**
   - 120 s IDLE timeout, SLEEP screen retention, light-sleep current, either-button wake to IDLE, repeated cycles.
10. **Fault injection**
    - No SD, full SD, invalid RTC, low battery simulation, corrupt notes, power loss/cancel mid-record.

## Risk register

| Risk | Probability | Impact | Mitigation |
| --- | --- | --- | --- |
| ES8311 raw I2S/manual init fails or is noisy | High | High | Bring up in isolation; keep register code encapsulated; log I2S errors; use known-good WAV/header tests. |
| Display and audio contend on core 0 | Medium | High | Audio priority; defer/skip partial refresh ticks under audio load; measure underruns. |
| SD write interruption corrupts notes | Medium | High | Temp file + commit discipline; header backfill before metadata commit; robust scan ignoring incomplete files. |
| RTC invalid/stale time writes bogus metadata | Medium | High | Strict OS/sanity checks; TIME NOT SET condition; omit `created_utc` when invalid rather than inventing. |
| `rtc_set_utc` reapplied every boot | Medium | Medium | Implement mark-applied before shipping RTC bootstrap. |
| Battery gauge inaccurate | High | Medium | Treat as provisional; measure against pack voltage; update curve if needed. |
| Light-sleep current insufficient | Medium | Medium | Power seam supports deep-sleep later; collect current/wake latency data for TD-4. |
| VBAT latch release mistake powers off device | Medium | High | GPIO17 semantics are explicit; verify on hardware, centralize latch writes, and avoid release calls in v1 unless intentional. |
| CHARGING screen cannot be triggered because charger detect is unspecified | Medium | Medium | Identify board mechanism or mark as stub/simulated until hardware fact is added. |
| Long date clips on glass | Medium | Low | Verify UX-6; do not choose fallback without UX decision. |

## Immediate next implementation checkpoint

Before broad rewrite, create a small production skeleton that compiles while preserving the existing display prototype as a reference:

1. update `hal/pins.h` with the complete documented pin map;
2. replace the remaining ambiguous `board_power_bsp` API with a clearer `hal::power` seam; keep explicit `holdVbatLatch()` / `releaseVbatLatch()` naming;
3. extract button handling into an `app/buttons` or `services/buttons` module with explicit OneButton thresholds;
4. add condition-screen enum/model support without wiring speculative behavior;
5. add SD/config/RTC compile-time stubs so the app architecture can grow subsystem-by-subsystem.

## Execution log

### 2026-06-15 checkpoint 1

Completed and compile-checked:

- expanded `scribr/src/hal/pins.h` to include documented battery, I2C, I2S/audio, PA, and SD-MMC pins;
- added `scribr/src/hal/power.h/.cpp` with semantic power controls:
  - `holdVbatLatch()` / `releaseVbatLatch()`;
  - `enableEpaperRail()` / `disableEpaperRail()`;
  - `enableAudioRail()` / `disableAudioRail()`;
- updated display startup to use `hal::power` rather than constructing the legacy BSP power object;
- kept legacy `VBAT_POWER_ON/OFF` wrappers in `board_power_bsp` for compatibility, but documented them as deprecated in favor of explicit latch names;
- extracted OneButton setup/polling into `scribr/src/app/buttons.h/.cpp` with explicit 200 ms double-click, 600 ms long-press, and 50 ms debounce configuration;
- added initial compile-time seams:
  - `scribr/src/services/config_store.h/.cpp` for `/scribr.cfg` text parsing and validation helpers;
  - `scribr/src/services/transfer.h` for future BLE/config sync interface shape.

Validation: `arduino-cli compile --profile esp32s3 scribr` passes.

### 2026-06-15 checkpoint 2

Completed a broad production-skeleton pass and compile-checked:

- added shared I2C seam `scribr/src/hal/i2c_bus.h/.cpp` with a FreeRTOS mutex;
- added SD-MMC service `scribr/src/services/sd_card.h/.cpp` using documented 1-bit pins and `/notes` creation;
- added battery service `scribr/src/services/battery.h/.cpp` with 16-sample ADC averaging, x2 divider, documented voltage curve, 5% snap, and 15/20% low-battery hysteresis;
- added PCF85063 RTC driver `scribr/src/drivers/rtc_pcf85063.h/.cpp` for BCD read/write, OS-bit handling, ISO-8601 parsing, and epoch sanity checks;
- added clock service `scribr/src/services/clock.h/.cpp` for RTC bootstrap from parsed config and UTC/local formatting;
- expanded config service with SD-backed `/scribr.cfg` loading;
- added storage service `scribr/src/services/storage.h/.cpp` for `/notes` scan, note ID derivation, metadata parsing, newest-first sorting, path generation, and wav/meta delete;
- added audio service seam `scribr/src/services/audio_service.h/.cpp`; current implementation is an explicit stub until ES8311/I2S bring-up is performed;
- added sleep service `scribr/src/services/sleep_service.h/.cpp` for light sleep with button wake;
- changed app startup to initialize SD, config, clock, battery, storage, audio, and buttons;
- changed list model to populate from SD scan instead of seeded fake recordings;
- added condition-screen model/rendering for LOW BATTERY, CHARGING, NO SD CARD, STORAGE FULL, TIME NOT SET, and SLEEP;
- wired NO SD blocking for record/list attempts, TIME NOT SET on invalid RTC, LOW BATTERY advisory, and 120 s IDLE -> SLEEP screen -> light sleep.

Validation: `arduino-cli compile --profile esp32s3 scribr` passes.

Known remaining hard blocker: real ES8311/I2S capture/playback is still behind the audio service stub and requires hardware bring-up per ADR 0003.

### 2026-06-15 checkpoint 3

Completed and compile-checked:

- implemented `/scribr.cfg` `rtc_set_utc` mark-applied behavior:
  - after a successful RTC bootstrap, the exact active `rtc_set_utc=...` line is rewritten as `# applied rtc_set_utc=...`;
  - comments and unknown config keys are preserved where practical;
  - rewrite uses `/scribr.cfg.tmp` plus `/scribr.cfg.bak` restore logic if rename fails;
  - failures are logged because stale time would otherwise be reapplied on later boots.
- fixed playback state-machine integration so entering PLAYBACK calls `services::audio::startPlayback(...)`, manual stop calls `stopPlayback()`, and auto-finish stops the audio service before returning to IDLE.
- added storage metadata commit helper and wired the RECORDING save path so, once the audio service returns a committed WAV, the app writes duration metadata and only writes `created_utc` when the clock service reports valid time.

Validation: `arduino-cli compile --profile esp32s3 scribr` passes.

### 2026-06-15 checkpoint 4

Completed and compile-checked an initial real audio implementation behind `services::audio`:

- replaced the explicit audio stub with an ESP-IDF `driver/i2s_std` implementation;
- added manual ES8311 I²C register initialization over the shared I²C seam;
- configured I²S standard mode for 16 kHz, 16-bit, 2-channel codec slots;
- implemented recording task:
  - writes a 44-byte placeholder WAV header;
  - reads 8 KB stereo I²S buffers;
  - stores the left slot only as mono PCM;
  - backfills the WAV header on stop;
  - discards takes below the 1000-byte validity floor;
- implemented playback task:
  - rejects WAVs of 44 bytes or less;
  - skips the WAV header;
  - reads mono PCM in 1024-byte chunks;
  - duplicates mono samples to L/R for codec output;
  - polls the stop flag during playback;
- added PA enable handling on GPIO46 and audio rail enable/disable discipline;
- updated PLAYBACK screen timer logic to return to IDLE when the audio service finishes independently of the UI duration estimate.

Validation: `arduino-cli compile --profile esp32s3 scribr` passes.

Important: this is the first hardware-facing ES8311/I²S implementation and is not yet hardware-validated. The ES8311 register sequence is intentionally isolated in `scribr/src/services/audio_service.cpp` and should be tuned during bring-up if capture/playback is silent, noisy, inverted, or clock-unstable.
