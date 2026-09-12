#include <AccelStepper.h>
#include <Arduino.h>
#include <TMCStepper.h>

#include "config.h"
#include "motor.h"

// Continuous-run dispense engine with automatic anti-jam. Motion is constant
// speed (runSpeed / runSpeedToPosition) because StallGuard needs steady step
// timing. Stalls are detected by polling SG_RESULT over UART (the DIAG pin
// didn't assert on this driver).

namespace {

HardwareSerial& kTmcSerial = Serial1;
TMC2209Stepper driver(&kTmcSerial, TMC_RSENSE, TMC_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

enum Phase { STOPPED, FORWARD, REVERSING };
Phase phase = STOPPED;

bool gaveUp = false;
unsigned long fwdStartMs = 0;   // when the current forward run began (accel/settle window)
unsigned long jamStartMs = 0;   // start of the current jam episode (0 = none)

// SG_RESULT polling
int sgLowCount = 0;
unsigned long lastSgPollMs = 0;

void enableDriver(bool on) { digitalWrite(PIN_EN, on ? LOW : HIGH); }

float fwdSpeed() { return DISPENSE_CW ? STEPPER_MAX_SPEED : -STEPPER_MAX_SPEED; }

void startForward() {
  stepper.setSpeed(fwdSpeed());
  phase = FORWARD;
  fwdStartMs = millis();
}

// Stall via SG_RESULT, valid only at cruise speed and past the settle window.
bool stallDetected() {
  if (millis() - fwdStartMs < STALL_IGNORE_MS) { sgLowCount = 0; return false; }
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

void beginReverse() {
  const long back = DISPENSE_CW ? -UNJAM_REVERSE_STEPS : UNJAM_REVERSE_STEPS;
  stepper.move(back);                    // relative back-off from here
  stepper.setSpeed(STEPPER_MAX_SPEED);   // magnitude; direction from the target
  phase = REVERSING;
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
  driver.en_spreadCycle(!USE_STEALTHCHOP);
  driver.TCOOLTHRS(0xFFFFF);
  driver.SGTHRS(STALL_THRESHOLD);

  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setEnablePin(-1);  // we manage EN ourselves

  Serial.print(F("TMC2209 version: 0x"));
  Serial.println(driver.version(), HEX);  // 0x21 when the UART link is good
}

void motor::run() {
  gaveUp = false;
  jamStartMs = 0;
  enableDriver(true);
  startForward();
}

void motor::stop() {
  phase = STOPPED;
  if (!HOLD_TORQUE_WHEN_IDLE) enableDriver(false);
}

void motor::update() {
  switch (phase) {
    case STOPPED:
      break;

    case FORWARD:
      stepper.setSpeed(fwdSpeed());
      stepper.runSpeed();  // continuous forward at constant speed

      if (stallDetected()) {
        if (jamStartMs == 0) jamStartMs = millis();
        if (millis() - jamStartMs > ANTIJAM_TIMEOUT_MS) {
          Serial.println(F("anti-jam: gave up (still stuck)"));
          gaveUp = true;
          motor::stop();
        } else {
          Serial.println(F("jam — backing off to clear"));
          beginReverse();
        }
      } else if (jamStartMs != 0 && millis() - fwdStartMs > JAM_CLEAR_MS) {
        jamStartMs = 0;  // ran forward stall-free long enough — jam cleared
      }
      break;

    case REVERSING:
      stepper.runSpeedToPosition();
      if (stepper.distanceToGo() == 0) startForward();  // resume dispensing
      break;
  }
}

bool motor::isRunning() { return phase != STOPPED; }
bool motor::jammedGaveUp() { return gaveUp; }
