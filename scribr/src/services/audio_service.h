#pragma once

#include <stdint.h>

namespace services::audio {

enum class State { Idle, Recording, Playing };

void begin();
State state();

// Production seam. Full ES8311/I2S implementation will live behind this API.
bool startRecording(const char* wavPath);
bool stopRecordingAndSave(uint32_t& durationSec, uint32_t& bytesWritten);
void cancelRecording();

bool startPlayback(const char* wavPath);
void stopPlayback();
uint32_t playbackElapsedSec();

}  // namespace services::audio
