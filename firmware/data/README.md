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

## Encoding (important for the single-core S2)

The S2 has one CPU core, so MP3 decode at 44.1 kHz can't quite keep up with
real-time — it plays a few seconds (the buffer), then crackles. Encode for a
light decode load:

- **22050 Hz, mono, CBR, no metadata** — halves the decode work vs 44.1 kHz:
  ```bash
  ffmpeg -i in.mp3 -map_metadata -1 -c:a libmp3lame -ar 22050 -b:a 96k -ac 1 idle.mp3
  ```
  (`-map_metadata -1` strips ID3/Xing headers, which can also stop it decoding.)
- **Still crackling? Use WAV** — raw PCM has *zero* decode cost, so it always
  plays clean on the S2:
  ```bash
  ffmpeg -i in.mp3 -map_metadata -1 -ar 22050 -ac 1 -c:a pcm_s16le idle.wav
  ```
  (then set the file names in `../include/config.h` to `.wav`.)

## Size

- The LittleFS partition is **~2 MB** (see `../partitions_audio.csv`). MP3 at
  96 kbps ≈ 12 KB/s (~2.7 min); 22 kHz mono WAV ≈ 44 KB/s (~45 s). Keep both
  files under 2 MB combined; shorten the idle loop if needed.
- The `.mp3` files are git-ignored (they can be large / personal); only this
  README is tracked.
