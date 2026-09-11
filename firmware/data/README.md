# Audio files (LittleFS)

Put the dispenser's sounds here, then upload them to the board's flash:

```bash
cd firmware
pio run -t uploadfs      # writes this folder to the LittleFS partition
```

Expected files (names set in [`../include/config.h`](../include/config.h)):

| File          | Played when            |
|---------------|------------------------|
| `celebrate.mp3` | on each button press (`AUDIO_FILE`) |
| `idle.mp3`      | looped while idle (`IDLE_FILE`)     |

Tips:
- **MP3, ≤128 kbps** keeps decode light on the S2 and files small.
- The LittleFS partition is ~1.4 MB, so keep the two files under that combined
  (≈90 s of 128 kbps audio). Shorten the idle loop if needed.
- The `.mp3` files are git-ignored (they can be large / personal); only this
  README is tracked.
