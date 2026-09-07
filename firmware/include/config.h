#pragma once

// ===========================================================================
//  Celebration Dispenser — configuration
//  Target board: Adafruit ESP32-S2 Feather (4 MB flash, 2 MB PSRAM)
//  Edit this file to retune the device.
// ===========================================================================

// WiFi credentials + stream URL live in secrets.h (git-ignored).
// Copy secrets.h.example -> secrets.h and fill it in.
#include "secrets.h"

// ---- GPIO pin map (Adafruit ESP32-S2 Feather) -----------------------------
// Numbers are the raw GPIOs; the Feather silkscreen label is in the comment.
// TMC2209 (step/dir + UART):
static const int PIN_STEP   = 5;   // "D5"  -> TMC2209 STEP
static const int PIN_DIR    = 6;   // "D6"  -> TMC2209 DIR
static const int PIN_EN     = 9;   // "D9"  -> TMC2209 EN   (active LOW)
static const int PIN_DIAG   = 10;  // "D10" <- TMC2209 DIAG (HIGH on stall)
static const int PIN_TMC_TX = 39;  // "TX"  -> TMC2209 PDN_UART (via 1k)
static const int PIN_TMC_RX = 38;  // "RX"  <- TMC2209 PDN_UART

// Controls / feedback:
static const int PIN_BUTTON = 11;  // "D11" push button to GND (INPUT_PULLUP)
static const int PIN_LED    = 12;  // "D12" WS2812 strip data (330-470R in series)

// MAX98357A I2S audio amp:
static const int PIN_I2S_BCLK = 36;  // "SCK" -> MAX98357A BCLK
static const int PIN_I2S_LRC  = 35;  // "MO"  -> MAX98357A LRC (word select)
static const int PIN_I2S_DOUT = 37;  // "MI"  -> MAX98357A DIN

// ---- TMC2209 stepper driver ------------------------------------------------
static const float TMC_RSENSE   = 0.11f;   // sense resistor on most 2209 modules
static const uint8_t TMC_ADDRESS = 0b00;   // set by MS1/MS2 (both to GND = 0)
static const uint32_t TMC_BAUD  = 115200;  // UART baud to the driver
static const int  MOTOR_CURRENT_MA = 600;  // RMS coil current — match your motor
static const int  MICROSTEPPING    = 16;   // driver microsteps (set over UART)
static const bool USE_STEALTHCHOP  = true; // quiet mode (StallGuard4 still works)

// ---- Stepper / dispensing --------------------------------------------------
static const long STEPS_PER_REV = 200;     // 1.8°/step NEMA 17 = 200 full steps
// Gear reduction from motor to the finger wheel: 12T pinion -> 36T gear = 3:1.
// The motor turns GEAR_RATIO times for one wheel revolution.
static const float GEAR_RATIO   = 36.0f / 12.0f;
static const float DISPENSE_REVS = 1.0f;   // WHEEL (output) revolutions per celebration
static const bool  DISPENSE_CW   = true;   // false to reverse
static const float STEPPER_MAX_SPEED = 1600.0f;  // microsteps/sec
static const float STEPPER_ACCEL     = 3200.0f;  // microsteps/sec^2
// Release holding torque when idle? false = motor free-spins, silent, cooler.
static const bool  HOLD_TORQUE_WHEN_IDLE = false;

// ---- Stall detection / auto-unjam (StallGuard via DIAG pin) ----------------
// SGTHRS: higher = trips more easily. Tune per motor/load (0-255). Start ~60-90.
static const uint8_t STALL_THRESHOLD    = 80;
// Ignore stalls for this long after a move starts (StallGuard is invalid during
// initial acceleration / below a minimum speed).
static const unsigned long STALL_IGNORE_MS = 120;
static const float STALL_MIN_SPEED         = 300.0f;  // microsteps/sec
// On a jam: back off this many microsteps, then retry the dispense.
static const long UNJAM_REVERSE_STEPS = (long)(0.25f * STEPS_PER_REV * MICROSTEPPING);
static const int  UNJAM_MAX_RETRIES   = 3;  // give up (and log) after this many

// ---- LED strip -------------------------------------------------------------
static const int LED_COUNT      = 16;
static const int LED_BRIGHTNESS = 120;  // 0-255

// ---- Button ----------------------------------------------------------------
static const unsigned long DEBOUNCE_MS = 40;

// ---- Audio (MAX98357A over I2S, streamed from a URL) -----------------------
// Two streams (URLs in secrets.h): a looping IDLE_AUDIO_URL while waiting, and
// AUDIO_URL for the celebration. Volumes are on the 0-21 ESP32-audioI2S scale.
static const int  CELEBRATION_VOLUME = 15;
static const bool ENABLE_IDLE_AUDIO  = true;  // loop an ambient sound while idle
static const int  IDLE_VOLUME        = 8;     // usually quieter than celebration
static const unsigned long WIFI_TIMEOUT_MS = 12000;  // connect timeout at boot

// ---- Celebration timing ----------------------------------------------------
// The show runs at least this long even if the motor finishes early.
static const unsigned long MIN_CELEBRATION_MS = 1500;
