// ===========================================================================
//  Audio streaming test — Celebration Dispenser
//
//  Standalone MAX98357A + WiFi bring-up: connects to WiFi and streams a free
//  sample MP3 from the web to the I2S amp. Build/flash with:
//
//      pio run -e audiotest -t upload
//      pio device monitor -e audiotest
//
//  Needs your WiFi in firmware/include/secrets.h (copy secrets.h.example).
//  The stream URL below is a public sample; swap it for any of the alternates.
//
//  I2S pins MUST match firmware/include/config.h.
// ===========================================================================
#include <Arduino.h>
#include <WiFi.h>

#include "Audio.h"     // ESP32-audioI2S (schreibfaul1)
#include "secrets.h"   // WIFI_SSID / WIFI_PASS

// ---- I2S pins (match config.h) ----
static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const int TEST_VOLUME = 6;     // 0..21  (low, to rule out clipping/power)

// Default = a plain-HTTP 128k stream. IMPORTANT: on the single-core S2, HTTPS/TLS
// is throughput- and heap-limited (~37 KB/s, ~18 KB heap free) — barely above the
// playback rate, which is what causes the crackle. Plain HTTP streams at full
// speed and plays clean, so host your real sounds over HTTP too (ideally on your
// own LAN).
static const char* TEST_URL = "http://ice1.somafm.com/groovesalad-128-mp3";

// Other plain-HTTP streams to try:
//   "http://icecast.radiofrance.fr/fip-midfi.mp3"
//   "http://mp3.ffh.de/radioffh/hqlivestream.mp3"
// A finite HTTPS file (plays, but crackles on the S2 due to TLS throughput):
//   "https://www.soundhelix.com/examples/mp3/SoundHelix-Song-1.mp3"

Audio audio;

void setup() {
  Serial.begin(115200);
  // Wait for the USB serial monitor to attach (or 8s) so boot prints aren't missed.
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== Celebration Dispenser — audio streaming test ==="));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("WiFi connecting"));
  const unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi FAILED — check WIFI_SSID/WIFI_PASS in secrets.h."));
    return;
  }
  Serial.print(F("WiFi OK: "));
  Serial.println(WiFi.localIP());

  // Disable WiFi modem power-save. On the single-core S2 its micro-sleeps stall
  // the stream and cause crackle/stutter — this is the big fix for that.
  WiFi.setSleep(false);

  Serial.printf("PSRAM: %u bytes | free heap: %u | RSSI: %d dBm\n",
                ESP.getPsramSize(), ESP.getFreeHeap(), WiFi.RSSI());
  if (ESP.getPsramSize() == 0)
    Serial.println(F("!! No PSRAM detected — audio buffer will be tiny (expect glitches)."));

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);   // MAX98357A is mono; halves decode/I2S work on the S2
  audio.setVolume(TEST_VOLUME);
  Serial.print(F("Streaming: "));
  Serial.println(TEST_URL);
  audio.connecttohost(TEST_URL);
}

void loop() {
  audio.loop();   // must be serviced constantly to keep the stream fed
}

// ---- optional diagnostics emitted by ESP32-audioI2S ----
void audio_info(const char* info)     { Serial.print(F("[info] ")); Serial.println(info); }
void audio_id3data(const char* info)  { Serial.print(F("[id3 ] ")); Serial.println(info); }
void audio_showstreamtitle(const char* info) { Serial.print(F("[song] ")); Serial.println(info); }
void audio_eof_mp3(const char* info)  { Serial.print(F("[eof ] ")); Serial.println(info);
                                        Serial.println(F("Playback finished.")); }
