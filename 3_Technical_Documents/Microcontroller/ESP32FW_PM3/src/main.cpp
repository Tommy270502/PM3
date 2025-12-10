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
static const int I2S_BCLK_PIN = 26;
static const int I2S_LRCK_PIN = 25;
static const int I2S_DATA_OUT = 23;

// ESP32 → STM32 UART pins
static const int UART_RX = 21;
static const int UART_TX = 27;

HardwareSerial STM32Serial(2);

// ------------ WiFi + OTA helpers ------------
void setupWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  STM32Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    STM32Serial.print(".");
  }

  STM32Serial.println();
  STM32Serial.print("WiFi connected, IP address: ");
  STM32Serial.println(WiFi.localIP());

  STM32Serial.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
}

void setupOTA() {

  ArduinoOTA.setHostname("ESP32_A2DP_I2S");
  ArduinoOTA.setPort(3232);

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    STM32Serial.println("OTA Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    STM32Serial.println("OTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    STM32Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    STM32Serial.printf("OTA Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR)     STM32Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)    STM32Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)  STM32Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)  STM32Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)      STM32Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  STM32Serial.println("OTA Ready.");
}

// ------------ SETUP / LOOP ------------
void setup() {
  // No Serial.begin() → no USB Serial output
  STM32Serial.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  delay(500);

  STM32Serial.println("Booting ESP32 A2DP + I2S + OTA...");

  setupWiFi();
  setupOTA();

  // Configure I2S pins
  i2s_pin_config_t pin_config = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_OUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  a2dp_sink.set_pin_config(pin_config);

  // Start A2DP Bluetooth
  a2dp_sink.start("ESP_Receiver");

  STM32Serial.println("A2DP sink started. Pair and play audio.");
}

void loop() {
  ArduinoOTA.handle();

  // Print only to STM32
  STM32Serial.println("Hello");

  delay(1);
}
