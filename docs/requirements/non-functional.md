# Non-Functional Requirements

Cross-cutting quality requirements for scribr. The subsystem specs
(`recording-playback-spec.md`, `time-power-spec.md`) own the concrete numbers;
this document states the goals they serve and the constraints that span
subsystems. Previously referenced from `recording-playback-spec.md` as "the
broader NFR notes."

## Power & battery (core goal)

- **Battery life is a primary product goal.** The device must sleep aggressively
  when idle: light-sleep after 120 s of inactivity in v1.
- Active states (RECORDING, PLAYBACK, browsing) may run subsystems at full power;
  IDLE must shed everything it can while keeping the VBAT latch held and the
  e-paper image.
- Low-battery handling: warn at ≤15%, recover at ≥20% (hysteresis), re-checked
  every 30 s. The warning must reach the user visually (see `ui-screens.md`).
- The device is powered only while software holds the VBAT latch; the firmware
  must never drop the latch unintentionally.

## Responsiveness

- Button presses must feel immediate even though a panel refresh busy-waits
  ~300–500 ms. Input servicing must not be blocked by display refresh — the
  prototype already offloads refresh to the second core
  (`../adr/0001-display-and-rendering-architecture.md`,
  `../adr/0002-concurrency-model.md`).
- Live timers (REC/PLAY) update once per second via partial refresh; this must
  not glitch or stall audio.

## Display fidelity

- All UI must render within the 1-bit GFX constraints in
  `../reference/device-rendering-constraints.md`: discrete font sizes, ASCII only, no
  letter-spacing, FreeSans proportional widths, 192 px content width.
- No animation, motion, or grayscale. Full refresh only on screen change;
  partial refresh for isolated live fields, with a periodic cleanup full-refresh
  to clear accumulated ghosting.

## Audio quality & integrity

- Voice-grade capture: 16 kHz, 16-bit mono PCM in WAV (`recording-playback-spec.md`).
  Adequate for speech and future transcription; not music-grade by design.
- A recording must be written durably: a power loss or cancel must not leave a
  corrupt note that breaks the list. Sub-~1000-byte captures are discarded.

## Storage & scale

- The recordings list is built by **scanning the SD card**, not from a fixed RAM
  array — it must handle hundreds of notes, paging the 3-row window
  (`../adr/0004-storage-and-metadata-model.md`).
- Each note carries sidecar metadata (creation time in UTC, duration). Storage is
  the source of truth; RAM holds only what the current screen needs.

## Timekeeping

- Canonical time is **UTC**, stored per note. Display converts to local via a
  configurable offset (`../adr/0005-time-model.md`). No timezone database / DST in
  v1; the offset is set manually.
- If the RTC reports an invalid/stopped clock, the firmware must degrade
  gracefully (still record; surface a "time not set" state) rather than write
  bogus timestamps silently.

## Maintainability & portability

- The firmware is layered (`hal/` → `drivers/` → `app/`) so subsystems can be
  brought up and tested independently. The current `scribr/` tree is a UX
  prototype and is fully reworkable; only the display/rendering layer is worth
  preserving as-is.
- Hardware access goes through HAL/driver seams so the board pin map and codec
  details stay in one place (`../reference/hardware-pinout.md`).

## Forward compatibility

- A **transfer/sync seam** must exist in v1 even though no sync ships, so a future
  **BLE** companion-app sync drops in without reworking storage or the state
  machine (`../adr/0007-connectivity-and-sync-seam.md`).
- The time model must accommodate a later WiFi/NTP sync without changing the
  stored format.

See also `../adr/README.md` for the decisions that satisfy these requirements.
