# Firmware

ESP32-S2 firmware for the Celebration Dispenser, built with
[PlatformIO](https://platformio.org/) on the Arduino framework.

**Board:** Adafruit ESP32-S2 Feather (4 MB flash, 2 MB PSRAM).

## Layout

```
firmware/
├── platformio.ini            # board + pinned dependencies
├── include/
│   ├── config.h              # ALL tunables: pins, dispense, stall, LEDs, audio
│   ├── secrets.h.example      # copy -> secrets.h (WiFi + stream URL; git-ignored)
│   └── secrets.h              # your credentials (not committed)
└── src/
    ├── main.cpp              # button + celebration state machine
    ├── motor.{h,cpp}         # TMC2209 (UART) + AccelStepper + StallGuard unjam
    ├── audio_player.{h,cpp}  # WiFi + streamed-URL audio over I2S (MAX98357A)
    └── lights.{h,cpp}        # non-blocking WS2812 rainbow show
```

## First-time setup

```bash
cd firmware
cp include/secrets.h.example include/secrets.h   # then edit in your WiFi + URL
```

## Wiring

Default pin map (edit in [`include/config.h`](include/config.h)). Pins are named
by the Feather's **silkscreen labels**. For a wiring diagram of the driver +
audio, see [`docs/WIRING.md`](../docs/WIRING.md).

### TMC2209 stepper driver

| Feather | TMC2209        | Notes                                         |
|---------|----------------|-----------------------------------------------|
| D5      | STEP           |                                               |
| D6      | DIR            |                                               |
| D9      | EN             | active LOW; released when idle                |
| D10     | DIAG           | driver pulls HIGH on stall → triggers unjam   |
| TX      | PDN_UART       | **through a 1 kΩ resistor**                    |
| RX      | PDN_UART       | direct (single-wire UART node)                |
| 3.3V    | VIO            | logic supply                                  |
| GND     | GND            | common ground                                 |

Also on the driver: **VM + GND** → motor supply (e.g. 12 V) with a **100 µF cap**
across them; coil pairs → **1A/1B, 2A/2B**; **MS1 + MS2 → GND** (sets UART address
`0b00`, matching `TMC_ADDRESS`). Set the run current with `MOTOR_CURRENT_MA` in
`config.h` (it's configured over UART — no Vref pot to tune).

### Audio — MAX98357A (I2S)

| Feather | MAX98357A | Notes                          |
|---------|-----------|--------------------------------|
| SCK     | BCLK      |                                |
| MO      | LRC       | word-select                    |
| MI      | DIN       |                                |
| USB/5V  | VIN       | 2.5–5.5 V                      |
| GND     | GND       |                                |

Speaker (4–8 Ω) to the amp's **+ / –** screw terminals. Leave `SD` unconnected
(enabled) and `GAIN` unconnected for the default 9 dB.

### Button & LEDs

| Feather | To                              |
|---------|---------------------------------|
| D11     | push button → **GND** (internal pull-up) |
| D12     | WS2812 **DIN** (330–470 Ω in series)     |

> ⚠️ Share **all grounds** (Feather, driver logic, motor supply, amp, LEDs).
> Don't power the motor from the Feather — use a dedicated motor supply.

## Build & flash

```bash
pio run                 # compile
pio run -t upload       # flash over USB (native USB CDC on the S2)
pio device monitor      # serial log @ 115200
```

On boot the serial log prints the WiFi result and the TMC2209 version
(`0x21` = UART link good).

## Bench test (motor)

Before wiring the whole thing, bring up just the TMC2209 + motor with the
standalone test in `src/tools/motor_test.cpp` — a separate PlatformIO env that
needs **no WiFi, audio, or `secrets.h`**:

```bash
pio run -e motortest -t upload
pio device monitor -e motortest
```

It prints the driver version (`0x21` = UART OK), jogs the wheel ±1 revolution,
and streams `speed` / `DIAG` / `SG_RESULT` so you can confirm motion and tune
`STALL_THRESHOLD` (push on the wheel and watch `SG_RESULT` drop toward 0 / `DIAG`
go HIGH). Wiring for this is the TMC2209 half of
[`docs/WIRING.md`](../docs/WIRING.md).

## Behavior

While **idle**, a looping ambient sound (`IDLE_AUDIO_URL`, at `IDLE_VOLUME`)
plays over the amp — set `ENABLE_IDLE_AUDIO = false` to disable it.

Press the button → one **celebration**, all at once:
- **Dispense:** the TMC2209 turns the finger wheel `DISPENSE_REVS` revolutions.
- **Auto-unjam:** if the candy jams, the motor stalls, DIAG goes HIGH, and the
  firmware backs off `UNJAM_REVERSE_STEPS` and retries — up to
  `UNJAM_MAX_RETRIES` times before giving up and logging a jam.
- **Lights:** a rainbow sweeps the WS2812 strip.
- **Audio:** the idle loop is interrupted and `AUDIO_URL` streams once to the
  MAX98357A (at `CELEBRATION_VOLUME`); the idle loop resumes when it ends. If
  WiFi didn't connect, the rest of the celebration still runs (just no sound).

Presses during a celebration are ignored. When idle, the motor is de-energized
(silent, cool) unless `HOLD_TORQUE_WHEN_IDLE = true`.

## Tuning cheatsheet

| Want to…                        | Change in `config.h`                       |
|---------------------------------|--------------------------------------------|
| Dispense more/less              | `DISPENSE_REVS` (wheel revolutions)        |
| Change the gears                | `GEAR_RATIO` (default 36T/12T = 3:1)       |
| Dispense faster/slower          | `STEPPER_MAX_SPEED`, `STEPPER_ACCEL`      |
| Reverse direction               | `DISPENSE_CW`                             |
| Motor current                   | `MOTOR_CURRENT_MA`                        |
| Stall sensitivity               | `STALL_THRESHOLD` (higher = trips easier) |
| Unjam back-off / retries        | `UNJAM_REVERSE_STEPS`, `UNJAM_MAX_RETRIES`|
| Quieter vs. more torque         | `USE_STEALTHCHOP`                        |
| More/fewer LEDs, dimmer         | `LED_COUNT`, `LED_BRIGHTNESS`            |
| Volumes                         | `CELEBRATION_VOLUME`, `IDLE_VOLUME` (0–21) |
| Idle ambience on/off            | `ENABLE_IDLE_AUDIO`                       |
| Sounds / WiFi                   | `secrets.h` (`AUDIO_URL`, `IDLE_AUDIO_URL`, `WIFI_*`) |

## Notes

- Dependencies (pinned in `platformio.ini`): **AccelStepper**, **Adafruit
  NeoPixel**, **TMCStepper**, **ESP32-audioI2S** `2.0.6`.
- The audio library and arduino-esp32 core are a matched pair: this project is
  on core **2.0.x** (platform `espressif32@6.9.0`) with audio **2.0.6**. The
  audio **3.x** line requires core 3.x — upgrade both together if you move.
- The S2 is single-core, so MP3 decoding and software step generation share one
  CPU. It builds and runs, but if you hear audio stutter under heavy dispensing,
  lower `STEPPER_MAX_SPEED` or move to an ESP32-S3.
