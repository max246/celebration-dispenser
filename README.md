# Celebration Dispenser

[![firmware](https://github.com/max246/celebration-dispenser/actions/workflows/firmware.yml/badge.svg)](https://github.com/max246/celebration-dispenser/actions/workflows/firmware.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A button-triggered desktop celebration machine. Press the button and it dispenses
a treat (confetti, candy, sprinkles — your choice) via a stepper-driven mechanism
while a WS2812 LED strip runs a light show and a sound plays over a small
amplifier.

This repo has two halves:

| Folder      | What's inside                                                        |
|-------------|----------------------------------------------------------------------|
| [`hardware/`](hardware/) | 3D-printable STL parts (finger-wheel dispenser) + bill of materials + assembly guide |
| [`firmware/`](firmware/) | ESP32-S2 firmware (PlatformIO / Arduino) that runs the dispenser |

## Results

It ended up as a Halloween installation — watch it in action:

[![Halloween installation 2026 — watch on YouTube](https://img.youtube.com/vi/IRejrBzZh8A/maxresdefault.jpg)](https://www.youtube.com/watch?v=IRejrBzZh8A)

### Showcase

From printed prototypes to a candy-eating monster on Halloween night:

| | | |
|:---:|:---:|:---:|
| <img src="docs/showcase/01-finger-wheel-prototypes.jpg" width="260" alt="Finger-wheel prototypes"> | <img src="docs/showcase/02-wheel-in-housing.jpg" width="260" alt="Finger wheel in its housing"> | <img src="docs/showcase/03-bench-build.jpg" width="260" alt="Bench build on the stand"> |
| Finger-wheel prototypes | Wheel + gear in the housing | Full bench build: hopper, NEMA 17, ESP32-S2, button, LED strip |
| <img src="docs/showcase/04-monster-face.jpg" width="260" alt="Painting the monster face"> | <img src="docs/showcase/05-installed-halloween.jpg" width="260" alt="Installed for Halloween"> | <img src="docs/showcase/06-candy-hopper.jpg" width="260" alt="Candy loaded in the hopper"> |
| Painting the monster face | Installed: press the nose, get candy | Loaded with treats |

## How it works

```
        [ push button ]
              │  press
              ▼
    ┌──────────────┐   UART+step/dir   ┌──────────┐
    │  ESP32-S2    │ ────────────────▶ │ TMC2209  │─▶ NEMA 17 ─▶ finger wheel
    │  Feather     │ ◀──── DIAG stall ─┘ (auto-unjam on jam)
    │  (WiFi)      │ ────────────────▶ WS2812 strip ─▶ light show
    │              │ ──I2S──▶ MAX98357A ─▶ speaker ─▶ audio (from on-board flash)
    └──────────────┘
```

On a button press the ESP32-S2 runs a single **celebration sequence**: the
TMC2209 turns the finger wheel to dispense a portion (backing off and retrying if
StallGuard detects a jam), a rainbow sweeps the LED strip, and a sound plays from
on-board flash through the MAX98357A — all at the same time — then it returns to
idle.

## Quick start

1. **Print & wire the hardware** — see [`hardware/README.md`](hardware/README.md)
   and the wiring diagram in [`docs/WIRING.md`](docs/WIRING.md).
2. **Flash the firmware** — see [`firmware/README.md`](firmware/README.md). Bench-test
   the motor first with `pio run -e motortest -t upload`.
3. Power it up, press the button, celebrate.

## Tuning

Almost everything you'll want to change (dispense amount, speed, stall
sensitivity, LED count, volume, GPIO pins) lives in
[`firmware/include/config.h`](firmware/include/config.h); the celebration and
idle sounds are 16-bit WAV files in `firmware/data/` (uploaded with
`pio run -t uploadfs`). The printable parts and assembly guide live in
[`hardware/`](hardware/).

## License

[MIT](LICENSE) © Christian Bianchini. The sounds used in the final build are
included — source MP3s in `music/`, ready-to-flash WAVs in `firmware/data/`
(`pio run -t uploadfs`). They are not covered by the MIT license; swap in your
own if you redistribute.
