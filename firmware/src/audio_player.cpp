#include <Arduino.h>
#include <LittleFS.h>

#include "Audio.h"  // ESP32-audioI2S (schreibfaul1)
#include "audio_player.h"
#include "config.h"

namespace {
Audio audio;
bool fsOk = false;

enum Mode { OFF, IDLE_LOOP, CELEBRATION };
Mode mode = OFF;
unsigned long lastStartMs = 0;

// Guard so a momentary isRunning()==false right after starting doesn't restart
// the idle file on top of itself.
const unsigned long kIdleRestartGuardMs = 800;

void startFile(const char* path, int volume) {
  audio.setVolume(volume);
  audio.connecttoFS(LittleFS, path);
  lastStartMs = millis();
}
}  // namespace

void audioplayer::begin() {
  fsOk = LittleFS.begin();
  if (!fsOk) {
    Serial.println(F("LittleFS mount FAILED — run `pio run -t uploadfs`."));
  }
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);  // MAX98357A is mono; halves the per-sample work
}

void audioplayer::playIdle() {
  if (!fsOk || !ENABLE_IDLE_AUDIO) {
    mode = OFF;
    return;
  }
  mode = IDLE_LOOP;
  startFile(IDLE_FILE, IDLE_VOLUME);
}

void audioplayer::playCelebration() {
  if (!fsOk) return;
  mode = CELEBRATION;
  startFile(AUDIO_FILE, CELEBRATION_VOLUME);
}

void audioplayer::update() {
  audio.loop();

  // Loop the idle ambience: if it reached the end, play it again.
  if (mode == IDLE_LOOP && !audio.isRunning() &&
      millis() - lastStartMs > kIdleRestartGuardMs) {
    startFile(IDLE_FILE, IDLE_VOLUME);
  }
}

bool audioplayer::isCelebrationPlaying() {
  return mode == CELEBRATION && audio.isRunning();
}
