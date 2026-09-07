#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "lights.h"

namespace {

Adafruit_NeoPixel strip(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);
bool running = false;
unsigned long startMs = 0;

const unsigned long kCycleMs = 1200;  // time for one full color rotation

}  // namespace

void lights::begin() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void lights::startShow() {
  running = true;
  startMs = millis();
}

void lights::update() {
  if (!running) return;

  const unsigned long elapsed = millis() - startMs;
  // Base hue sweeps the full 16-bit color wheel once per kCycleMs, and each
  // pixel is offset so a rainbow is spread across the strip and appears to move.
  const uint16_t base = (uint16_t)((elapsed * 65535UL / kCycleMs) & 0xFFFF);
  for (int i = 0; i < LED_COUNT; i++) {
    const uint16_t hue = base + (uint16_t)(i * (65535UL / LED_COUNT));
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
  }
  strip.show();
}

void lights::off() {
  running = false;
  strip.clear();
  strip.show();
}
