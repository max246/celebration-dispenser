// ===========================================================================
//  Break-beam drop-sensor test — Celebration Dispenser
//
//  Verifies the IR break-beam sensor wiring in isolation. Prints the beam state
//  once a second and logs every break (something passing through). Wave a finger
//  or drop a treat through the beam to see the count climb.
//
//      pio run -e beamtest -t upload
//      pio device monitor -e beamtest
//
//  Pin/polarity come from config.h (PIN_DROP_BEAM, DROP_BEAM_ACTIVE_LOW).
//  Receiver signal -> PIN_DROP_BEAM, receiver+emitter powered from 3.3V + GND.
// ===========================================================================
#include <Arduino.h>

#include "config.h"

static int lastReading = HIGH;
static int stableState = HIGH;
static unsigned long lastChangeMs = 0;
static unsigned long drops = 0;

static bool broken(int level) {
  return DROP_BEAM_ACTIVE_LOW ? (level == LOW) : (level == HIGH);
}

void setup() {
  Serial.begin(115200);
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== break-beam drop-sensor test ==="));
  pinMode(PIN_DROP_BEAM, INPUT_PULLUP);
  lastReading = stableState = digitalRead(PIN_DROP_BEAM);
  Serial.println(F("Wave something through the beam — each break should count."));
}

void loop() {
  const int reading = digitalRead(PIN_DROP_BEAM);
  const unsigned long now = millis();
  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }
  if (now - lastChangeMs >= DROP_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (broken(stableState)) {
      drops++;
      Serial.printf("BREAK  (drops=%lu)\n", drops);
    } else {
      Serial.println(F("clear"));
    }
  }

  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.printf("[beam] %s  drops=%lu\n", broken(stableState) ? "BROKEN" : "intact", drops);
  }
}
