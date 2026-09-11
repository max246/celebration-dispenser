// ===========================================================================
//  WAV player test — Celebration Dispenser
//
//  Plays BOTH WAV files from PSRAM (no MP3 decode, no library, no WiFi, no flash
//  access during playback — the clean path for the single-core S2). Loops
//  idle.wav and fires celebrate.wav every ~12 s, like a button press.
//
//      # put celebrate.wav + idle.wav in firmware/data/ first (mono 16-bit)
//      pio run -e wavtest -t upload        # flash test + partition table
//      pio run -e wavtest -t uploadfs      # upload the WAVs
//      pio device monitor -e wavtest
//
//  I2S pins match config.h.
// ===========================================================================
#include <Arduino.h>
#include <LittleFS.h>

#include "driver/i2s.h"

static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const int VOL_SHIFT = 1;                 // >>1 ~= half amplitude
static const unsigned long CELEB_EVERY_MS = 12000;
static const i2s_port_t I2S_PORT = I2S_NUM_0;

struct Clip {
  int16_t* pcm = nullptr;
  uint32_t frames = 0;
  uint16_t channels = 1;
  uint32_t rate = 22050;
  bool ok = false;
};
Clip idleC, celebC;

Clip* cur = nullptr;
uint32_t fidx = 0;
bool loopCur = true;
uint32_t curRate = 0;
unsigned long lastCeleb = 0;

static bool loadClip(const char* path, Clip& c) {
  File f = LittleFS.open(path, "r");
  if (!f || f.size() < 44) { Serial.printf("  %s missing\n", path); return false; }
  uint8_t hdr[12];
  f.read(hdr, 12);
  if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) return false;

  uint16_t bits = 0;
  uint32_t dataStart = 0, dataBytes = 0, pos = 12;
  while (pos + 8 <= f.size()) {
    f.seek(pos);
    char id[4]; uint32_t sz = 0;
    f.read((uint8_t*)id, 4); f.read((uint8_t*)&sz, 4);
    const uint32_t body = pos + 8;
    if (!memcmp(id, "fmt ", 4)) {
      uint8_t x[16]; f.read(x, 16);
      c.channels = x[2] | (x[3] << 8);
      c.rate = x[4] | (x[5] << 8) | (x[6] << 16) | ((uint32_t)x[7] << 24);
      bits = x[14] | (x[15] << 8);
    } else if (!memcmp(id, "data", 4)) {
      dataStart = body; dataBytes = sz; break;
    }
    pos = body + sz + (sz & 1);
  }
  if (bits != 16 || !dataStart || (c.channels != 1 && c.channels != 2)) return false;

  c.pcm = (int16_t*)ps_malloc(dataBytes);
  if (!c.pcm) { Serial.printf("  ps_malloc(%u) failed\n", dataBytes); return false; }
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
  Serial.printf("  loaded %s: %u Hz, %u ch, %u frames\n", path, c.rate, c.channels, c.frames);
  return true;
}

static void installI2S(uint32_t rate) {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = rate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
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
}

static inline int16_t frameSample(const Clip& c, uint32_t fi) {
  if (c.channels == 1) return c.pcm[fi];
  return (int16_t)(((int32_t)c.pcm[2 * fi] + c.pcm[2 * fi + 1]) / 2);
}

static void startClip(Clip& c, bool loopIt) {
  if (curRate != c.rate) { i2s_set_clk(I2S_PORT, c.rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO); curRate = c.rate; }
  cur = &c; fidx = 0; loopCur = loopIt;
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println(F("\n=== WAV player test (both files, from PSRAM) ==="));
  if (!LittleFS.begin()) { Serial.println(F("LittleFS mount FAILED — uploadfs first.")); return; }
  Serial.println(F("Loading:"));
  loadClip("/idle.wav", idleC);
  loadClip("/celebrate.wav", celebC);
  if (!idleC.ok && !celebC.ok) { Serial.println(F("No playable WAVs.")); return; }

  installI2S(idleC.ok ? idleC.rate : celebC.rate);
  startClip(idleC.ok ? idleC : celebC, idleC.ok);  // loop idle if present
  lastCeleb = millis();
  Serial.println(F("Looping idle; celebration every ~12 s."));
}

void loop() {
  if (!cur) return;

  // fire the celebration clip periodically (like a button press)
  if (cur == &idleC && celebC.ok && millis() - lastCeleb > CELEB_EVERY_MS) {
    Serial.println(F("[test] -> celebrate"));
    startClip(celebC, false);
  }

  static const int FRAMES = 256;
  int16_t out[FRAMES * 2];
  uint32_t t = fidx;
  int built = 0;
  bool ended = false;
  for (; built < FRAMES; built++) {
    if (t >= cur->frames) { if (loopCur) t = 0; else { ended = true; break; } }
    const int16_t s = frameSample(*cur, t) >> VOL_SHIFT;
    out[2 * built] = s;
    out[2 * built + 1] = s;
    t++;
  }
  if (built > 0) {
    size_t w = 0;
    i2s_write(I2S_PORT, out, built * 4, &w, portMAX_DELAY);
  }
  fidx = t;

  if (ended) {  // celebration finished -> back to idle
    Serial.println(F("[test] -> idle loop"));
    startClip(idleC.ok ? idleC : celebC, idleC.ok);
    lastCeleb = millis();
  }
}
