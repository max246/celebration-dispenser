# Brag Plan: Celebration Dispenser

## What is this app?
A button-triggered desktop machine that dispenses a treat (candy/confetti) while
flashing LED "eyes", spinning a finger-wheel, and playing a fanfare — a physical
celebration button, driven by an ESP32-S2 with real motion control.

## The angle
It's a candy machine that thinks it's a robot. Press a big button → the whole
machine performs: eyes flash orange, the wheel dispenses, and an IR break-beam
sensor actually *confirms the treat dropped* before it's happy — StallGuard
anti-jam and drop-detection, for candy. Serious robotics, joyful purpose.

## Hook (first 2-3 seconds)
A single big glowing button in the dark, breathing. A press lands with a click →
the screen bursts. "Press the button."

## Key moments (the middle)
- Two orange "eyes" flash on (the real celebration look).
- The finger-wheel spins and a candy drops through an IR beam → "drop confirmed ✓".
- A rainbow sweep across a WS2812 strip + a sound wave (audio plays).
- A quick spec flash: ESP32-S2 · TMC2209 StallGuard anti-jam · IR break-beam drop
  sensor · WS2812 light show · WAV audio over I2S.

## Outro / punchline
"It even knows when the candy actually dropped." → repo + Open source · MIT, with
a final confetti pop.

## User flow worth showing
Entry → key action → result: **press the button → eyes flash + wheel dispenses →
beam confirms the drop → lights + sound → ready for the next win.** This is the
centerpiece (Scenes 1→3).

## Tone
- Preset: default
- Creative direction: joyful desktop celebration-machine launch — a maker product reveal
- Interpretation: playful, clean, postable; snappy entrances that HOLD long enough
  to read; warmth and delight over corporate polish; motion carries the pace.

## Format: landscape — 1920x1080
## Duration: ~19s

## Visual identity (from the project)
- Background: #0e0f13 (near-black, "device off" darkness)
- Accent: #ff7a1a (the orange eye colour from config.h — EYE_R/G/B ≈ 255,60,0)
- Secondary accents: rainbow pops (WS2812), warm candy tones
- Text: #f5f5f2 (off-white)
- Display font: bold geometric sans (Poppins/Inter-ish); Body/spec: monospace (device vibe)
- Strongest visual element: the celebration burst — orange eyes + candy drop + rainbow sweep

## Share copy (draft)
I built a physical button that dispenses candy, flashes its eyes, and plays a
fanfare — and it actually knows when the treat dropped. 🍬🎉 (ESP32 + open source)

## Audio direction
- Role: warm upbeat music bed with tasteful, motion-matched SFX
- Music: a bundled "happy-beats-business-moves" track (upbeat, postable)
- Music treatment: start ~0s at moderate level, small duck under the final line,
  let a final chime/confetti SFX ring over the outro.
- Music cue guidance: use the bundled track's cue preset; lock the Scene-2 title
  reveal and the Scene-3 "drop confirmed" payoff to strong cues; snap the Scene-4
  spec cards to the beat grid (every other beat so labels stay readable).
- Audio-reactive treatment: subtle — let the eyes' glow / background warmth breathe
  with the music RMS; no waveform bars.
- SFX posture: moderate, motion-matched — button click, motor whir, a candy "tock"
  on the drop, a soft confirm chime, a final celebratory pop.
- Audio-coupled moments: button press (click), candy drop (tock + chime), spec
  cards (soft ticks), outro (confetti pop).
- Restraint rule: no strobing, no equalizer visuals, no wall of SFX; each sound
  ties to one visible action.

## Storyboard

### Scene 1 — Press the button — 3.0s
Dark screen. A big circular button glows/breathes in the centre (orange rim).
Text under it: "Press the button." A press animates (button depresses + flash).
Sequential/interaction: yes — simulate the button press (depress + release).
Audio intent: quiet anticipation → a satisfying click on the press.
Audio-coupled idea: button-press click SFX on the depress.
Music: soft intro.
Transition mood: hard (flash) → Scene 2

### Scene 2 — Reveal — 3.5s
The burst resolves into the product title: "CELEBRATION DISPENSER" slams in,
with a stylized machine silhouette (hopper + body + legs) behind it. Subtitle:
"a desktop machine for winning." Two orange eyes blink on.
Sequential/interaction: none (title + eyes reveal).
Audio intent: arrival — the drop/downbeat lands with the title.
Audio-coupled idea: title reveal beat-locked to a strong cue.
Music: full bed kicks in.
Transition mood: clean → Scene 3

### Scene 3 — The celebration sequence — 5.5s
The centerpiece. Left: the finger-wheel icon spins. A candy pill falls, crosses a
dashed IR beam line → beam flashes → "drop confirmed ✓" stamps in. Above, two
orange eyes flash in rhythm; a rainbow bar sweeps left→right (WS2812). A small
"♪" / soundwave pulses (audio plays).
Sequential/interaction: yes — candy falls → beam breaks → confirm stamp (one by one).
Audio intent: playful mechanical energy → a clean confirm chime on ✓.
Audio-coupled idea: motor whir during the spin; candy "tock" at the beam; chime on ✓.
Music: driving.
Transition mood: soft → Scene 4

### Scene 4 — Under the hood — 4.0s
Spec cards arrive one by one (readable, every-other-beat): "ESP32-S2",
"TMC2209 · StallGuard anti-jam", "IR break-beam drop sensor", "WS2812 light show",
"WAV audio over I2S". A faint real wiring-diagram texture behind, if clean.
Sequential/interaction: yes — 4–5 spec chips pop in one by one, held together after.
Audio intent: confident, ticking momentum.
Audio-coupled idea: soft tick per chip (every other beat so text stays readable).
Music: steady.
Transition mood: clean → Scene 5

### Scene 5 — Outro — 3.0s
Big line: "It even knows when the candy actually dropped." Then the repo:
"github.com/max246/celebration-dispenser" · "Open source · MIT". A final confetti
pop in orange + rainbow.
Sequential/interaction: yes — final confetti burst.
Audio intent: payoff → a celebratory pop that rings as the music fades.
Audio-coupled idea: confetti-pop SFX over a music duck/fade.
Music: fade under the final line.
Transition mood: end.

**Music mood for this video:** upbeat
**Audio summary:** soft anticipatory intro → a downbeat that lands the title →
driving playful energy through the celebration and specs, motion-matched SFX on
each action → a final confetti pop that rings as the music fades.
