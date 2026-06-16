# ES8311 Audio Codec — Hardware Reference

Hardware facts about the board's audio codec, for the audio implementation. For
capture/playback *requirements* (format, down-mix, WAV handling) see
`../requirements/recording-playback-spec.md`; for the driver decision see
`../adr/0003-audio-subsystem.md`.

The board — **Waveshare ESP32-S3 1.54" e-Paper** (codec board id
`S3_ePaper_1_54`) — uses a **single ES8311 codec for both input and output**:

```
in_out: {codec: ES8311, pa: 46, use_mclk: 1, pa_gain:6}
```

The ES8311 is a **mono** codec: one ADC (one microphone), one DAC. There is no
second microphone and no stereo capture. The I²S link is opened as 2-channel,
but both slots carry the same single mic signal, so capture is effectively mono.

True multi-mic capture would require different hardware (e.g. the ES7210
4-channel mic-array ADC, present in the reference codebase only for a different
board).

I²C control address **0x18**, PA enable on **GPIO46**, and the I²S/MCLK pins are
in `hardware-pinout.md`.
