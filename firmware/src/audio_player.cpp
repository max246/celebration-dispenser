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
}  // namespace

// Set by the library's end-of-file callback (global weak function). Using the
// real eof event to loop is robust — polling isRunning() can misfire during a
// file's startup phase and restart it before it ever plays.
static volatile bool s_audioEof = false;

namespace {
void startFile(const char* path, int volume) {
  s_audioEof = false;
  audio.setVolume(volume);
  audio.connecttoFS(LittleFS, path);
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

  // Loop the idle ambience: when it reaches end-of-file, start it again.
  if (mode == IDLE_LOOP && s_audioEof) {
    startFile(IDLE_FILE, IDLE_VOLUME);
  }
}

bool audioplayer::isCelebrationPlaying() {
  return mode == CELEBRATION && audio.isRunning();
}

// Called by ESP32-audioI2S when a file finishes.
void audio_eof_mp3(const char* /*info*/) { s_audioEof = true; }
