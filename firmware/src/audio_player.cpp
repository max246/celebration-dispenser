#include <Arduino.h>
#include <WiFi.h>

#include "Audio.h"  // ESP32-audioI2S (schreibfaul1)
#include "audio_player.h"
#include "config.h"

namespace {
Audio audio;
bool wifiOk = false;

enum Mode { OFF, IDLE_LOOP, CELEBRATION };
Mode mode = OFF;
unsigned long lastConnectMs = 0;

// Guard so a momentary isRunning()==false right after connecting doesn't cause
// us to re-connect the idle stream on top of itself.
const unsigned long kIdleRestartGuardMs = 1500;

void connectIdle() {
  audio.setVolume(IDLE_VOLUME);
  audio.connecttohost(IDLE_AUDIO_URL);
  lastConnectMs = millis();
}
}  // namespace

void audioplayer::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print(F("WiFi connecting"));
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(200);
    Serial.print('.');
  }
  wifiOk = (WiFi.status() == WL_CONNECTED);
  Serial.println();
  if (wifiOk) {
    // Disable WiFi modem power-save — its micro-sleeps stall the audio stream
    // on the single-core S2 and cause crackle/stutter.
    WiFi.setSleep(false);
    Serial.print(F("WiFi OK: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WiFi FAILED — running without audio."));
  }

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.forceMono(true);   // MAX98357A is mono; halves decode/I2S work on the S2
}

void audioplayer::playIdle() {
  if (!wifiOk || !ENABLE_IDLE_AUDIO) {
    mode = OFF;
    return;
  }
  mode = IDLE_LOOP;
  connectIdle();
}

void audioplayer::playCelebration() {
  if (!wifiOk) return;
  mode = CELEBRATION;
  audio.setVolume(CELEBRATION_VOLUME);
  audio.connecttohost(AUDIO_URL);
  lastConnectMs = millis();
}

void audioplayer::update() {
  audio.loop();

  // Loop the idle ambience: if it has run to the end, start it again.
  if (mode == IDLE_LOOP && !audio.isRunning() &&
      millis() - lastConnectMs > kIdleRestartGuardMs) {
    connectIdle();
  }
}

bool audioplayer::isCelebrationPlaying() {
  return mode == CELEBRATION && audio.isRunning();
}

bool audioplayer::wifiConnected() { return wifiOk; }
