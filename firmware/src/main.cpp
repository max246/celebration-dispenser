#include <Arduino.h>

#include "audio_player.h"
#include "beam.h"
#include "config.h"
#include "lights.h"
#include "motor.h"

enum State { IDLE, CELEBRATING };
static State state = IDLE;
static unsigned long celebrationStart = 0;
static bool dispenseDone = false;

// Fixed dispense time (used only when the drop sensor is off): how long to turn
// DISPENSE_REVS at the run speed.
static const unsigned long DISPENSE_FIXED_MS =
    (unsigned long)(DISPENSE_REVS * GEAR_RATIO * STEPS_PER_REV * MICROSTEPPING /
                    STEPPER_MAX_SPEED * 1000.0f);

// ---- button debounce state ----
static int lastReading = HIGH;
static int stableState = HIGH;
static unsigned long lastChangeMs = 0;

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
  dispenseDone = false;
  beam::resetDrops();
  motor::run();                    // start dispensing (continuous)
  lights::startShow();
  audioplayer::playCelebration();  // takes over from the idle ambience
}

static void endCelebration() {
  state = IDLE;
  lights::off();
  audioplayer::playIdle();  // back to the looping idle sound
}

// Decide when the dispensing part of a celebration is finished, and stop the
// motor. With the drop sensor: run until a treat drops, or DISPENSE_TIMEOUT_MS,
// or anti-jam gives up. Without it: turn a fixed portion.
static void serviceDispense() {
  if (dispenseDone) return;

  const unsigned long elapsed = millis() - celebrationStart;

  if (motor::jammedGaveUp()) {
    dispenseDone = true;
    Serial.println(F("dispense: jammed, gave up clearing"));
    return;
  }

  if (ENABLE_DROP_SENSOR) {
    if (beam::drops() > 0) {
      motor::stop();
      dispenseDone = true;
      Serial.println(F("dispense: treat dropped"));
    } else if (elapsed > DISPENSE_TIMEOUT_MS) {
      motor::stop();
      dispenseDone = true;
      Serial.println(F("dispense: no drop within timeout (empty/refill?)"));
    }
  } else if (elapsed > DISPENSE_FIXED_MS) {
    motor::stop();
    dispenseDone = true;
    Serial.println(F("dispense: portion done"));
  }
}

void setup() {
  Serial.begin(115200);
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  lights::begin();
  motor::begin();
  beam::begin();
  audioplayer::begin();
  audioplayer::playIdle();  // start the looping idle ambience

  Serial.println(F("Celebration Dispenser ready. Press the button!"));
}

void loop() {
  audioplayer::update();  // service the audio decoder constantly
  beam::update();          // debounce the drop sensor

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
      serviceDispense();

      const unsigned long elapsed = millis() - celebrationStart;
      const bool audioDone = !audioplayer::isCelebrationPlaying();
      const bool minTimeUp = elapsed >= MIN_CELEBRATION_MS;

      if (dispenseDone && audioDone && minTimeUp) {
        endCelebration();
        Serial.println(F("Done. Ready for the next one."));
      }
      break;
    }
  }
}
