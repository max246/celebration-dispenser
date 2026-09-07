#include <Arduino.h>

#include "audio_player.h"
#include "config.h"
#include "lights.h"
#include "motor.h"

enum State { IDLE, CELEBRATING };
static State state = IDLE;
static unsigned long celebrationStart = 0;

// ---- button debounce state ----
static int lastReading = HIGH;
static int stableState = HIGH;
static unsigned long lastChangeMs = 0;

// Returns true once per fresh press (falling edge; button wired to GND).
static bool buttonPressed() {
  const int reading = digitalRead(PIN_BUTTON);
  const unsigned long now = millis();

  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }
  if (now - lastChangeMs >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (stableState == LOW) return true;  // just pressed
  }
  return false;
}

static void startCelebration() {
  state = CELEBRATING;
  celebrationStart = millis();
  motor::dispense();
  lights::startShow();
  audioplayer::playCelebration();  // takes over from the idle ambience
}

static void endCelebration() {
  state = IDLE;
  lights::off();
  audioplayer::playIdle();  // back to the looping idle sound
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  lights::begin();
  motor::begin();
  audioplayer::begin();  // brings up WiFi (may take a few seconds)
  audioplayer::playIdle();  // start the looping idle ambience

  Serial.println(F("Celebration Dispenser ready. Press the button!"));
}

void loop() {
  // The audio decoder must be serviced constantly to keep the stream fed.
  audioplayer::update();

  const bool pressed = buttonPressed();

  switch (state) {
    case IDLE:
      if (pressed) {
        Serial.println(F("Celebrate!"));
        startCelebration();
      }
      break;

    case CELEBRATING: {
      motor::update();
      lights::update();

      const unsigned long elapsed = millis() - celebrationStart;
      const bool motorDone = !motor::isBusy();
      const bool audioDone = !audioplayer::isCelebrationPlaying();
      const bool minTimeUp = elapsed >= MIN_CELEBRATION_MS;

      if (motorDone && audioDone && minTimeUp) {
        endCelebration();
        Serial.println(motor::wasJammed()
                           ? F("Done (dispense jammed — check the hopper).")
                           : F("Done. Ready for the next one."));
      }
      break;
    }
  }
}
