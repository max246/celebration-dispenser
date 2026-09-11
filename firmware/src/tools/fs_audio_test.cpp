// ===========================================================================
//  LittleFS audio test — Celebration Dispenser
//
//  Plays an MP3 straight from on-board flash (LittleFS) to the MAX98357A — the
//  same path the real firmware uses, with NO WiFi, motor, or button. This is
//  the test for the "play from flash" fix to the streaming crackle.
//
//      # put celebrate.mp3 / idle.mp3 in firmware/data/ first
//      pio run -e fstest -t uploadfs      # upload the audio files
//      pio run -e fstest -t upload        # flash this test
//      pio device monitor -e fstest
//
//  It lists the files it found, then loops TEST_FILE. Should be clean audio.
//  I2S pins MUST match config.h.
// ===========================================================================
#include <Arduino.h>
#include <LittleFS.h>

#include "Audio.h"  // ESP32-audioI2S (schreibfaul1)

static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const int   TEST_VOLUME = 8;   // 0..21
static const char* TEST_FILE   = "/idle.mp3";   // loop this one

Audio audio;
bool fsOk = false;
unsigned long lastStart = 0;

static void startPlay() {
  audio.setVolume(TEST_VOLUME);
  audio.connecttoFS(LittleFS, TEST_FILE);
  lastStart = millis();
}

void setup() {
  Serial.begin(115200);
  delay(5000);  // time to attach the serial monitor
  Serial.println(F("\n=== LittleFS audio test (no WiFi / motor) ==="));

  fsOk = LittleFS.begin();
  if (!fsOk) {
    Serial.println(F("LittleFS mount FAILED — run `pio run -e fstest -t uploadfs`."));
    return;
  }
  Serial.println(F("Files on flash:"));
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile())
    Serial.printf("  %s  (%u bytes)\n", f.name(), (unsigned)f.size());

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);
  Serial.printf("Looping %s ...\n", TEST_FILE);
  startPlay();
}

void loop() {
  audio.loop();
  if (fsOk && !audio.isRunning() && millis() - lastStart > 800) startPlay();
}

void audio_info(const char* info)    { Serial.print(F("[info] ")); Serial.println(info); }
void audio_eof_mp3(const char* info) { Serial.print(F("[eof ] ")); Serial.println(info); }
