# 0002 — Concurrency & Core Allocation

**Status:** Accepted

## Context

The ESP32-S3 is dual-core at 240 MHz. Three things contend for time: button
servicing (must feel instant), the ~300–500 ms blocking panel refresh, and —
once real audio lands — continuous I²S DMA for capture/playback plus SD writes.
A naive single loop would stall buttons during refresh and risk audio underruns.

## Decision

- **Core 1 (Arduino `loop`)**: the app — button polling, the state machine,
  timers, and composing the canvas. Stays responsive; never blocks on refresh.
- **Core 0**: the blocking e-paper refresh, on a dedicated FreeRTOS task
  signalled by notification (already in the prototype), plus the audio I²S
  pipeline when active.
- The display service exposes a non-blocking `busy()`; the app only composes a
  new frame when no refresh is in flight, and keeps servicing buttons meanwhile.
- **Audio is the real-time citizen.** When recording or playing, the I²S DMA
  loop on core 0 takes priority; the per-second timer's *partial* refresh must be
  cheap and must never block audio. If a refresh would risk an underrun, skip/defer
  that tick rather than glitch audio.
- **Shared I²C** (ES8311, PCF85063 RTC, SHTC3) is accessed under a single
  mutex/owner; RTC/sensor reads happen between audio operations, not during DMA
  setup.

## Consequences

- Clear ownership: input + logic on one core, blocking/real-time work on the
  other.
- Display and audio both want core 0 — audio wins while active; this bounds how
  often live UI can repaint during REC/PLAY (acceptable: timers are 1 Hz).
- Canvas is single-writer (core 1) and read by the blit just before hand-off, so
  no canvas locking is needed; only the panel framebuffer crosses cores, guarded
  by the `busy()` handshake.

## Alternatives

- **Single super-loop** — rejected: refresh stalls buttons; audio underruns.
- **Refresh on core 1, audio on core 0** — viable, but co-locating both blocking
  jobs (refresh + audio) on core 0 keeps the logic core jitter-free.
