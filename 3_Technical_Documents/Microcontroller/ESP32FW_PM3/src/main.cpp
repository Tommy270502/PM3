#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

using namespace audio_tools;

// --------- WiFi CONFIG (EDIT THESE) ----------
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// --------- AUDIO OBJECTS ----------
I2SStream i2s;                      // I2S output stream
BluetoothA2DPSink a2dp_sink(i2s);   // BT sink → I2S

// ESP32 → STM32 I2S pins
static const int I2S_BCLK_PIN = 26;   // BCLK  -> STM32 BCLK
static const int I2S_LRCK_PIN = 25;   // LRCLK -> STM32 LRCLK
static const int I2S_DATA_OUT = 23;   // DOUT  -> STM32 DIN
// static const int I2S_DATA_IN = 22; // optional for RX

// --------- HELPER FUNCTIONS ----------
void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Simple blocking connect (you can add timeout if you want)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  // Optional: set a hostname for the board
  ArduinoOTA.setHostname("ESP32_A2DP_I2S");

  // Optional callbacks (nice for debugging)
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)  Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)    Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready. You should now see this ESP32 as a network port in the Arduino IDE.");
}

// --------- ARDUINO SETUP/LOOP ----------
void setup() {
  Serial.begin(115200);
  delay(300);

  // 1) WiFi + OTA
  setupWiFi();
  setupOTA();

  // 2) Configure I2S TX (ESP32 -> STM32)
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate     = 44100;   // A2DP = 44.1kHz
  cfg.bits_per_sample = 16;      // 16-bit stereo PCM
  cfg.channels        = 2;

  cfg.pin_bck  = I2S_BCLK_PIN;
  cfg.pin_ws   = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DATA_OUT;

  cfg.is_master = true;          // ESP32 generates BCLK + LRCLK

  i2s.begin(cfg);

  // 3) Start Bluetooth A2DP sink
  a2dp_sink.start("ESP32_to_STM_I2S");

  Serial.println("Bluetooth A2DP Sink + I2S + OTA initialized.");
}

void loop() {
  // Handle OTA requests
  ArduinoOTA.handle();

  // A2DP + I2S is handled internally by the libraries
  // Just keep loop running
}
