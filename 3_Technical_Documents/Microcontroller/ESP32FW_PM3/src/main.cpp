#include <Arduino.h>

// AudioTools must be included before BluetoothA2DPSink
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

using namespace audio_tools;

// I2S output stream
I2SStream i2s;

// A2DP sink feeding the I2S stream
BluetoothA2DPSink a2dp_sink(i2s);

// ------ Pin config: ESP32 -> CS4271 (standalone) ------
static const int I2S_BCLK_PIN = 26;   // to CS4271 SCLK
static const int I2S_LRCK_PIN = 25;   // to CS4271 LRCK
static const int I2S_DATA_PIN = 27;   // to CS4271 SDIN (DAC input)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("ESP32 Bluetooth A2DP -> CS4271 (standalone) via I2S");

  // Optional: AudioTools logging
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // ---- Configure I2S (ESP32 side) ----
  auto cfg = i2s.defaultConfig();   // default is "output" mode
  cfg.sample_rate     = 44100;      // A2DP usually 44.1 kHz
  cfg.bits_per_sample = 16;         // A2DP PCM is 16-bit stereo
  cfg.channels        = 2;
  cfg.pin_bck         = I2S_BCLK_PIN;
  cfg.pin_ws          = I2S_LRCK_PIN;
  cfg.pin_data        = I2S_DATA_PIN;
  cfg.is_master       = true;       // ESP32 provides BCLK/LRCK

  i2s.begin(cfg);

  // ---- Start Bluetooth A2DP sink ----
  a2dp_sink.start("ESP32_CS4271_Speaker");

  Serial.println("A2DP sink running. Pair to 'ESP32_CS4271_Speaker' and play music.");
}

void loop() {
  // All streaming is handled in background tasks
}
