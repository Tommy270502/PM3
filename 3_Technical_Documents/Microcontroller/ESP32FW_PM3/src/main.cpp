#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "BluetoothA2DPSink.h"

// ------------ WiFi CONFIG ------------
const char* WIFI_SSID = "Leck";
const char* WIFI_PASS = "nTR6E6enn4h2JS9FXq";

// ------------ A2DP + I2S ------------
BluetoothA2DPSink a2dp_sink;

// ESP32 → STM32 I2S pins
static const int I2S_BCLK_PIN = 26;   // BCLK
static const int I2S_LRCK_PIN = 25;   // LRCLK
static const int I2S_DATA_OUT = 23;   // DOUT

// ------------ WiFi + OTA helpers ------------
void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  // Optional: hostname (also handy for upload_port = ESP32_A2DP_I2S.local)
  ArduinoOTA.setHostname("ESP32_A2DP_I2S");
  ArduinoOTA.setPort(3232);  // default is 3232, but explicit is fine

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
    if (error == OTA_AUTH_ERROR)        Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)  Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)    Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready.");
}

// ------------ SETUP / LOOP ------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Booting ESP32 A2DP + I2S + OTA...");

  // 1) WiFi + OTA
  setupWiFi();
  setupOTA();

  // 2) Configure I2S pins for A2DP sink (legacy I2S)
  i2s_pin_config_t pin_config = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_OUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  a2dp_sink.set_pin_config(pin_config);

  // 3) Start Bluetooth A2DP sink
  a2dp_sink.start("ESP32_to_STM_I2S");

  Serial.println("A2DP sink started. Pair and play audio. OTA also active. works");
}

void loop() {
  // Handle OTA requests
  ArduinoOTA.handle();

  // A2DP runs in its own tasks; nothing else needed here
  // but keep loop responsive:
  delay(1);
}
