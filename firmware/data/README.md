# Audio files (LittleFS)

Put the dispenser's sounds here as **WAV**, then upload them to the board's flash:

```bash
cd firmware
# make mono 16-bit WAVs (see Encoding below), e.g.:
#   ffmpeg -i celebrate.mp3 -map_metadata -1 -ar 22050 -ac 1 -c:a pcm_s16le celebrate.wav
#   ffmpeg -i idle.mp3      -map_metadata -1 -ar 22050 -ac 1 -c:a pcm_s16le -t 30 idle.wav
pio run -t uploadfs      # writes this folder to the LittleFS partition
```

Expected files — **16-bit PCM WAV** (names set in
[`../include/config.h`](../include/config.h)). WAV is used, not MP3: the files
are preloaded into PSRAM and played from RAM, so there's no decode and no flash
access mid-playback — the only path that stays clean on the single-core S2.

| File          | Played when            |
|---------------|------------------------|
| `celebrate.wav` | on each button press (`AUDIO_FILE`) |
| `idle.wav`      | looped while idle (`IDLE_FILE`)     |

## Encoding

Use **16-bit PCM WAV, mono**. MP3 was tried and dropped — decoding can't keep up
in real-time on the single-core S2 (it crackles once the buffer drains), whereas
raw PCM from PSRAM has zero decode cost and stays clean. Convert with:

```bash
ffmpeg -i in.mp3 -map_metadata -1 -ar 22050 -ac 1 -c:a pcm_s16le out.wav
```

- `-ar 22050` — good quality, half the data of 44.1 kHz (lower to `16000` /
  `11025` for longer clips)
- `-ac 1` — mono (the MAX98357A is mono anyway)
- `-map_metadata -1` — strip tags

## Size

WAV is uncompressed, and the LittleFS partition is **~2 MB** (see
`../partitions_audio.csv`). At 22050 Hz mono that's ~44 KB/s, so ~**45 s total**
across both files — trim the idle loop (`-t 30`) or drop the sample rate if you
need more. Each clip is also loaded whole into PSRAM at boot, so both must fit in
the ~2 MB of PSRAM too (they will, if they fit the partition).
- The `.mp3` files are git-ignored (they can be large / personal); only this
  README is tracked.
