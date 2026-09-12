#pragma once

// ===========================================================================
//  Celebration Dispenser — configuration
//  Target board: Adafruit ESP32-S2 Feather (4 MB flash, 2 MB PSRAM)
//  Edit this file to retune the device.
// ===========================================================================

// Audio is 16-bit WAV preloaded into PSRAM and played from RAM (not streamed,
// not MP3) — the only path that stays clean on the single-core S2. Put your WAVs
// in firmware/data/ and upload them with `pio run -t uploadfs`. No WiFi, no
// secrets.h needed for the main firmware.

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
// Dispense at the speed StallGuard was tuned at — SG_RESULT is speed-specific,
// so changing this means re-checking STALL_THRESHOLD with `motortest`.
static const float STEPPER_MAX_SPEED = 1000.0f;  // microsteps/sec
static const float STEPPER_ACCEL     = 3200.0f;  // microsteps/sec^2
// Release holding torque when idle? false = motor free-spins, silent, cooler.
static const bool  HOLD_TORQUE_WHEN_IDLE = false;

// ---- Stall detection / auto-unjam (StallGuard via DIAG pin) ----------------
// DIAG trips when SG_RESULT <= STALL_THRESHOLD*2. Set it below your free-running
// SG_RESULT (measure with `motortest`). Measured free ~150-205 -> 50 trips at
// 100, clear of normal running but catches a jam. Higher = trips more easily.
static const uint8_t STALL_THRESHOLD    = 50;
// Only evaluate a stall at cruise speed — StallGuard is invalid while the motor
// accelerates/decelerates. 0.9x max speed means "at full speed".
static const unsigned long STALL_IGNORE_MS = 120;
static const float STALL_MIN_SPEED         = 0.9f * STEPPER_MAX_SPEED;
// On a jam: back off this many microsteps, then retry the dispense.
static const long UNJAM_REVERSE_STEPS = (long)(0.25f * STEPS_PER_REV * MICROSTEPPING);
static const int  UNJAM_MAX_RETRIES   = 3;  // give up (and log) after this many

// ---- LED strip -------------------------------------------------------------
static const int LED_COUNT      = 16;
static const int LED_BRIGHTNESS = 120;  // 0-255

// ---- Button ----------------------------------------------------------------
static const unsigned long DEBOUNCE_MS = 40;

// ---- Audio (MAX98357A over I2S, 16-bit WAV preloaded into PSRAM) ------------
// Two 16-bit PCM WAV files on flash: a looping IDLE_FILE while waiting, and
// AUDIO_FILE for the celebration. They are loaded into PSRAM at boot and played
// from RAM (no MP3 decode / no flash reads mid-playback -> clean on the S2).
// Encode mono, e.g. 22050 Hz: see firmware/data/README.md. Put both in
// firmware/data/ and run `pio run -t uploadfs`. Volumes are 0-21.
static const char* const AUDIO_FILE = "/celebrate.wav";
static const char* const IDLE_FILE  = "/idle.wav";
static const int  CELEBRATION_VOLUME = 21;    // 0-21 (21 = full digital scale)
static const bool ENABLE_IDLE_AUDIO  = true;  // loop an ambient sound while idle
static const int  IDLE_VOLUME        = 12;    // usually quieter than celebration

// ---- Celebration timing ----------------------------------------------------
// The show runs at least this long even if the motor finishes early.
static const unsigned long MIN_CELEBRATION_MS = 1500;
