#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "lights.h"

namespace {

Adafruit_NeoPixel strip(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);

enum Mode { OFF_M, RAINBOW_M, EYES_M };
Mode mode = OFF_M;
unsigned long startMs = 0;
int lastEyeOn = -1;  // -1 = force first render; else 0/1 last blink state

const unsigned long kCycleMs = 1200;  // time for one full rainbow rotation

// Light a contiguous run of pixels [first, first+len) with the eye colour,
// clamped to the strip so a mis-set eye index can't run off the end.
void paintEye(int first, int len, uint32_t color) {
  for (int i = first; i < first + len; i++) {
    if (i >= 0 && i < LED_COUNT) strip.setPixelColor(i, color);
  }
}

}  // namespace

void lights::begin() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void lights::startShow() {
  mode = RAINBOW_M;
  startMs = millis();
}

void lights::startEyes() {
  mode = EYES_M;
  startMs = millis();
  lastEyeOn = -1;  // render on the next update()
}

void lights::update() {
  switch (mode) {
    case OFF_M:
      return;

    case RAINBOW_M: {
      const unsigned long elapsed = millis() - startMs;
      // Base hue sweeps the full wheel once per kCycleMs; each pixel is offset so
      // a rainbow spreads across the strip and appears to move.
      const uint16_t base = (uint16_t)((elapsed * 65535UL / kCycleMs) & 0xFFFF);
      for (int i = 0; i < LED_COUNT; i++) {
        const uint16_t hue = base + (uint16_t)(i * (65535UL / LED_COUNT));
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
      }
      strip.show();
      break;
    }

    case EYES_M: {
      // Square-wave blink; only push to the strip when the on/off state flips,
      // so we aren't calling show() every loop while the motor is stepping.
      const unsigned long period = EYE_FLASH_ON_MS + EYE_FLASH_OFF_MS;
      const int on = ((millis() - startMs) % period) < EYE_FLASH_ON_MS ? 1 : 0;
      if (on == lastEyeOn) break;
      lastEyeOn = on;
      strip.clear();
      if (on) {
        const uint32_t orange = strip.Color(EYE_R, EYE_G, EYE_B);
        paintEye(EYE1_FIRST, EYE1_LEN, orange);
        paintEye(EYE2_FIRST, EYE2_LEN, orange);
      }
      strip.show();
      break;
    }
  }
}

void lights::off() {
  mode = OFF_M;
  strip.clear();
  strip.show();
}
