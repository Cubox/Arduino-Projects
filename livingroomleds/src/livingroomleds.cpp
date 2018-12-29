#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <RemoteDebug.h>
#include <WiFiUdp.h>
//#define FASTLED_INTERRUPT_RETRY_COUNT 1
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include "secrets.h"

#define LED_PIN 2
#define NUM_LEDS (39 + 64 + 39)
#define BRIGHTNESS 200
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

const char *ssid = SSID;
const char *password = PASSWORD;

CRGBPalette16 currentPalette;
TBlendType currentBlending;

byte red;
byte green;
byte blue;
byte brightness;
bool rainbow = false;
bool epilepsy = false;
bool breath = false;
bool toUpdate = true;

#include "html.html" // htmlTemplate

ESP8266WebServer server(80);
//RemoteDebug Debug;

void redirect303(const String &url) {
    server.sendHeader("Location", url);
    server.send(303);
}

void onMissing() {
    // Handle Unknown Request
    server.send(404, "text/html", "Not found");
}

void handleIndexGet() {
    char *bufferHtml = (char *)malloc(sizeof(htmlTemplate) + sizeof(htmlTemplateCSS) + 42);
    char *bufferCss = (char *)malloc(sizeof(htmlTemplateCSS));
    memset(bufferHtml, 0, sizeof(htmlTemplate) + 42);
    memset(bufferCss, 0, sizeof(htmlTemplateCSS));
    strcpy_P(bufferCss, htmlTemplateCSS);
    sprintf_P(bufferHtml, htmlTemplate, bufferCss, red, green, blue, brightness);
    server.send(200, "text/html", bufferHtml);
    free(bufferHtml);
    free(bufferCss);
}

void handleIndexPost() {
    if (server.arg("red") != "same") {
        red = server.arg("red").toInt();
        EEPROM.write(0, red);
    }
    if (server.arg("green") != "same") {
        green = server.arg("green").toInt();
        EEPROM.write(1, green);
    }
    if (server.arg("blue") != "same") {
        blue = server.arg("blue").toInt();
        EEPROM.write(2, blue);
    }
    if (server.arg("brightness") != "same") {
        brightness = server.arg("brightness").toInt();
        EEPROM.write(3, brightness);
    }

    if (server.arg("rainbow") != "") {
        rainbow = true;
        epilepsy = false;
        breath = false;
    } else if (server.arg("epilepsy") != "") {
        epilepsy = true;
        rainbow = false;
        breath = false;
    } else if (server.arg("breath") != "") {
        breath = true;
        rainbow = false;
        epilepsy = false;
    } else {
        epilepsy = false;
        rainbow = false;
        breath = false;
    }
    EEPROM.write(4, rainbow);
    EEPROM.write(5, breath);

    EEPROM.commit();
    toUpdate = true;

    redirect303("/");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    EEPROM.begin(6);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(1000);
        ESP.restart();
    }

    red = EEPROM.read(0);
    green = EEPROM.read(1);
    blue = EEPROM.read(2);
    brightness = EEPROM.read(3); // Load from EEPROM here
    rainbow = EEPROM.read(4);
    breath = EEPROM.read(5);

    ArduinoOTA.setHostname("Leds");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // SPIFFS.end();
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    //Debug.begin("Telnet_HostName");

    server.onNotFound(onMissing);
    server.on("/", HTTP_GET, handleIndexGet);
    server.on("/", HTTP_POST, handleIndexPost);
    server.begin();

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);

    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

unsigned long previousMillis = 0;

void loop() {
    static uint8_t startIndex = 0;
    startIndex += 1; /* motion speed */

/*     // Hardcoded mode
    toUpdate = true;
    breath = false;
    red = 255;
    green = 0;
    blue = 255;
    rainbow = false;
    epilepsy = false;
    brightness = 100; */

    if (rainbow) {
        FillLEDsFromPaletteColors(startIndex);
    } if (epilepsy) {
        FastLED.delay(20);
        FastLED.setBrightness(0);
        FastLED.show();
        FastLED.delay(20);
        startIndex += 1;
    } if (breath) {
        double breathValue, modifiedBrightness;
        breathValue = (exp(sin(millis()/2000.0*PI))-0.36787944)*108.0;
        modifiedBrightness = map(breathValue, 0, 255, 0, brightness);
        FastLED.setBrightness(modifiedBrightness);
    } if (toUpdate) {
        fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
        toUpdate = false;
    }
    if (!breath) {
        FastLED.setBrightness(brightness);
    }
    FastLED.show();
    ArduinoOTA.handle();
    server.handleClient();
}
