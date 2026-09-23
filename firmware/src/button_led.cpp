#include <Arduino.h>
#include <math.h>

#include "button_led.h"
#include "config.h"

namespace {
enum Mode { OFF_M, BREATHE_M };
Mode mode = OFF_M;
}  // namespace

void buttonled::begin() {
  ledcSetup(BTN_LED_PWM_CH, BTN_LED_PWM_FREQ, BTN_LED_PWM_BITS);
  ledcAttachPin(PIN_BUTTON_LED, BTN_LED_PWM_CH);
  ledcWrite(BTN_LED_PWM_CH, 0);
}

void buttonled::breathe() { mode = BREATHE_M; }

void buttonled::off() {
  mode = OFF_M;
  ledcWrite(BTN_LED_PWM_CH, 0);
}

void buttonled::update() {
  if (mode != BREATHE_M) return;
  // Sinusoidal breathe between IDLE_MIN and IDLE_MAX over BTN_LED_BREATHE_MS.
  const float phase = (millis() % BTN_LED_BREATHE_MS) / (float)BTN_LED_BREATHE_MS;
  const float s = 0.5f * (1.0f - cosf(2.0f * PI * phase));  // 0..1, smooth
  const int duty = BTN_LED_IDLE_MIN + (int)(s * (BTN_LED_IDLE_MAX - BTN_LED_IDLE_MIN));
  ledcWrite(BTN_LED_PWM_CH, duty);
}
