#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RemoteDebug.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#include "secrets.h"

RemoteDebug Debug;
Adafruit_ADS1115 ads;

const double multiplier = 0.0625;
const double FACTOR = 50;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  });
  ArduinoOTA.onEnd([]() {
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
    
  });
  ArduinoOTA.begin();
  Debug.begin("Telnet_HostName");
  NTP.begin("europe.pool.ntp.org", 1, true, 0);
  ads.setGain(GAIN_TWO);
  ads.begin();
}

double getCurrent() {
  long t = millis();
  long rawAdc = ads.readADC_Differential_0_1();
  long minRaw = rawAdc;
  long maxRaw = rawAdc;
  while (millis() - t < 1000) {
    rawAdc = ads.readADC_Differential_0_1();
    maxRaw = maxRaw > rawAdc ? maxRaw : rawAdc;
    minRaw = minRaw < rawAdc ? minRaw : rawAdc;
  }

  maxRaw = maxRaw > minRaw ? maxRaw : -minRaw;
  double voltagePeak = maxRaw * multiplier / 1000;
  double voltageRMS = voltagePeak * 0.70710678118;
  double currentRMS = voltageRMS * FACTOR;
  return currentRMS;
}

void loop() {
  double currentRMS = getCurrent();
  double power = 230 * currentRMS;

  Debug.printf("Current is: %fA, which makes power: %fW\n", currentRMS, power);

  ArduinoOTA.handle();
  Debug.handle();
}