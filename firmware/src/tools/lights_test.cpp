// ===========================================================================
//  WS2812 LED-strip test — Celebration Dispenser
//
//  Verifies the addressable strip wiring in isolation, with patterns chosen to
//  expose the usual mistakes:
//    A. First pixel only (red)  -> which end is DATA-IN; is data getting through?
//    B. Solid RED / GREEN / BLUE -> pixel count + colour order (GRB vs RGB).
//    C. Single dot running the length -> every pixel lights; strip length.
//    D. Rainbow -> the real celebration effect (same math as lights.cpp).
//
//      pio run -e lightstest -t upload
//      pio device monitor -e lightstest
//
//  Pin/count/brightness come from config.h (PIN_LED, LED_COUNT, LED_BRIGHTNESS).
//  Wiring: D12 -> 330-470R -> strip DIN; strip 5V -> USB/5V; strip GND -> GND
//  (common ground with the Feather). A 1000uF cap across the strip 5V/GND helps.
// ===========================================================================
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "config.h"

static Adafruit_NeoPixel strip(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);

// Phase timing (ms).
static const unsigned long A_MS = 1500;   // first-pixel
static const unsigned long WIPE_MS = 1200;  // each solid colour
static const unsigned long DOT_STEP_MS = 70;  // running-dot speed
static const unsigned long RAINBOW_MS = 4000;  // rainbow duration
static const unsigned long RAINBOW_CYCLE = 1200;

static void fillAll(uint32_t c) {
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, c);
  strip.show();
}

void setup() {
  Serial.begin(115200);
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== WS2812 LED-strip test ==="));
  Serial.printf("pin D%d, %d pixels, brightness %d\n", PIN_LED, LED_COUNT, LED_BRIGHTNESS);
  Serial.println(F("Expect: 1st pixel red -> RED/GREEN/BLUE fills -> running dot -> rainbow."));
  Serial.println(F("If fills are mislabelled, fix NEO_* colour order in lights.cpp."));
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  // A) First pixel only — confirms the data-in end and that data flows.
  Serial.println(F("[A] first pixel = RED"));
  strip.clear();
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(A_MS);

  // B) Solid colour wipes — confirms count and colour order.
  const uint32_t cols[3] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0),
                            strip.Color(0, 0, 255)};
  const char* names[3] = {"RED", "GREEN", "BLUE"};
  for (int k = 0; k < 3; k++) {
    Serial.printf("[B] all %s\n", names[k]);
    fillAll(cols[k]);
    delay(WIPE_MS);
  }

  // C) Running dot — every pixel should light in turn, 0..LED_COUNT-1.
  Serial.println(F("[C] running dot 0 -> end"));
  for (int i = 0; i < LED_COUNT; i++) {
    strip.clear();
    strip.setPixelColor(i, strip.Color(255, 255, 255));
    strip.show();
    delay(DOT_STEP_MS);
  }

  // D) Rainbow — the actual celebration animation.
  Serial.println(F("[D] rainbow"));
  const unsigned long start = millis();
  while (millis() - start < RAINBOW_MS) {
    const unsigned long elapsed = millis() - start;
    const uint16_t base = (uint16_t)((elapsed * 65535UL / RAINBOW_CYCLE) & 0xFFFF);
    for (int i = 0; i < LED_COUNT; i++) {
      const uint16_t hue = base + (uint16_t)(i * (65535UL / LED_COUNT));
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
    }
    strip.show();
    delay(16);
  }
}
