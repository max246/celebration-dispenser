// ===========================================================================
//  LittleFS audio test — Celebration Dispenser
//
//  Plays an MP3 straight from on-board flash (LittleFS) to the MAX98357A — the
//  same path the real firmware uses, with NO WiFi, motor, or button.
//
//      # put celebrate.mp3 / idle.mp3 in firmware/data/ first
//      pio run -e fstest -t upload        # flash this test + partition table
//      pio run -e fstest -t uploadfs      # upload the audio files
//      pio device monitor -e fstest
//
//  Plays TEST_FILE ONCE and prints a per-second progress line (running + time),
//  so you can see whether decoding actually advances. I2S pins match config.h.
// ===========================================================================
#include <Arduino.h>
#include <LittleFS.h>

#include "Audio.h"  // ESP32-audioI2S (schreibfaul1)

static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const int   TEST_VOLUME = 10;  // 0..21
static const char* TEST_FILE   = "/idle.mp3";

Audio audio;

void setup() {
  Serial.begin(115200);
  // Wait for the USB serial monitor to attach (or 8s) so boot prints aren't missed.
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== LittleFS audio test (no WiFi / motor) ==="));

  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount FAILED — run `pio run -e fstest -t uploadfs`."));
    return;
  }
  Serial.println(F("Files on flash:"));
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile())
    Serial.printf("  %s  (%u bytes)\n", f.name(), (unsigned)f.size());

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);
  audio.setVolume(TEST_VOLUME);
  Serial.printf("Playing %s once...\n", TEST_FILE);
  audio.connecttoFS(LittleFS, TEST_FILE);
}

void loop() {
  audio.loop();

  static unsigned long t = 0;
  if (millis() - t >= 1000) {
    t = millis();
    Serial.printf("[stat] running=%d  time=%u/%u s\n",
                  audio.isRunning(), audio.getAudioCurrentTime(),
                  audio.getAudioFileDuration());
  }
}

void audio_info(const char* info)    { Serial.print(F("[info] ")); Serial.println(info); }
void audio_eof_mp3(const char* info) { Serial.print(F("[eof ] ")); Serial.println(info); }
