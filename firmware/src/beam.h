#pragma once

// IR break-beam drop sensor. The receiver signal is HIGH when the beam is intact
// and LOW when a treat passes through it. Counts debounced beam-break events.
namespace beam {
void begin();          // configure the input — call once from setup()
void update();          // debounce + count breaks; call every loop iteration
void resetDrops();      // zero the counter (call at the start of a dispense)
unsigned long drops();  // number of beam breaks since the last reset
bool isBroken();        // true while the beam is currently broken
}  // namespace beam
