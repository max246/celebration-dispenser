// ===========================================================================
//  I2S tone test — Celebration Dispenser
//
//  The definitive audio-path check: generates a clean 440 Hz sine wave straight
//  to the MAX98357A over I2S. NO WiFi, NO streaming, NO MP3 decode, NO audio
//  library — just the raw I2S hardware.
//
//      pio run -e tonetest -t upload
//      pio device monitor -e tonetest      (to see the banner)
//
//  If the tone is CLEAN  -> wiring/amp/speaker are fine; the crackle is from
//                           streaming/decode on the single-core S2.
//  If the tone is SCRATCHY -> it's hardware (I2S wiring / power / speaker).
//
//  I2S pins MUST match config.h.
// ===========================================================================
#include <Arduino.h>
#include <math.h>

#include "driver/i2s.h"

static const int PIN_I2S_BCLK = 36;   // "SCK"
static const int PIN_I2S_LRC  = 35;   // "MO"
static const int PIN_I2S_DOUT = 37;   // "MI"

static const int SAMPLE_RATE = 44100;
static const i2s_port_t I2S_PORT = I2S_NUM_0;

void setup() {
  Serial.begin(115200);
  delay(5000);  // time to attach the serial monitor
  Serial.println(F("\n=== I2S tone test — clean 440 Hz, no WiFi/decode ==="));
  Serial.println(F("Clean tone => wiring OK (crackle is from streaming)."));
  Serial.println(F("Scratchy tone => hardware (I2S wiring / power / speaker)."));

  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = PIN_I2S_BCLK;
  pins.ws_io_num = PIN_I2S_LRC;
  pins.data_out_num = PIN_I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
  Serial.println(F("Playing 440 Hz..."));
}

void loop() {
  static const int N = 256;
  static float phase = 0.0f;
  const float inc = 2.0f * (float)M_PI * 440.0f / SAMPLE_RATE;

  int16_t frame[N * 2];  // interleaved L/R
  for (int i = 0; i < N; i++) {
    const int16_t s = (int16_t)(3000.0f * sinf(phase));  // modest amplitude
    phase += inc;
    if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
    frame[2 * i] = s;
    frame[2 * i + 1] = s;
  }
  size_t written = 0;
  i2s_write(I2S_PORT, frame, sizeof(frame), &written, portMAX_DELAY);
}
