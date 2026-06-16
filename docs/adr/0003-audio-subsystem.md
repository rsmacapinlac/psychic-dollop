# 0003 — Audio Subsystem: Raw I²S + Manual ES8311

**Status:** Accepted

## Context

Audio is the highest-risk subsystem and the one with the least existing code:
`../requirements/recording-playback-spec.md` documents the format and capture rules, but notes
that the reference firmware's codec/I²S BSP is **not vendored** into scribr. The
board uses a single **ES8311** mono codec for both mic input and speaker output
(I²C control on the shared bus at 0x18; PA enable on GPIO46; I²S on the pins in
`../reference/hardware-pinout.md`). Three implementation routes exist: lift the reference
BSP, adopt ESP-ADF, or hand-roll it.

## Decision

Drive audio with the **ESP-IDF `driver/i2s` API directly, plus manual ES8311
register init over the existing I²C** — no ESP-ADF, no opaque BSP.

- Bring up I²S in standard mode, 16 kHz, 16-bit, 2-channel slots (the codec link
  is 2-channel even though the mic is mono).
- Manually configure ES8311 registers for capture (input gain 45.0) and playback
  (out vol 85 active / 0 idle), toggling PA on GPIO46 and the audio power rail on
  GPIO42.
- **Capture:** read the stereo buffer, down-mix to mono by keeping the **left**
  slot, stream to a WAV file (44 zero bytes first, backfill the header on stop).
- **Playback:** skip the 44-byte header, read mono in 1024-byte chunks, duplicate
  each sample to L+R for the 2-channel DAC path. Poll a stop flag mid-stream.
- Encapsulate all of this behind a small `audio` service (`begin/recordTo/play/
  stop`) so the state machine never touches I²S/registers directly.

## Consequences

- Maximum control and the most learning; no framework lock-in or BSP black box.
- **Most code to get right** — ES8311 register sequencing and I²S clocking are
  the likeliest source of bring-up iteration. Budget hardware debugging here.
- The codec lives on the shared I²C bus (ADR 0002) alongside the RTC and SHTC3;
  init order and bus arbitration must be handled.
- Format choices (16 kHz/16-bit/mono) are fixed for v1 per the spec; raising them
  later is a localized change in the `audio` service.

## Alternatives

- **Port the reference BSP** — fastest to known-good, but the BSP isn't vendored,
  hides the learning, and drags in its abstractions.
- **ESP-ADF** — batteries-included pipelines, but heavy, a different build setup
  than the current sketch flow, and overkill for one mono codec.
