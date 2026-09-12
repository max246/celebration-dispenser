#include <Arduino.h>

#include "beam.h"
#include "config.h"

namespace {
int lastReading = HIGH;
int stableState = HIGH;
unsigned long lastChangeMs = 0;
unsigned long dropCount = 0;

bool isBrokenLevel(int level) {
  return DROP_BEAM_ACTIVE_LOW ? (level == LOW) : (level == HIGH);
}
}  // namespace

void beam::begin() {
  pinMode(PIN_DROP_BEAM, INPUT_PULLUP);
  lastReading = digitalRead(PIN_DROP_BEAM);
  stableState = lastReading;
  lastChangeMs = millis();
  dropCount = 0;
}

void beam::update() {
  const int reading = digitalRead(PIN_DROP_BEAM);
  const unsigned long now = millis();
  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }
  if (now - lastChangeMs >= DROP_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (isBrokenLevel(stableState)) dropCount++;  // count each fresh break
  }
}

void beam::resetDrops() { dropCount = 0; }
unsigned long beam::drops() { return dropCount; }
bool beam::isBroken() { return isBrokenLevel(stableState); }
