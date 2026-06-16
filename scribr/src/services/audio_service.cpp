#include "audio_service.h"

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <driver/gpio.h>
#include <driver/i2s_std.h>
#include <esp_err.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../hal/i2c_bus.h"
#include "../hal/pins.h"
#include "../hal/power.h"
#include "sd_card.h"

namespace services::audio {
namespace {
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint16_t BITS_PER_SAMPLE = 16;
constexpr uint16_t STORED_CHANNELS = 1;
constexpr uint16_t I2S_CHANNELS = 2;
constexpr uint32_t BYTE_RATE = SAMPLE_RATE * STORED_CHANNELS * (BITS_PER_SAMPLE / 8);
constexpr uint16_t BLOCK_ALIGN = STORED_CHANNELS * (BITS_PER_SAMPLE / 8);
constexpr size_t REC_STEREO_BYTES = 8 * 1024;
constexpr size_t PLAY_MONO_BYTES = 1024;
constexpr uint32_t MIN_VALID_DATA_BYTES = 1000;
constexpr uint8_t ES8311_ADDR = 0x18;

State current = State::Idle;
i2s_chan_handle_t txChan = nullptr;
i2s_chan_handle_t rxChan = nullptr;
TaskHandle_t workerTask = nullptr;
volatile bool stopRequested = false;
volatile bool workerDone = true;
bool i2sReady = false;
char activePath[64] = {0};
uint32_t recordDataBytes = 0;
uint32_t startedMs = 0;

int16_t applyGainSat(int16_t sample, int gain) {
  int32_t v = (int32_t)sample * gain;
  if (v > 32767) return 32767;
  if (v < -32768) return -32768;
  return (int16_t)v;
}

void logAudio(const char* fmt, ...) {
  // Keep diagnostics off the SD card in normal firmware; SD appends during
  // audio bring-up made button handling feel sluggish. Serial-only logs are
  // retained for rare hardware failures.
  char line[192];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  Serial.println(line);
}

void put16(uint8_t* p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

void put32(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

void makeWavHeader(uint8_t out[44], uint32_t dataBytes) {
  memset(out, 0, 44);
  memcpy(out + 0, "RIFF", 4);
  put32(out + 4, 36 + dataBytes);
  memcpy(out + 8, "WAVE", 4);
  memcpy(out + 12, "fmt ", 4);
  put32(out + 16, 16);
  put16(out + 20, 1);  // PCM
  put16(out + 22, STORED_CHANNELS);
  put32(out + 24, SAMPLE_RATE);
  put32(out + 28, BYTE_RATE);
  put16(out + 32, BLOCK_ALIGN);
  put16(out + 34, BITS_PER_SAMPLE);
  memcpy(out + 36, "data", 4);
  put32(out + 40, dataBytes);
}

bool esWrite(uint8_t reg, uint8_t value) {
  hal::i2c::Lock lock(pdMS_TO_TICKS(200));
  if (!lock.acquired()) return false;
  TwoWire& w = hal::i2c::bus();
  w.beginTransmission(ES8311_ADDR);
  w.write(reg);
  w.write(value);
  return w.endTransmission() == 0;
}

bool esRead(uint8_t reg, uint8_t& value) {
  hal::i2c::Lock lock(pdMS_TO_TICKS(200));
  if (!lock.acquired()) return false;
  TwoWire& w = hal::i2c::bus();
  w.beginTransmission(ES8311_ADDR);
  w.write(reg);
  if (w.endTransmission(false) != 0) return false;
  if (w.requestFrom((int)ES8311_ADDR, 1) != 1) return false;
  value = w.read();
  return true;
}

void dumpCodecRegs(const char* tag) {
  logAudio("scribr: ES8311 dump %s", tag);
  for (uint8_t base = 0; base < 0x40; base += 8) {
    char line[128];
    int n = snprintf(line, sizeof(line), "  %02x:", base);
    for (uint8_t i = 0; i < 8; ++i) {
      uint8_t v = 0;
      if (esRead(base + i, v)) n += snprintf(line + n, sizeof(line) - n, " %02x", v);
      else n += snprintf(line + n, sizeof(line) - n, " ??");
    }
    logAudio("%s", line);
  }
}

bool esWriteChecked(uint8_t reg, uint8_t value) {
  const bool ok = esWrite(reg, value);
  if (!ok) {
    logAudio("scribr: ES8311 write failed reg=0x%02x", reg);
  }
  delay(1);
  return ok;
}

bool initCodec(bool playback) {
  // Closely follow the known-good Pala Note esp_codec_dev ES8311 driver:
  // es8311_open() + set_fs(16 kHz/16-bit/I2S) + es8311_start().
  bool ok = true;

  // es8311_open()
  ok &= esWriteChecked(0x44, 0x08);  // I2C noise immunity, written twice in reference
  ok &= esWriteChecked(0x44, 0x08);
  ok &= esWriteChecked(0x01, 0x30);
  ok &= esWriteChecked(0x02, 0x00);
  ok &= esWriteChecked(0x03, 0x10);
  ok &= esWriteChecked(0x16, 0x07);  // mic PGA gain = ES8311_MIC_GAIN_42DB (enum 7),
                                     // matching the reference set_in_gain(45.0). 0x24 is
                                     // the es8311_open() reset default (~24 dB) that the
                                     // reference overwrites; using it left capture too soft.
  ok &= esWriteChecked(0x04, 0x10);
  ok &= esWriteChecked(0x05, 0x00);
  ok &= esWriteChecked(0x0B, 0x00);
  ok &= esWriteChecked(0x0C, 0x00);
  ok &= esWriteChecked(0x10, 0x1F);
  ok &= esWriteChecked(0x11, 0x7F);
  ok &= esWriteChecked(0x00, 0x80);  // release reset, slave mode
  ok &= esWriteChecked(0x01, 0x3F);  // use external MCLK, clocks enabled
  ok &= esWriteChecked(0x06, 0x00);  // no SCLK invert
  ok &= esWriteChecked(0x13, 0x10);
  ok &= esWriteChecked(0x1B, 0x0A);
  ok &= esWriteChecked(0x1C, 0x6A);
  ok &= esWriteChecked(0x44, 0x58);  // internal reference signal, no_dac_ref=false

  // set_fs(): normal I2S format + 16-bit sample words. The ESP I2S peripheral
  // still uses 32-bit slots per the reference firmware.
  ok &= esWriteChecked(0x09, 0x0C);  // DAC serial digital port: I2S, 16-bit
  ok &= esWriteChecked(0x0A, 0x0C);  // ADC serial digital port: I2S, 16-bit

  // es8311_start() for BOTH ADC + DAC enabled.
  ok &= esWriteChecked(0x00, 0x80);
  ok &= esWriteChecked(0x01, 0x3F);
  ok &= esWriteChecked(0x09, 0x0C);  // clear bit6 = enable DAC serial path
  ok &= esWriteChecked(0x0A, 0x0C);  // clear bit6 = enable ADC serial path
  ok &= esWriteChecked(0x17, 0xBF);  // ADC volume
  ok &= esWriteChecked(0x0E, 0x02);
  ok &= esWriteChecked(0x12, 0x00);  // enable DAC
  ok &= esWriteChecked(0x14, 0x1A);  // analog PGA path, DMIC disabled
  ok &= esWriteChecked(0x0D, 0x01);
  ok &= esWriteChecked(0x15, 0x40);
  ok &= esWriteChecked(0x37, 0x08);
  ok &= esWriteChecked(0x45, 0x00);

  // Vol/mute state.
  ok &= esWriteChecked(0x31, playback ? 0x00 : 0x00);
  ok &= esWriteChecked(0x32, playback ? 0xBF : 0x00);

  gpio_set_level((gpio_num_t)AUDIO_PA_PIN, playback ? 1 : 0);
  delay(80);
  return ok;
}

bool ensureI2s() {
  if (i2sReady) return true;

  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chanCfg.dma_desc_num = 8;
  chanCfg.dma_frame_num = 256;
  esp_err_t err = i2s_new_channel(&chanCfg, &txChan, &rxChan);
  if (err != ESP_OK) {
    logAudio("scribr: i2s_new_channel failed: %s", esp_err_to_name(err));
    return false;
  }
  logAudio("scribr: i2s channels allocated");

  i2s_std_clk_config_t clkCfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE);
  clkCfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  i2s_std_config_t stdCfg = {
      .clk_cfg = clkCfg,
      // Standard (Philips) I2S framing with 32-bit stereo slots. The ES8311 SDP
      // registers (0x09/0x0A) select I2S/Philips format, and the reference
      // firmware's esp_codec_dev reconfigures the link to Philips on open via
      // set_drv_fs(). MSB framing here skews data by one BCLK vs the codec,
      // corrupting captured samples (audible as noise on a correct decoder).
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = (gpio_num_t)I2S_MCLK_PIN,
          .bclk = (gpio_num_t)I2S_BCLK_PIN,
          .ws = (gpio_num_t)I2S_WS_PIN,
          .dout = (gpio_num_t)I2S_DOUT_PIN,
          .din = (gpio_num_t)I2S_DIN_PIN,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };

  err = i2s_channel_init_std_mode(txChan, &stdCfg);
  if (err != ESP_OK) {
    logAudio("scribr: i2s tx init failed: %s", esp_err_to_name(err));
    return false;
  }
  err = i2s_channel_init_std_mode(rxChan, &stdCfg);
  if (err != ESP_OK) {
    logAudio("scribr: i2s rx init failed: %s", esp_err_to_name(err));
    return false;
  }

  i2sReady = true;
  logAudio("scribr: i2s ready");
  return true;
}

void finishWorker() {
  workerTask = nullptr;
  workerDone = true;
  vTaskDelete(nullptr);
}

void recordTask(void*) {
  workerDone = false;
  recordDataBytes = 0;

  File wav = sdcard::fs().open(activePath, FILE_WRITE);
  if (!wav) {
    logAudio("scribr: failed to open WAV for recording path=%s", activePath);
    sdcard::invalidate();  // likely a removed/stale card — re-probe next attempt
    current = State::Idle;
    finishWorker();
  }

  uint8_t header[44] = {0};
  wav.write(header, sizeof(header));

  uint8_t* stereo = (uint8_t*)malloc(REC_STEREO_BYTES);
  int16_t* mono = (int16_t*)malloc(REC_STEREO_BYTES / (I2S_CHANNELS * sizeof(int32_t)) * sizeof(int16_t));
  if (!stereo || !mono) {
    logAudio("scribr: audio record buffer allocation failed");
    free(stereo);
    free(mono);
    wav.close();
    sdcard::fs().remove(activePath);
    current = State::Idle;
    finishWorker();
  }

  while (!stopRequested) {
    size_t bytesRead = 0;
    esp_err_t err = i2s_channel_read(rxChan, stereo, REC_STEREO_BYTES, &bytesRead, pdMS_TO_TICKS(250));
    if (err != ESP_OK || bytesRead == 0) {
      static uint32_t readFailCount = 0;
      if (++readFailCount <= 10) logAudio("scribr: i2s read err=%s bytes=%u", esp_err_to_name(err), (unsigned)bytesRead);
      continue;
    }

    // I2S is configured like the reference firmware: 32-bit MSB slots carrying
    // 16-bit audio. Convert left 32-bit slot to stored 16-bit mono.
    const size_t frames = bytesRead / (sizeof(int32_t) * I2S_CHANNELS);
    const int32_t* samples = (const int32_t*)stereo;
    for (size_t i = 0; i < frames; ++i) {
      mono[i] = (int16_t)(samples[i * 2] >> 16);  // left slot MSB position of 32-bit slot
    }
    const size_t monoBytes = frames * sizeof(int16_t);
    if (wav.write((const uint8_t*)mono, monoBytes) != monoBytes) {
      logAudio("scribr: WAV write failed; stopping recording");
      break;
    }
    recordDataBytes += monoBytes;
  }
  i2s_channel_disable(rxChan);
  i2s_channel_disable(txChan);

  if (recordDataBytes >= MIN_VALID_DATA_BYTES) {
    makeWavHeader(header, recordDataBytes);
    wav.seek(0);
    wav.write(header, sizeof(header));
    wav.flush();
    logAudio("scribr: recording committed %lu bytes", (unsigned long)recordDataBytes);
  }
  wav.close();

  if (recordDataBytes < MIN_VALID_DATA_BYTES) {
    sdcard::fs().remove(activePath);
    logAudio("scribr: recording discarded (too short or no captured bytes)");
  }

  free(stereo);
  free(mono);
  current = State::Idle;
  hal::power::disableAudioRail();
  finishWorker();
}

void playbackTask(void*) {
  workerDone = false;

  File wav = sdcard::fs().open(activePath, FILE_READ);
  if (!wav || wav.size() <= 44) {
    logAudio("scribr: invalid WAV for playback path=%s", activePath);
    if (wav) wav.close();
    current = State::Idle;
    finishWorker();
  }
  wav.seek(44);

  uint8_t* monoBytes = (uint8_t*)malloc(PLAY_MONO_BYTES);
  int32_t* stereo = (int32_t*)malloc((PLAY_MONO_BYTES / sizeof(int16_t)) * I2S_CHANNELS * sizeof(int32_t));
  if (!monoBytes || !stereo) {
    logAudio("scribr: audio playback buffer allocation failed");
    free(monoBytes);
    free(stereo);
    wav.close();
    current = State::Idle;
    finishWorker();
  }

  while (!stopRequested && wav.available()) {
    const size_t n = wav.read(monoBytes, PLAY_MONO_BYTES);
    if (n == 0) break;
    const size_t samples = n / sizeof(int16_t);
    const int16_t* mono = (const int16_t*)monoBytes;
    for (size_t i = 0; i < samples; ++i) {
      const int32_t s = ((int32_t)mono[i]) << 16;  // 16-bit audio in MSB position of 32-bit slot
      stereo[i * 2] = s;
      stereo[i * 2 + 1] = s;
    }
    const size_t outBytes = samples * I2S_CHANNELS * sizeof(int32_t);
    size_t written = 0;
    esp_err_t err = i2s_channel_write(txChan, stereo, outBytes, &written, pdMS_TO_TICKS(500));
    static uint32_t playWriteCount = 0;
    if (++playWriteCount <= 10) logAudio("scribr: playback write err=%s requested=%u written=%u", esp_err_to_name(err), (unsigned)outBytes, (unsigned)written);
  }
  i2s_channel_disable(txChan);
  i2s_channel_disable(rxChan);

  wav.close();
  free(monoBytes);
  free(stereo);
  gpio_set_level((gpio_num_t)AUDIO_PA_PIN, 0);
  esWrite(0x32, 0x00);
  hal::power::disableAudioRail();
  current = State::Idle;
  logAudio("scribr: playback finished");
  finishWorker();
}

bool startWorker(TaskFunction_t fn, const char* name) {
  workerDone = false;
  BaseType_t ok = xTaskCreatePinnedToCore(fn, name, 8192, nullptr, 5, &workerTask, 1);
  if (ok != pdPASS) {
    workerTask = nullptr;
    workerDone = true;
    return false;
  }
  return true;
}

void waitWorker(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (!workerDone && millis() - start < timeoutMs) delay(10);
}
}  // namespace

void begin() {
  hal::power::begin();
  hal::i2c::begin();
  gpio_config_t pa = {};
  pa.intr_type = GPIO_INTR_DISABLE;
  pa.mode = GPIO_MODE_OUTPUT;
  pa.pin_bit_mask = (1ULL << AUDIO_PA_PIN);
  pa.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pa.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&pa);
  gpio_set_level((gpio_num_t)AUDIO_PA_PIN, 0);
  ensureI2s();
  hal::power::disableAudioRail();
  current = State::Idle;
}

State state() { return current; }

bool startRecording(const char* wavPath) {
  if (current != State::Idle || !wavPath || !sdcard::mounted()) return false;
  if (!ensureI2s()) return false;

  // The card may have been swapped since boot; cardType() is cached and won't
  // reveal it, leaving a stale mount that fails every write. Probe with a real
  // round-trip and remount once so a reinserted card records without a reset.
  if (!sdcard::probeWritable()) {
    logAudio("scribr: SD write probe failed; remounting");
    sdcard::invalidate();
    if (!sdcard::ensureMounted() || !sdcard::probeWritable()) return false;
  }

  strncpy(activePath, wavPath, sizeof(activePath) - 1);
  activePath[sizeof(activePath) - 1] = 0;
  sdcard::fs().remove(activePath);

  hal::power::enableAudioRail();
  delay(30);
  esp_err_t txErr = i2s_channel_enable(txChan);
  logAudio("scribr: i2s tx enable for record clocks: %s", esp_err_to_name(txErr));
  if (txErr != ESP_OK) {
    hal::power::disableAudioRail();
    return false;
  }
  esp_err_t enErr = i2s_channel_enable(rxChan);
  logAudio("scribr: i2s rx enable before codec init: %s", esp_err_to_name(enErr));
  if (enErr != ESP_OK) {
    i2s_channel_disable(txChan);
    hal::power::disableAudioRail();
    return false;
  }
  delay(20);
  if (!initCodec(false)) {
    logAudio("scribr: ES8311 record init failed");
    i2s_channel_disable(rxChan);
    i2s_channel_disable(txChan);
    hal::power::disableAudioRail();
    return false;
  }
  logAudio("scribr: ES8311 record init ok");

  stopRequested = false;
  recordDataBytes = 0;
  startedMs = millis();
  current = State::Recording;
  if (!startWorker(recordTask, "scribr_rec")) {
    current = State::Idle;
    i2s_channel_disable(rxChan);
    i2s_channel_disable(txChan);
    hal::power::disableAudioRail();
    return false;
  }
  logAudio("scribr: audio recording started path=%s", activePath);
  return true;
}

bool stopRecordingAndSave(uint32_t& durationSec, uint32_t& bytesWritten) {
  if (current != State::Recording && workerDone) return false;
  stopRequested = true;
  waitWorker(5000);
  durationSec = recordDataBytes / BYTE_RATE;
  bytesWritten = recordDataBytes;
  const bool committed = recordDataBytes >= MIN_VALID_DATA_BYTES;
  current = State::Idle;
  return committed;
}

void cancelRecording() {
  if (current == State::Recording || !workerDone) {
    stopRequested = true;
    waitWorker(5000);
  }
  if (activePath[0]) sdcard::fs().remove(activePath);
  current = State::Idle;
  recordDataBytes = 0;
  hal::power::disableAudioRail();
  logAudio("scribr: audio recording canceled");
}

bool startPlayback(const char* wavPath) {
  if (current != State::Idle || !wavPath || !sdcard::mounted()) return false;
  File f = sdcard::fs().open(wavPath, FILE_READ);
  if (!f || f.size() <= 44) {
    if (f) f.close();
    return false;
  }
  f.close();
  if (!ensureI2s()) return false;

  strncpy(activePath, wavPath, sizeof(activePath) - 1);
  activePath[sizeof(activePath) - 1] = 0;
  hal::power::enableAudioRail();
  delay(30);
  esp_err_t rxErr = i2s_channel_enable(rxChan);
  logAudio("scribr: i2s rx enable for playback clocks: %s", esp_err_to_name(rxErr));
  if (rxErr != ESP_OK) {
    hal::power::disableAudioRail();
    return false;
  }
  esp_err_t enErr = i2s_channel_enable(txChan);
  logAudio("scribr: i2s tx enable before codec init: %s", esp_err_to_name(enErr));
  if (enErr != ESP_OK) {
    i2s_channel_disable(rxChan);
    hal::power::disableAudioRail();
    return false;
  }
  delay(20);
  if (!initCodec(true)) {
    logAudio("scribr: ES8311 playback init failed");
    i2s_channel_disable(txChan);
    i2s_channel_disable(rxChan);
    hal::power::disableAudioRail();
    return false;
  }
  logAudio("scribr: ES8311 playback init ok");

  stopRequested = false;
  startedMs = millis();
  current = State::Playing;
  if (!startWorker(playbackTask, "scribr_play")) {
    current = State::Idle;
    i2s_channel_disable(txChan);
    i2s_channel_disable(rxChan);
    hal::power::disableAudioRail();
    return false;
  }
  logAudio("scribr: audio playback started path=%s", activePath);
  return true;
}

void stopPlayback() {
  if (current == State::Playing || !workerDone) {
    stopRequested = true;
    waitWorker(3000);
  }
  gpio_set_level((gpio_num_t)AUDIO_PA_PIN, 0);
  current = State::Idle;
}

uint32_t playbackElapsedSec() {
  if (current != State::Playing) return 0;
  return (millis() - startedMs) / 1000;
}

}  // namespace services::audio
