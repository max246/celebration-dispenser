// ===========================================================================
//  Motor bench test — Celebration Dispenser
//
//  Standalone TMC2209 + NEMA 17 bring-up. Independent of the main firmware:
//  no WiFi, no audio, no secrets.h needed. Build/flash with:
//
//      pio run -e motortest -t upload
//      pio device monitor -e motortest
//
//  What it does:
//   1. Opens UART to the TMC2209 and prints its version (0x21 = link OK).
//   2. Configures current / microstepping / StallGuard.
//   3. Continuously jogs the wheel +1 then -1 output revolution.
//   4. Every 250 ms prints speed, the DIAG pin, and the live StallGuard load
//      (SG_RESULT) so you can tune STALL_THRESHOLD and confirm the auto-unjam.
//
//  Pins/values below MUST match firmware/include/config.h.
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
static const uint8_t STALL_THRESHOLD = 80;
static const float STEPPER_MAX_SPEED = 1600.0f;
static const float STEPPER_ACCEL = 3200.0f;

// How much to jog each way, in WHEEL revolutions.
static const float JOG_REVS = 1.0f;

HardwareSerial& tmcSerial = Serial1;
TMC2209Stepper driver(&tmcSerial, TMC_RSENSE, TMC_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

static long jogSteps() {
  return (long)(JOG_REVS * GEAR_RATIO * STEPS_PER_REV * MICROSTEPPING);
}

void setup() {
  Serial.begin(115200);
  // Wait for the USB serial monitor to attach (or 8s) so boot prints aren't missed.
  for (unsigned long _t = millis(); !Serial && millis() - _t < 8000;) delay(10);
  delay(300);
  Serial.println(F("\n=== Celebration Dispenser — motor bench test ==="));

  pinMode(PIN_EN, OUTPUT);
  digitalWrite(PIN_EN, LOW);   // enable driver (active LOW)
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
  Serial.print(F("TMC2209 version: 0x"));
  Serial.println(ver, HEX);
  if (ver != 0x21) {
    Serial.println(F("!! UART link not confirmed (expected 0x21)."));
    Serial.println(F("   Check TX->PDN via 1k, RX->PDN, common GND, VIO=3V3."));
  }

  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);
  stepper.setEnablePin(-1);          // we drive EN ourselves
  stepper.moveTo(jogSteps());

  Serial.println(F("Jogging +/- 1 wheel rev. Press on the wheel to test stall."));
}

void loop() {
  stepper.run();

  // reverse at each end of travel
  if (stepper.distanceToGo() == 0) {
    stepper.moveTo(stepper.currentPosition() == 0 ? jogSteps() : 0);
  }

  static unsigned long last = 0;
  if (millis() - last >= 250) {
    last = millis();
    Serial.print(F("speed="));
    Serial.print(stepper.speed(), 0);
    Serial.print(F("  DIAG="));
    Serial.print(digitalRead(PIN_DIAG));
    Serial.print(F("  SG_RESULT="));
    Serial.println(driver.SG_RESULT());   // lower = more load; 0 near stall
  }
}
