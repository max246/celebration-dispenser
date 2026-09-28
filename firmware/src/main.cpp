#include <Arduino.h>

#include "audio_player.h"
#include "beam.h"
#include "button_led.h"
#include "config.h"
#include "lights.h"
#include "motor.h"

// Behaviour:
//   IDLE        button LED breathes; the idle sound chimes every
//               IDLE_AUDIO_PERIOD_MS (5 min). A button press starts a celebration.
//   CELEBRATING button LED off; the eyes flash orange; the celebration sound
//               plays; the motor dispenses (with anti-jam) until the drop sensor
//               sees a treat fall, or DISPENSE_TIMEOUT_MS (30 s) gives up. Once
//               the dispense is done and the sound has finished, it returns to IDLE.
enum State { IDLE, CELEBRATING };
static State state = IDLE;
static unsigned long celebrationStart = 0;
static unsigned long nextIdleAudioMs = 0;
static bool dispenseDone = false;
static bool idleGlowOn = false;  // FLASH section glowing yellow for the idle chime

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

static void scheduleNextIdleAudio() {
  nextIdleAudioMs = millis() + IDLE_AUDIO_PERIOD_MS;
}

static void startCelebration() {
  state = CELEBRATING;
  celebrationStart = millis();
  dispenseDone = false;
  beam::resetDrops();
  idleGlowOn = false;              // the eyes show replaces the idle glow
  buttonled::off();                // button goes dark for the show
  lights::startEyes();             // orange eyes flash
  audioplayer::playCelebration();  // takes over from the idle chime
  motor::run();                    // start dispensing (continuous, auto-unjam)
}

static void endCelebration() {
  state = IDLE;
  motor::stop();
  lights::off();
  buttonled::breathe();     // glowing button again
  scheduleNextIdleAudio();  // next idle chime in 5 min (no immediate replay)
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
      motor::stop();  // wheel off; the celebration keeps running until the audio ends
      dispenseDone = true;
      Serial.println(F("dispense: treat dropped (finishing the track)"));
    } else if (elapsed > DISPENSE_TIMEOUT_MS) {
      motor::stop();
      dispenseDone = true;
      Serial.println(F("dispense: no drop within 30 s (empty/refill?)"));
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
  buttonled::begin();
  audioplayer::begin();

  buttonled::breathe();     // idle glow from the start
  scheduleNextIdleAudio();  // first idle chime one period from boot

  Serial.println(F("Celebration Dispenser ready. Press the button!"));
}

void loop() {
  audioplayer::update();  // service the audio constantly
  beam::update();          // debounce the drop sensor

  const bool pressed = buttonPressed();

  switch (state) {
    case IDLE:
      buttonled::update();  // breathe
      lights::update();     // pulse the idle glow (no-op when the strip is off)
      if (ENABLE_IDLE_AUDIO && (long)(millis() - nextIdleAudioMs) >= 0) {
        audioplayer::playIdle();
        if (audioplayer::isIdlePlaying()) {
          lights::startIdleGlow();  // yellow glow for as long as the chime plays
          idleGlowOn = true;
        }
        scheduleNextIdleAudio();
      }
      if (idleGlowOn && !audioplayer::isIdlePlaying()) {
        lights::off();
        idleGlowOn = false;
      }
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
