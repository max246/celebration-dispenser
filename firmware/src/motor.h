#pragma once

// TMC2209-driven dispense motor with StallGuard auto-unjam.
//
// The stepper is stepped in step/dir mode (AccelStepper) and configured over
// UART (TMCStepper). When the candy jams, the motor stalls, the driver pulls
// DIAG high, and this module backs off and retries automatically.
namespace motor {
void begin();       // configure the driver over UART — call once from setup()
void dispense();    // start a single dispense move
void update();       // run the motion + stall/unjam logic; call every loop
bool isBusy();       // true while dispensing or clearing a jam
bool wasJammed();    // true if the last dispense gave up after max retries
}  // namespace motor
