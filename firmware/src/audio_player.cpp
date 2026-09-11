#include <Arduino.h>
#include <LittleFS.h>

#include "driver/i2s.h"

#include "audio_player.h"
#include "config.h"

// Audio is 16-bit PCM WAV, preloaded into PSRAM at boot and fed to I2S from RAM.
// No MP3 decode and no flash access during playback — that's what keeps it clean
// on the single-core S2 (decoding or reading flash mid-playback starves I2S).
// Feeding is non-blocking so the motor and LEDs keep running during a celebration.

namespace {
const i2s_port_t I2S_PORT = I2S_NUM_0;

struct Clip {
  int16_t* pcm = nullptr;  // interleaved samples in PSRAM
  uint32_t frames = 0;     // number of sample frames
  uint16_t channels = 1;
  uint32_t rate = 22050;
  bool ok = false;
};
Clip idleClip, celebClip;

enum Mode { OFF, IDLE, CELEBRATION };
Mode mode = OFF;
Clip* cur = nullptr;
uint32_t frameIdx = 0;
bool loopCur = false;
int curVol = 15;           // 0..21
uint32_t curRate = 0;
bool celebEnded = false;
bool i2sReady = false;

// ---- WAV loading -----------------------------------------------------------
bool loadWav(const char* path, Clip& c) {
  File f = LittleFS.open(path, "r");
  if (!f || f.size() < 44) {
    Serial.printf("audio: %s missing\n", path);
    return false;
  }
  uint8_t hdr[12];
  f.read(hdr, 12);
  if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) return false;

  uint16_t bits = 0;
  uint32_t dataStart = 0, dataBytes = 0, pos = 12;
  while (pos + 8 <= f.size()) {
    f.seek(pos);
    char id[4];
    uint32_t sz = 0;
    f.read((uint8_t*)id, 4);
    f.read((uint8_t*)&sz, 4);
    const uint32_t body = pos + 8;
    if (!memcmp(id, "fmt ", 4)) {
      uint8_t x[16];
      f.read(x, 16);
      c.channels = x[2] | (x[3] << 8);
      c.rate = x[4] | (x[5] << 8) | (x[6] << 16) | ((uint32_t)x[7] << 24);
      bits = x[14] | (x[15] << 8);
    } else if (!memcmp(id, "data", 4)) {
      dataStart = body;
      dataBytes = sz;
      break;
    }
    pos = body + sz + (sz & 1);
  }
  if (bits != 16 || dataStart == 0 || (c.channels != 1 && c.channels != 2))
    return false;

  c.pcm = (int16_t*)ps_malloc(dataBytes);
  if (!c.pcm) {
    Serial.printf("audio: ps_malloc(%u) failed for %s\n", dataBytes, path);
    return false;
  }
  f.seek(dataStart);
  uint32_t rd = 0;
  while (rd < dataBytes) {
    int n = f.read((uint8_t*)c.pcm + rd, min((uint32_t)8192, dataBytes - rd));
    if (n <= 0) break;
    rd += n;
  }
  f.close();
  c.frames = rd / (2 * c.channels);
  c.ok = true;
  Serial.printf("audio: loaded %s (%u Hz, %u ch, %u frames)\n", path, c.rate,
                c.channels, c.frames);
  return true;
}

void installI2S(uint32_t rate) {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = rate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;   // 8*256 = ~93 ms cushion at 22 kHz
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = PIN_I2S_BCLK;
  pins.ws_io_num = PIN_I2S_LRC;
  pins.data_out_num = PIN_I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
  curRate = rate;
  i2sReady = true;
}

void startClip(Clip& c, int vol, bool loopIt) {
  if (!c.ok) return;
  if (!i2sReady) {
    installI2S(c.rate);
  } else if (curRate != c.rate) {
    i2s_set_clk(I2S_PORT, c.rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    curRate = c.rate;
  }
  cur = &c;
  frameIdx = 0;
  loopCur = loopIt;
  curVol = vol;
  celebEnded = false;
}

inline int16_t frameSample(const Clip& c, uint32_t fi) {
  if (c.channels == 1) return c.pcm[fi];
  return (int16_t)(((int32_t)c.pcm[2 * fi] + c.pcm[2 * fi + 1]) / 2);  // downmix
}
}  // namespace

void audioplayer::begin() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount FAILED — run `pio run -t uploadfs`."));
  }
  loadWav(IDLE_FILE, idleClip);
  loadWav(AUDIO_FILE, celebClip);
  const uint32_t r =
      idleClip.ok ? idleClip.rate : (celebClip.ok ? celebClip.rate : 22050);
  installI2S(r);
}

void audioplayer::playIdle() {
  if (!ENABLE_IDLE_AUDIO || !idleClip.ok) {
    mode = OFF;
    cur = nullptr;
    return;
  }
  mode = IDLE;
  startClip(idleClip, IDLE_VOLUME, true);
}

void audioplayer::playCelebration() {
  if (!celebClip.ok) {
    mode = OFF;
    cur = nullptr;
    return;
  }
  mode = CELEBRATION;
  startClip(celebClip, CELEBRATION_VOLUME, false);
}

void audioplayer::update() {
  if (mode == OFF || cur == nullptr || !cur->ok) return;

  static const int FRAMES = 128;
  int16_t out[FRAMES * 2];

  // Build a chunk from the current position (peek — don't commit until written).
  uint32_t t = frameIdx;
  int built = 0;
  for (; built < FRAMES; built++) {
    if (t >= cur->frames) {
      if (loopCur)
        t = 0;
      else
        break;
    }
    int32_t s = (int32_t)frameSample(*cur, t) * curVol / 21;
    out[2 * built] = (int16_t)s;
    out[2 * built + 1] = (int16_t)s;
    t++;
  }
  if (built == 0) {  // one-shot finished feeding
    celebEnded = true;
    return;
  }

  size_t written = 0;
  i2s_write(I2S_PORT, out, built * 4, &written, 0);  // non-blocking
  const uint32_t wf = written / 4;
  frameIdx += wf;
  if (loopCur) {
    if (cur->frames) frameIdx %= cur->frames;
  } else if (frameIdx >= cur->frames) {
    celebEnded = true;
  }
}

bool audioplayer::isCelebrationPlaying() {
  return mode == CELEBRATION && !celebEnded;
}
