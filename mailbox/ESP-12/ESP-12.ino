#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

const char* ssid     = "";
const char* password = "";
#define SERVERHOST ""
#define SERVERPORT 6534
WiFiClient client;

void reportError() {
  Serial.println("Error"); // error
  WiFi.disconnect();
  ESP.restart();
}

void setup() {
  Serial.begin(9600);
  delay(10);

  // We start by connecting to a WiFi network
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("wifi error"); // wifi error
    delay(5000);
    ESP.restart();
  }

  Serial.println("Ready"); // ready  

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp-mailbox");

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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  if (client.connect(SERVERHOST, SERVERPORT)) {
    Serial.println("Connected"); // success, ready to transmit data
  } else {
    // We could not connect to the server, telling Arduino to stand down
    reportError();
  }
}

void loop() {
  ArduinoOTA.handle();
  if (Serial.available()) {
    if (!client.connected()) {
      // error, we lost the TCP stream
      reportError();
    }

    char buf[256];
    if (Serial.readBytesUntil('\0', buf, 256) <= 0) {
      reportError();
    } else {
      client.write(buf);
      client.write("\0");
      Serial.println("Sent"); // we sent the data with success
    }
  }
}

