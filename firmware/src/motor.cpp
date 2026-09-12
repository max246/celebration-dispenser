#include <AccelStepper.h>
#include <Arduino.h>
#include <TMCStepper.h>

#include "config.h"
#include "motor.h"

namespace {

// UART to the TMC2209 (single-wire PDN_UART via a 1k resistor on TX).
HardwareSerial& kTmcSerial = Serial1;
TMC2209Stepper driver(&kTmcSerial, TMC_RSENSE, TMC_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

enum Phase { IDLE, DISPENSING, UNJAM_REVERSE };
Phase phase = IDLE;

long dispenseTarget = 0;
int retries = 0;
bool jammed = false;
unsigned long moveStartMs = 0;
int sgLowCount = 0;
unsigned long lastSgPollMs = 0;

void enableDriver(bool on) {
  // TMC2209 EN is active LOW.
  digitalWrite(PIN_EN, on ? LOW : HIGH);
}

long dispenseSteps() {
  // DISPENSE_REVS is wheel output revolutions; multiply by the gear ratio to
  // get motor revolutions, then by steps/rev and microstepping.
  const long steps =
      (long)(DISPENSE_REVS * GEAR_RATIO * STEPS_PER_REV * MICROSTEPPING);
  return DISPENSE_CW ? steps : -steps;
}

// Stall via SG_RESULT polled over UART (the DIAG pin didn't assert reliably).
// Valid only at cruise speed and past the accel window; needs SG_STALL_CONFIRM
// consecutive low reads to fire.
bool stallDetected() {
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

void beginUnjam() {
  Serial.print(F("Jam detected — backing off (retry "));
  Serial.print(retries + 1);
  Serial.print('/');
  Serial.print(UNJAM_MAX_RETRIES);
  Serial.println(')');
  // Reverse relative to the current position, opposite the dispense direction.
  const long back = DISPENSE_CW ? -UNJAM_REVERSE_STEPS : UNJAM_REVERSE_STEPS;
  stepper.move(back);
  stepper.setSpeed(STEPPER_MAX_SPEED);  // constant speed (StallGuard needs steady timing)
  phase = UNJAM_REVERSE;
  moveStartMs = millis();
}

void finish() {
  phase = IDLE;
  if (!HOLD_TORQUE_WHEN_IDLE) enableDriver(false);
}

}  // namespace

void motor::begin() {
  pinMode(PIN_EN, OUTPUT);
  enableDriver(false);
  pinMode(PIN_DIAG, INPUT);

  kTmcSerial.begin(TMC_BAUD, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(MOTOR_CURRENT_MA);
  driver.microsteps(MICROSTEPPING);
  driver.pwm_autoscale(true);
  driver.en_spreadCycle(!USE_STEALTHCHOP);  // StallGuard4 works in StealthChop
  driver.TCOOLTHRS(0xFFFFF);                 // enable StallGuard down to low speed
  driver.SGTHRS(STALL_THRESHOLD);            // stall sensitivity -> DIAG output

  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setEnablePin(-1);  // we manage EN ourselves
  // Constant-speed motion (runSpeedToPosition), not accelerated — StallGuard
  // needs steady step timing, and it's tuned at this one speed.

  Serial.print(F("TMC2209 version: 0x"));
  Serial.println(driver.version(), HEX);  // 0x21 when the UART link is good
}

void motor::dispense() {
  retries = 0;
  jammed = false;
  enableDriver(true);
  stepper.setCurrentPosition(0);
  dispenseTarget = dispenseSteps();
  stepper.moveTo(dispenseTarget);
  stepper.setSpeed(STEPPER_MAX_SPEED);  // after moveTo — constant-speed motion
  phase = DISPENSING;
  moveStartMs = millis();
}

void motor::update() {
  switch (phase) {
    case IDLE:
      break;

    case DISPENSING:
      stepper.runSpeedToPosition();
      if (stallDetected()) {
        beginUnjam();
      } else if (stepper.distanceToGo() == 0) {
        finish();
      }
      break;

    case UNJAM_REVERSE:
      stepper.runSpeedToPosition();
      if (stepper.distanceToGo() == 0) {
        if (retries < UNJAM_MAX_RETRIES) {
          retries++;
          stepper.moveTo(dispenseTarget);  // resume toward the original target
          stepper.setSpeed(STEPPER_MAX_SPEED);
          phase = DISPENSING;
          moveStartMs = millis();
        } else {
          jammed = true;
          Serial.println(F("Still jammed after max retries — giving up."));
          finish();
        }
      }
      break;
  }
}

bool motor::isBusy() { return phase != IDLE; }

bool motor::wasJammed() { return jammed; }
