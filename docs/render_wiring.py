#!/usr/bin/env python3
"""Regenerate docs/wiring-motor-audio.png.

The original full generator for this diagram was lost, so this script composites
the additions onto a pristine base image (`wiring-motor-audio-base.png`) instead
of redrawing everything. Re-running it is idempotent: it always starts from the
base. The base is drawn crisp at native resolution at the top, and the
illuminated push button is added in a new white band beneath it (so it never
crowds the ESP32 / drop-sensor corner).

    python3 docs/render_wiring.py
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle
from matplotlib.image import imread

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(HERE, "wiring-motor-audio-base.png")
OUT = os.path.join(HERE, "wiring-motor-audio.png")

# Palette (matched to the base diagram).
BTN = "#c0398a"   # button switch + LED wiring
GND = "#1a1a1a"   # common ground
MONO = {"family": "monospace"}

img = imread(BASE)
H, W = img.shape[0], img.shape[1]
BAND = 250                      # extra height added below the base for the button + strip
TOTAL = H + BAND

fig = plt.figure(figsize=(W / 100.0, TOTAL / 100.0), dpi=100, facecolor="white")
ax = fig.add_axes([0, 0, 1, 1])
ax.set_facecolor("white")
ax.imshow(img, extent=[0, W, H, 0], zorder=1)   # base pinned to the top, crisp
ax.set_xlim(0, W)
ax.set_ylim(TOTAL, 0)           # y grows downward; band lives in [H, TOTAL]
ax.axis("off")

# thin separator between the base diagram and the added band
ax.plot([0, W], [H + 2, H + 2], color="#cccccc", lw=1.0, zorder=2)

def wire(pts, color, lw=2.2, z=5):
    xs, ys = zip(*pts)
    ax.plot(xs, ys, color=color, lw=lw, solid_capstyle="round",
            solid_joinstyle="round", zorder=z)

def gnd_symbol(x, y, color=GND):
    """Standard ground symbol: three shrinking horizontal bars."""
    for i, hw in enumerate((11, 7, 3)):
        yy = y + i * 5
        ax.plot([x - hw, x + hw], [yy, yy], color=color, lw=2.2, zorder=6)

def flag(x, y, label, color=BTN):
    """A named net-label tag sitting on top of a wire end at (x, y)."""
    w, h = 48, 22
    ax.add_patch(FancyBboxPatch((x - w / 2, y - h), w, h,
                 boxstyle="round,pad=0,rounding_size=5",
                 linewidth=1.8, edgecolor=color, facecolor="white", zorder=6))
    ax.text(x, y - h / 2, label, ha="center", va="center",
            fontsize=9.5, color=color, zorder=7, **MONO)

# --- Illuminated push-button component (in the new bottom band) --------------
bx, by, bw, bh = 60, H + 70, 320, 130
ax.add_patch(FancyBboxPatch((bx, by), bw, bh,
             boxstyle="round,pad=0,rounding_size=12",
             linewidth=2.2, edgecolor=BTN, facecolor="#fbe3f0", zorder=4))
ax.text(bx + bw / 2, by + 26, "Illuminated push button",
        ha="center", va="center", fontsize=13, fontweight="bold",
        color="#7a1f5c", zorder=7, **MONO)
ax.text(bx + 70, by + 66, "SW", ha="center", va="center",
        fontsize=11, color="#333", zorder=7, **MONO)
ax.text(bx + 230, by + 66, "LED", ha="center", va="center",
        fontsize=11, color="#333", zorder=7, **MONO)
ax.text(bx + bw / 2, by + 104, "press: D11→GND  ·  LED: PWM glow (off in show)",
        ha="center", va="center", fontsize=8.5, color="#7a1f5c", zorder=7, **MONO)

SW_X, LED_X = bx + 70, bx + 230
TAG_Y = H + 44                  # net tags sit in the band, just below the base

# Switch signal -> D11 net tag.
wire([(SW_X, by), (SW_X, TAG_Y)], BTN)
flag(SW_X, TAG_Y, "D11")
# LED anode -> 220 ohm resistor -> D13 net tag.
rw, rh = 60, 24
ry = TAG_Y + 14
ax.add_patch(Rectangle((LED_X - rw / 2, ry), rw, rh, linewidth=1.8,
             edgecolor=GND, facecolor="white", zorder=6))
ax.text(LED_X, ry + rh / 2, "220Ω", ha="center", va="center",
        fontsize=9, color=GND, zorder=7, **MONO)
wire([(LED_X, by), (LED_X, ry + rh)], BTN)     # box top -> resistor
wire([(LED_X, ry), (LED_X, TAG_Y)], BTN)       # resistor -> tag
flag(LED_X, TAG_Y, "D13")

# Grounds -> local ground symbols (same common GND net as the rail above).
gnd_y = by + bh + 18
wire([(SW_X, by + bh), (SW_X, gnd_y)], GND)
wire([(LED_X, by + bh), (LED_X, gnd_y)], GND)
gnd_symbol(SW_X, gnd_y)
gnd_symbol(LED_X, gnd_y)

# --- WS2812 "eyes" strip (in the band, to the right of the button) ----------
LED_DATA = "#0e9bd6"   # WS2812 data line
V5 = "#e8912a"         # 5V, matched to the base diagram's "5V (USB)" colour
sx, sy, sw, sh = 560, H + 70, 380, 130
ax.add_patch(FancyBboxPatch((sx, sy), sw, sh,
             boxstyle="round,pad=0,rounding_size=12",
             linewidth=2.2, edgecolor="#8a8a8a", facecolor="#f0f0f0", zorder=4))
ax.text(sx + sw / 2, sy + 24, "WS2812 strip — eyes",
        ha="center", va="center", fontsize=13, fontweight="bold",
        color="#333", zorder=7, **MONO)
# a little run of pixels, two lit orange (the eyes)
px_n, px_w, gap = 10, 22, 6
row_w = px_n * px_w + (px_n - 1) * gap
px0 = sx + sw / 2 - row_w / 2
py = sy + 52
eyes = {2, 7}
for i in range(px_n):
    x = px0 + i * (px_w + gap)
    fc = "#ff7a1a" if i in eyes else "#ffffff"
    ax.add_patch(Rectangle((x, py), px_w, px_w, linewidth=1.2,
                 edgecolor="#999", facecolor=fc, zorder=6))
ax.text(sx + sw / 2, sy + sh - 14, "two small sections flash orange",
        ha="center", va="center", fontsize=8.5, color="#555", zorder=7, **MONO)

DIN_X, V5_X = sx + 55, sx + sw - 55
# DIN -> 330 ohm series resistor -> D12 net tag.
rw2, rh2 = 64, 24
ry2 = TAG_Y + 14
ax.add_patch(Rectangle((DIN_X - rw2 / 2, ry2), rw2, rh2, linewidth=1.8,
             edgecolor=GND, facecolor="white", zorder=6))
ax.text(DIN_X, ry2 + rh2 / 2, "330Ω", ha="center", va="center",
        fontsize=9, color=GND, zorder=7, **MONO)
wire([(DIN_X, sy), (DIN_X, ry2 + rh2)], LED_DATA)   # box top -> resistor
wire([(DIN_X, ry2), (DIN_X, TAG_Y)], LED_DATA)      # resistor -> tag
flag(DIN_X, TAG_Y, "D12", LED_DATA)
# 5V power -> net tag (USB/5V rail).
wire([(V5_X, sy), (V5_X, TAG_Y)], V5)
flag(V5_X, TAG_Y, "5V", V5)
# Ground -> local ground symbol.
gx = sx + sw / 2
gy2 = sy + sh + 18
wire([(gx, sy + sh), (gx, gy2)], GND)
gnd_symbol(gx, gy2)

fig.savefig(OUT, dpi=100)
print("wrote", OUT, "->", (TOTAL, W))
