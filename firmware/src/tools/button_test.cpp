// ===========================================================================
//  Push-button + illuminated-LED test — Celebration Dispenser
//
//  Verifies the trigger button AND its built-in LED in isolation.
//   - The switch: prints state once a second, logs every debounced press.
//   - The LED: a slow "breathing" glow while idle, solid full while held —
//     exactly how the real firmware will drive it.
//
//      pio run -e buttontest -t upload
//      pio device monitor -e buttontest
//
//  Wiring (both from config.h):
//   - Switch: one leg -> PIN_BUTTON, other leg -> GND (INPUT_PULLUP).
//   - LED:    PIN_BUTTON_LED -> ~220R -> LED anode(+), LED cathode(-) -> GND.
// ===========================================================================
#include <Arduino.h>
#include <math.h>

#include "config.h"

static int lastReading = HIGH;
static int stableState = HIGH;
static unsigned long lastChangeMs = 0;
static unsigned long presses = 0;

// LEDC full-scale duty for the configured resolution (8-bit -> 255).
static const int LED_FULL = (1 << BTN_LED_PWM_BITS) - 1;

// Sinusoidal breathe between IDLE_MIN and IDLE_MAX over BTN_LED_BREATHE_MS.
static int breatheDuty(unsigned long now) {
  const float phase = (now % BTN_LED_BREATHE_MS) / (float)BTN_LED_BREATHE_MS;
  const float s = 0.5f * (1.0f - cosf(2.0f * PI * phase));  // 0..1, smooth
  return BTN_LED_IDLE_MIN + (int)(s * (BTN_LED_IDLE_MAX - BTN_LED_IDLE_MIN));
}

void setup() {
  Serial.begin(115200);
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== push-button + LED test ==="));
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  lastReading = stableState = digitalRead(PIN_BUTTON);

  ledcSetup(BTN_LED_PWM_CH, BTN_LED_PWM_FREQ, BTN_LED_PWM_BITS);
  ledcAttachPin(PIN_BUTTON_LED, BTN_LED_PWM_CH);
  Serial.println(F("LED should breathe while idle; tap the button — it goes solid + counts."));
}

void loop() {
  const int reading = digitalRead(PIN_BUTTON);
  const unsigned long now = millis();
  if (reading != lastReading) {
    lastChangeMs = now;
    lastReading = reading;
  }
  if (now - lastChangeMs >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (stableState == LOW) {
      presses++;
      Serial.printf("PRESS  (presses=%lu)\n", presses);
    } else {
      Serial.println(F("release"));
    }
  }

  // LED: solid full while held (LOW), breathing glow while released (HIGH).
  const bool held = (stableState == LOW);
  ledcWrite(BTN_LED_PWM_CH, held ? LED_FULL : breatheDuty(now));

  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.printf("[button] %s  presses=%lu\n", held ? "HELD" : "released", presses);
  }
}
