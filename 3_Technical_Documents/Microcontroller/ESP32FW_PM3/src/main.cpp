#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

using namespace audio_tools;

// Create I2S output stream
I2SStream i2s;

// Connect BT sink → I2S output
BluetoothA2DPSink a2dp_sink(i2s);

// ESP32 → STM32 I2S pins
static const int I2S_BCLK_PIN = 26;   // BCLK -> STM32 BCLK
static const int I2S_LRCK_PIN = 25;   // LRCLK -> STM32 LRCLK
static const int I2S_DATA_OUT = 23;   // ESP32 DOUT -> STM32 DIN
// static const int I2S_DATA_IN = 22; // (optional if STM32 sends back audio)

void setup() {
  Serial.begin(115200);
  delay(300);

  // ---- Configure I2S TX (ESP32 -> STM32) ----
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate     = 44100;   // A2DP = 44.1kHz
  cfg.bits_per_sample = 16;      // A2DP output is 16-bit stereo PCM
  cfg.channels        = 2;

  cfg.pin_bck  = I2S_BCLK_PIN;
  cfg.pin_ws   = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DATA_OUT;

  cfg.is_master = true;          // ESP32 generates BCLK + LRCLK

  i2s.begin(cfg);

  // ---- Start Bluetooth A2DP sink ----
  a2dp_sink.start("ESP32_to_STM_I2S");
}

void loop() {
  // A2DP + I2S handled internally
}
