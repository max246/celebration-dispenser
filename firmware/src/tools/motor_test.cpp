// ===========================================================================
//  Motor bench test — Celebration Dispenser
//
//  Standalone TMC2209 + NEMA 17 bring-up that exercises the real DISPENSE +
//  AUTO-UNJAM behaviour: it drives forward one dispense amount; if StallGuard
//  trips mid-move (jam), it backs off and retries forward, up to a limit; then
//  it pauses and dispenses again. Grab/hold the wheel to trigger the unjam.
//  Independent of the main firmware: no WiFi, no audio, no secrets.h.
//
//      pio run -e motortest -t upload
//      pio device monitor -e motortest
//
//  Values below MUST match firmware/include/config.h.
// ===========================================================================
#include <AccelStepper.h>
#include <Arduino.h>
#include <TMCStepper.h>

// ---- pins (match config.h) ----
static const int PIN_STEP = 5, PIN_DIR = 6, PIN_EN = 9, PIN_DIAG = 10;
static const int PIN_TMC_TX = 39, PIN_TMC_RX = 38;

// ---- driver / motion (match config.h) ----
static const float   TMC_RSENSE = 0.11f;
static const uint8_t TMC_ADDRESS = 0b00;
static const uint32_t TMC_BAUD = 115200;
static const int  MOTOR_CURRENT_MA = 600;
static const int  MICROSTEPPING = 16;
static const long STEPS_PER_REV = 200;
static const float GEAR_RATIO = 36.0f / 12.0f;   // 12T -> 36T
static const float DISPENSE_REVS = 1.0f;         // WHEEL revolutions per dispense
static const bool  DISPENSE_CW = false;
// Dispense at the SAME speed StallGuard was tuned at (SG_RESULT is speed-specific).
static const float STEPPER_MAX_SPEED = 1000.0f;
static const float STEPPER_ACCEL = 3200.0f;

// ---- stall / auto-unjam (match config.h) ----
// Detection reads SG_RESULT over UART (the DIAG pin didn't assert reliably on
// this driver). Free-running SG_RESULT is ~150+; a stall drops it toward 0.
static const uint8_t STALL_THRESHOLD = 50;       // SGTHRS (driver config)
static const uint16_t SG_STALL_LEVEL = 70;       // stall when SG_RESULT below this
static const int      SG_STALL_CONFIRM = 2;      // consecutive low reads to confirm
static const unsigned long SG_POLL_MS = 30;      // how often to read SG_RESULT
static const unsigned long STALL_IGNORE_MS = 120;
// Only check at cruise speed — StallGuard is invalid during accel/decel.
static const float STALL_MIN_SPEED = 0.9f * STEPPER_MAX_SPEED;
static const long  UNJAM_REVERSE_STEPS = (long)(0.25f * STEPS_PER_REV * MICROSTEPPING);
static const int   UNJAM_MAX_RETRIES = 3;

// how long to wait between dispenses in this test loop
static const unsigned long DISPENSE_INTERVAL_MS = 3000;

HardwareSerial& tmcSerial = Serial1;
TMC2209Stepper driver(&tmcSerial, TMC_RSENSE, TMC_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

enum Phase { WAITING, DISPENSING, UNJAM_REVERSE };
Phase phase = WAITING;
long dispenseTarget = 0;
int retries = 0;
unsigned long moveStartMs = 0;
unsigned long waitStartMs = 0;
int sgLowCount = 0;
unsigned long lastSgPollMs = 0;

static void enableDriver(bool on) { digitalWrite(PIN_EN, on ? LOW : HIGH); }

static long dispenseSteps() {
  const long s = (long)(DISPENSE_REVS * GEAR_RATIO * STEPS_PER_REV * MICROSTEPPING);
  return DISPENSE_CW ? s : -s;
}

// Stall via SG_RESULT (polled over UART). Valid only at cruise speed and past
// the accel window; needs SG_STALL_CONFIRM consecutive low reads to fire.
static bool stallDetected() {
  if (millis() - moveStartMs < STALL_IGNORE_MS) { sgLowCount = 0; return false; }
  if (fabs(stepper.speed()) < STALL_MIN_SPEED) { sgLowCount = 0; return false; }
  if (millis() - lastSgPollMs < SG_POLL_MS) return false;
  lastSgPollMs = millis();
  if (driver.SG_RESULT() < SG_STALL_LEVEL) {
    if (++sgLowCount >= SG_STALL_CONFIRM) { sgLowCount = 0; return true; }
  } else {
    sgLowCount = 0;
  }
  return false;
}

static void startDispense() {
  retries = 0;
  enableDriver(true);
  stepper.setCurrentPosition(0);
  dispenseTarget = dispenseSteps();
  stepper.moveTo(dispenseTarget);
  stepper.setSpeed(STEPPER_MAX_SPEED);  // after moveTo — constant-speed motion
  phase = DISPENSING;
  moveStartMs = millis();
  Serial.println(F("-> dispensing"));
}

static void beginUnjam() {
  Serial.printf("!! jam (SG=%u) -> back off, retry %d/%d\n", driver.SG_RESULT(),
                retries + 1, UNJAM_MAX_RETRIES);
  const long back = DISPENSE_CW ? -UNJAM_REVERSE_STEPS : UNJAM_REVERSE_STEPS;
  stepper.move(back);         // reverse relative to current position
  stepper.setSpeed(STEPPER_MAX_SPEED);
  phase = UNJAM_REVERSE;
  moveStartMs = millis();
}

void setup() {
  Serial.begin(115200);
  // Wait for the USB serial monitor to attach (or 8s) so boot prints aren't missed.
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== Celebration Dispenser — dispense + auto-unjam test ==="));

  pinMode(PIN_EN, OUTPUT);
  enableDriver(false);
  pinMode(PIN_DIAG, INPUT);

  tmcSerial.begin(TMC_BAUD, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(MOTOR_CURRENT_MA);
  driver.microsteps(MICROSTEPPING);
  driver.pwm_autoscale(true);
  driver.en_spreadCycle(false);      // StealthChop; StallGuard4 still works
  driver.TCOOLTHRS(0xFFFFF);
  driver.SGTHRS(STALL_THRESHOLD);

  const uint8_t ver = driver.version();
  Serial.printf("TMC2209 version: 0x%02X%s\n", ver,
                ver == 0x21 ? " (UART OK)" : " (!! check TX/1k, RX, GND, VIO)");

  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setEnablePin(-1);

  Serial.println(F("Dispensing every 3 s. Grab the wheel to trigger an unjam."));
  waitStartMs = millis();
}

void loop() {
  // live diagnostics while moving — watch SG/DIAG when you hold the wheel
  static unsigned long lastDbg = 0;
  if (phase != WAITING && millis() - lastDbg >= 200) {
    lastDbg = millis();
    Serial.printf("   [dbg] SG=%u dist=%ld spd=%.0f lowcnt=%d\n",
                  driver.SG_RESULT(), stepper.distanceToGo(), stepper.speed(),
                  sgLowCount);
  }

  switch (phase) {
    case WAITING:
      if (millis() - waitStartMs >= DISPENSE_INTERVAL_MS) startDispense();
      break;

    case DISPENSING:
      stepper.runSpeedToPosition();
      if (stallDetected()) {
        beginUnjam();
      } else if (stepper.distanceToGo() == 0) {
        Serial.println(F("-> done"));
        enableDriver(false);
        phase = WAITING;
        waitStartMs = millis();
      }
      break;

    case UNJAM_REVERSE:
      stepper.runSpeedToPosition();
      if (stepper.distanceToGo() == 0) {
        if (retries < UNJAM_MAX_RETRIES) {
          retries++;
          stepper.moveTo(dispenseTarget);   // resume toward the original target
          stepper.setSpeed(STEPPER_MAX_SPEED);
          phase = DISPENSING;
          moveStartMs = millis();
          Serial.println(F("-> retry forward"));
        } else {
          Serial.println(F("!! still jammed after max retries — giving up"));
          enableDriver(false);
          phase = WAITING;
          waitStartMs = millis();
        }
      }
      break;
  }
}
