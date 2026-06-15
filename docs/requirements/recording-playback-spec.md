# Recording & Playback Spec

Audio capture and playback requirements for scribr, extracted from the
reference firmware. These are the specs a compatible implementation must honor
when real audio is wired into scribr.

## Hardware constraint — mono only

The board — a **Waveshare ESP32-S3 1.54" e-Paper** (codec board id
`S3_ePaper_1_54`) — uses a **single ES8311 codec for both input and output**:

```
in_out: {codec: ES8311, pa: 46, use_mclk: 1, pa_gain:6}
```

The ES8311 is a **mono** codec: one ADC (one microphone), one DAC. There is no
second microphone and no stereo capture. The I²S link is opened as 2-channel,
but both slots carry the same single mic signal, so capture is effectively mono.

True multi-mic capture would require different hardware (e.g. the ES7210
4-channel mic-array ADC, present in the codebase only for a different board).

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
- **File naming:** `/notes/note_%03d.wav` (zero-padded, e.g. `note_001.wav`).
- **Validity floor:** a recording shorter than ~1000 bytes is discarded.

## Playback

- **Read:** skip the 44-byte header, read mono PCM in **1024-byte** chunks.
- **Mono → stereo:** duplicate each mono sample into both L and R for the codec
  (the DAC output path runs 2-channel).
- **Volume:** 85 during playback (codec out vol; 0 when idle).
- **Stop control:** polled mid-stream; a button press ends playback early.
- **Guard:** files of ≤ 44 bytes (header only, no audio) are rejected.

## Decisions to revisit for scribr

- **Mono is a hardware limit**, not a choice — don't design for stereo capture on
  this board.
- **Left-channel-only down-mix** vs. averaging L+R (moot while mono, but relevant
  if multi-mic hardware is ever used).
- **Fixed 16 kHz / 16-bit** — adequate for voice/speech-to-text; raise only if a
  use case needs it (cost: storage + bandwidth).

See also `docs/device-rendering-constraints.md` and the broader NFR notes.
