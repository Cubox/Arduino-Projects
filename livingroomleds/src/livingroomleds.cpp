#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#define MAX_TIME_INACTIVE -1 // Disable timeout feature
#include <RemoteDebug.h>
#include <WiFiUdp.h>
//#define FASTLED_INTERRUPT_RETRY_COUNT 1
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#include <TimeLib.h>
#include <NTPClientLib.h>

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

#include "html.html" // htmlTemplate

ESP8266WebServer server(80);
RemoteDebug Debug;

struct configuration {
    uint8_t brightness;
    uint8_t red, green, blue;
    bool rainbow, epilepsy, breath;
};

bool toUpdate = true;

struct configuration savedConf;
struct configuration loopConf;

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
    sprintf_P(bufferHtml, htmlTemplate, bufferCss, savedConf.red, savedConf.green, savedConf.blue, savedConf.brightness);
    server.send(200, "text/html", bufferHtml);
    free(bufferHtml);
    free(bufferCss);
}

void handleIndexPost() {
    if (server.arg("red") != "same") {
        savedConf.red = server.arg("red").toInt();
    }
    if (server.arg("green") != "same") {
        savedConf.green = server.arg("green").toInt();
    }
    if (server.arg("blue") != "same") {
        savedConf.blue = server.arg("blue").toInt();
    }
    if (server.arg("brightness") != "same") {
        savedConf.brightness = server.arg("brightness").toInt();
    }

    if (server.arg("rainbow") != "") {
        savedConf.rainbow = true;
        savedConf.epilepsy = false;
        savedConf.breath = false;
    } else if (server.arg("epilepsy") != "") {
        savedConf.epilepsy = true;
        savedConf.rainbow = false;
        savedConf.breath = false;
    } else if (server.arg("breath") != "") {
        savedConf.breath = true;
        savedConf.rainbow = false;
        savedConf.epilepsy = false;
    } else {
        savedConf.epilepsy = false;
        savedConf.rainbow = false;
        savedConf.breath = false;
    }

    EEPROM.put(0, savedConf);

    EEPROM.commit();
    toUpdate = true;

    redirect303("/");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    EEPROM.begin(sizeof(struct configuration));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(1000);
        ESP.restart();
    }

    EEPROM.get(0, savedConf);

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

    Debug.begin("Telnet_HostName");
    NTP.begin("europe.pool.ntp.org", 1, true, 0);

    server.onNotFound(onMissing);
    server.on("/", HTTP_GET, handleIndexGet);
    server.on("/", HTTP_POST, handleIndexPost);
    server.begin();

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(savedConf.brightness);

    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette(currentPalette, colorIndex, loopConf.brightness, currentBlending);
        colorIndex += 3;
    }
}

void rgb(struct configuration *conf, int red, int green, int blue) {
    conf->red = red;
    conf->green = green;
    conf->blue = blue;
}

bool toUpdateNextLoop = false;

// long lastMillis = 0;
// long loops = 0;

void loop() {
    // long currentMillis = millis();
    // loops++;
    // if (currentMillis - lastMillis > 1000){
    //     Debug.print("Loops last second: ");
    //     Debug.println(loops);
    
    //     lastMillis = currentMillis;
    //     loops = 0;
    // }
    static uint8_t startIndex = 0;
    startIndex += 1; /* motion speed */

    loopConf = savedConf;
    
    if (toUpdateNextLoop) {
        toUpdate = true;
        toUpdateNextLoop = false;
    }

    time_t t = now();
    if (hour(t) == 0 && minute(t) == 0 && second(t) == 0) { // midnight, set as full red, warning
        rgb(&loopConf, 255, 0, 0);
        loopConf.brightness = 255;
        toUpdate = true;
    } else if (hour(t) == 0 && minute(t) == 0 && second(t) == 2) { // two seconds later, dim the brightness. Set sensible colours
        savedConf.brightness = 50;
        toUpdate = true;
    } else if (hour(t) == 7 && minute(t) == 0 && second(t) == 0) { // 7 am, lights off
        savedConf.brightness = 0;
    } else if (minute(t) == 0 && second(t) <= 5) {
        loopConf.rainbow = true;
        loopConf.breath = false;
        loopConf.epilepsy = false;
        loopConf.brightness = 255;
        toUpdateNextLoop = true;
    }

    if (loopConf.rainbow) {
        FillLEDsFromPaletteColors(startIndex);
    } else if (loopConf.epilepsy) {
        FastLED.delay(20);
        FastLED.setBrightness(0);
        FastLED.show();
        FastLED.delay(20);
        startIndex += 1;
    } else if (loopConf.breath) {
        double breathValue, modifiedBrightness;
        breathValue = (exp(sin(millis()/2000.0*PI))-0.36787944)*108.0;
        modifiedBrightness = map(breathValue, 0, 255, 0, loopConf.brightness);
        FastLED.setBrightness(modifiedBrightness);
    }

    if (toUpdate && !loopConf.rainbow) {
        fill_solid(leds, NUM_LEDS, CRGB(loopConf.red, loopConf.green, loopConf.blue));
    }

    if (!loopConf.breath) {
        FastLED.setBrightness(loopConf.brightness);
    }

    toUpdate = false; // DO NOT PUT AFTER HTTP HANDLING
    FastLED.show();
    ArduinoOTA.handle();
    server.handleClient();
    Debug.handle();
}
