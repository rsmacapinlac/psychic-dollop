# Recording & Playback Spec

Audio capture and playback requirements for scribr, extracted from the
reference firmware. These are the specs a compatible implementation must honor
when real audio is wired into scribr.

## Hardware constraint — mono only

Capture is **mono** — the board has a single ES8311 codec (one mic, one
speaker), so there is no stereo capture. Don't design for stereo on this board.
The codec hardware details (config, I²S, the multi-mic caveat) are in the board
reference: `../reference/audio-codec-es8311.md`.

## Audio format

| Property        | Value                                  |
| --------------- | -------------------------------------- |
| Sample rate     | 16 kHz                                 |
| Bit depth       | 16-bit signed PCM, little-endian       |
| Channels stored | 1 (mono)                               |
| Container       | WAV (RIFF/WAVE), 44-byte PCM header    |
| Data rate       | ~32 KB/s (~1.9 MB/min)                 |

## Recording

- **Codec capture:** opened as 16 kHz, **2-channel**, 16-bit; input gain 45.0.
- **Down-mix to mono:** keep the **left** channel only (`mbuf[i] = sbuf[i*2]`).
  The right channel is a duplicate of the same mic and is discarded.
- **WAV header:** write 44 zero bytes first, stream PCM data, then `seek(0)` and
  back-fill the header once the length is known. Header fields:
  - `audioFormat = 1` (PCM), `channels = 1`, `sampleRate = 16000`
  - `byteRate = 32000` (sampleRate × blockAlign), `blockAlign = 2`
  - `bitsPerSample = 16`
- **Capture buffer:** 8 KB stereo read buffer, 4 KB mono write buffer
  (`REC_BUF = 8 * 1024`), allocated from heap.
- **File naming:** one directory per recording named with the UTC capture time,
  `/recordings/<YYYYMMDDTHHMMSS>/session.wav` (e.g.
  `/recordings/20260618T091500/session.wav`); duration is stored beside it in
  `session.meta`. See ADR 0004 for the full layout (and the `unset-NNN` fallback
  when the clock is not yet set).
- **Validity floor:** a recording shorter than ~1000 bytes is discarded.

## Playback

- **Read:** skip the 44-byte header, read mono PCM in **1024-byte** chunks.
- **Mono → stereo:** duplicate each mono sample into both L and R for the codec
  (the DAC output path runs 2-channel).
- **Volume:** 85 during playback (codec out vol; 0 when idle).
- **Stop control:** polled mid-stream; a button press ends playback early.
- **Guard:** files of ≤ 44 bytes (header only, no audio) are rejected.

> Open technical decisions from this spec are centralized in
> `open-technical-decisions.md`: the down-mix method (**TD-2**) and the
> 16 kHz / 16-bit quality ceiling (**TD-3**). Mono is a hardware limit, not a
> choice (see "Hardware constraint — mono only" above).

See also `docs/reference/device-rendering-constraints.md` and
`docs/requirements/non-functional.md`.
