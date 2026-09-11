// ===========================================================================
//  WAV player test — Celebration Dispenser
//
//  Plays a WAV file from LittleFS as RAW PCM straight to I2S — NO MP3 decode,
//  NO audio library, NO WiFi. Same clean path as the tone test, just reading
//  samples from a file. This is the reliable audio route for the single-core S2.
//
//      # make a mono 16-bit WAV, e.g.:
//      #   ffmpeg -i in.mp3 -map_metadata -1 -ar 22050 -ac 1 -c:a pcm_s16le data/idle.wav
//      pio run -e wavtest -t upload        # flash test + partition table
//      pio run -e wavtest -t uploadfs      # upload data/ (idle.wav)
//      pio device monitor -e wavtest
//
//  Loops WAV_FILE forever. Expects 16-bit PCM WAV (mono or stereo). I2S pins
//  match config.h.
// ===========================================================================
#include <Arduino.h>
#include <LittleFS.h>

#include "driver/i2s.h"

static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const char* WAV_FILE = "/idle.wav";
static const int   VOL_SHIFT = 1;     // >>1 = ~half amplitude (avoid clipping)
static const i2s_port_t I2S_PORT = I2S_NUM_0;

File wav;
uint32_t dataStart = 0, dataEnd = 0;
uint16_t channels = 1, bits = 16;
uint32_t sampleRate = 22050;

// The entire PCM payload is loaded into PSRAM so playback never touches flash
// (reading flash during playback stalls the CPU -> I2S underruns -> crackle).
int16_t* pcm = nullptr;   // interleaved samples in PSRAM
uint32_t pcmSamples = 0;  // total int16 samples (frames * channels)

static bool parseWav() {
  if (wav.size() < 44) return false;
  wav.seek(0);
  uint8_t hdr[12];
  wav.read(hdr, 12);
  if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) return false;

  uint32_t pos = 12;
  bool fmtOk = false, dataOk = false;
  while (pos + 8 <= wav.size()) {
    wav.seek(pos);
    char id[4];
    uint32_t sz = 0;
    wav.read((uint8_t*)id, 4);
    wav.read((uint8_t*)&sz, 4);
    const uint32_t body = pos + 8;
    if (!memcmp(id, "fmt ", 4)) {
      uint8_t f[16];
      wav.read(f, 16);
      channels = f[2] | (f[3] << 8);
      sampleRate = f[4] | (f[5] << 8) | (f[6] << 16) | ((uint32_t)f[7] << 24);
      bits = f[14] | (f[15] << 8);
      fmtOk = true;
    } else if (!memcmp(id, "data", 4)) {
      dataStart = body;
      dataEnd = body + sz;
      dataOk = true;
      break;
    }
    pos = body + sz + (sz & 1);  // chunks are word-aligned
  }
  return fmtOk && dataOk && bits == 16 && (channels == 1 || channels == 2);
}

static void setupI2S(uint32_t sr) {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sr;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
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
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println(F("\n=== WAV player test (raw PCM -> I2S, no decode) ==="));

  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount FAILED — run `pio run -e wavtest -t uploadfs`."));
    return;
  }
  wav = LittleFS.open(WAV_FILE, "r");
  if (!wav) {
    Serial.printf("open %s FAILED\n", WAV_FILE);
    return;
  }
  if (!parseWav()) {
    Serial.println(F("Not a supported 16-bit PCM WAV."));
    return;
  }
  const uint32_t dataBytes = dataEnd - dataStart;
  Serial.printf("WAV: %u Hz, %u ch, %u-bit, %u bytes of audio\n",
                sampleRate, channels, bits, dataBytes);

  // Load the whole payload into PSRAM.
  pcm = (int16_t*)ps_malloc(dataBytes);
  if (!pcm) {
    Serial.printf("ps_malloc(%u) FAILED — PSRAM missing or file too big.\n", dataBytes);
    return;
  }
  wav.seek(dataStart);
  uint32_t rd = 0;
  while (rd < dataBytes) {
    int n = wav.read((uint8_t*)pcm + rd, min((uint32_t)8192, dataBytes - rd));
    if (n <= 0) break;
    rd += n;
  }
  wav.close();
  pcmSamples = rd / 2;
  Serial.printf("Loaded %u bytes into PSRAM. Looping from RAM — should be clean.\n", rd);

  setupI2S(sampleRate);
}

void loop() {
  if (!pcm) return;

  static const int FRAMES = 256;
  static uint32_t idx = 0;  // sample index into pcm[]
  int16_t out[FRAMES * 2];  // interleaved L/R to I2S

  for (int i = 0; i < FRAMES; i++) {
    int16_t l, r;
    if (channels == 1) {
      if (idx >= pcmSamples) idx = 0;  // loop
      l = r = pcm[idx++] >> VOL_SHIFT;
    } else {
      if (idx + 1 >= pcmSamples) idx = 0;
      l = pcm[idx++] >> VOL_SHIFT;
      r = pcm[idx++] >> VOL_SHIFT;
    }
    out[2 * i] = l;
    out[2 * i + 1] = r;
  }
  size_t written = 0;
  i2s_write(I2S_PORT, out, sizeof(out), &written, portMAX_DELAY);
}
