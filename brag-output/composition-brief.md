# Hyperframes Composition Brief: Celebration Dispenser

## Objective
A short, postable launch video for the Celebration Dispenser — a button-triggered
desktop candy machine driven by an ESP32-S2.

## Output
- Composition directory: `brag-output/composition/`
- Rendered video: `brag-output/brag.mp4`
- Format: landscape — 1920x1080
- Duration: 19s

## Source Material
- Project root: /home/christian/projects/celebration-dispenser
- Primary files read: README.md, firmware/include/config.h, firmware/src/main.cpp,
  hardware/full-machine-upright.png, docs/wiring-motor-audio.png
- Product name: Celebration Dispenser
- Strongest claim: it uses StallGuard anti-jam + an IR break-beam so it *knows* a
  candy actually dropped.
- Copy that must appear verbatim: "CELEBRATION DISPENSER", "It even knows when the
  candy actually dropped.", "github.com/max246/celebration-dispenser", "Open source · MIT"

## Creative Direction
- Tone preset: default (playful, clean, postable)
- Creative direction: joyful desktop celebration-machine launch — a maker product reveal
- Angle: a candy machine that thinks it's a robot — press a button and the whole
  machine performs, then confirms the drop before it's happy.
- Hook: a big glowing button breathing in the dark; a press bursts the screen.
- Outro: "It even knows when the candy actually dropped." → repo + MIT + confetti.
- Avoid: generic SaaS language, abstract filler, CAD-on-white.

## Visual Identity
- Background: #0e0f13
- Text: #f5f5f2 / muted #9aa0aa
- Accent: #ff7a1a (the firmware's EYE colour) + rainbow (WS2812) + green ✓
- Display font: system-ui bold; Body/spec: ui-monospace
- References: the machine silhouette (hopper + body + legs), orange "eyes", the
  finger-wheel, IR beam + drop confirm, rainbow strip.

## Storyboard
See `brag-output/brag-plan.md` (the creative contract). Scene summary:
1. Press the button — 3s — glowing button + "Press the button" → press + flash
2. Reveal — 3.5s — "CELEBRATION DISPENSER" + machine + orange eyes
3. Celebration — 5.5s — eyes flash, wheel spins, candy drops through the IR beam → "drop confirmed ✓", rainbow sweep
4. Under the hood — 4s — spec chips (ESP32-S2 · TMC2209 StallGuard · IR break-beam · WS2812 · WAV/I2S)
5. Outro — 3s — "It even knows when the candy actually dropped." + repo + confetti

## Audio
- Music: assets/music/bgm.mp3 (happy-beats-business-moves vol.1, 120 BPM), vol 0.5, 1.2s fade-out.
- SFX: press click (2.0s), candy drop (8.85s), confirm chime (9.05s), outro pop (16.2s).
- Beat: title reveal locked ~3.02s; spec chips on a 0.5s grid for readability.
- Audio-reactive: not wired (kept the build deterministic and simple); documented here.

## Notes
- Composition is monolithic (5 scene clips in index.html). `hyperframes check`
  passes with 0 errors; remaining warnings are Studio-timeline "use sub-compositions"
  suggestions, which don't affect the render.
