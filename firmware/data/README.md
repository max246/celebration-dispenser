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
- The LittleFS partition is **~2 MB** (see `../partitions_audio.csv`), so keep
  both files under that combined (≈2 min of 128 kbps audio). If a file is too
  big, re-encode lower, e.g.:
  ```bash
  ffmpeg -i in.mp3 -codec:a libmp3lame -b:a 96k -ac 1 idle.mp3
  ```
- The `.mp3` files are git-ignored (they can be large / personal); only this
  README is tracked.
