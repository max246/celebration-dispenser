#pragma once

// TMC2209-driven dispense motor. Runs the wheel continuously ("dispensing") and,
// if the candy jams, automatically wiggles (reverse + forward) to clear it, up
// to ANTIJAM_TIMEOUT_MS before giving up. The caller decides when to stop
// (e.g. when the drop sensor confirms a treat fell, or a time limit is hit).
namespace motor {
void begin();          // configure the driver over UART — call once from setup()
void run();            // start dispensing (continuous forward)
void stop();            // stop and release holding torque
void update();          // run motion + auto anti-jam; call every loop iteration
bool isRunning();       // true while dispensing (or clearing a jam)
bool jammedGaveUp();    // true if anti-jam ran out of time without clearing
}  // namespace motor
