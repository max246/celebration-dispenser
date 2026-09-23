#pragma once

// The illuminated push button's LED (LEDC hardware PWM on PIN_BUTTON_LED).
// Idle = a slow "breathing" glow; during a celebration it is switched off.
namespace buttonled {
void begin();     // set up the PWM channel — call once from setup()
void breathe();   // enter the idle breathing glow
void off();        // switch the LED off (during a celebration)
void update();     // advance the breathe animation; call every loop iteration
}  // namespace buttonled
