#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>
#include <lwip/etharp.h>

#include <TimeLib.h>
#include <NTPClientLib.h>

#include "secrets.h"

#define LED_PIN 2
#define NUM_LEDS (39 + 64 + 39)
#define BRIGHTNESS 200
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGBArray<NUM_LEDS> leds;

const char *ssid = SSID;
const char *password = PASSWORD;

CRGBPalette16 currentPalette;
TBlendType currentBlending;

#include "html.html" // htmlTemplate

ESP8266WebServer server(80);

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
    Serial.begin(9600);
    EEPROM.begin(sizeof(struct configuration));
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    WiFi.mode(WIFI_STA);
    // WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(100);
    }

    EEPROM.get(0, savedConf);

    ArduinoOTA.setHostname("leds.cubox");

    ArduinoOTA.onStart([]() {
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.setBrightness(255);
        FastLED.show();
    });

    ArduinoOTA.begin();

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
    for (int i = NUM_LEDS-1; i >= 0; i--) {
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

void loop() {
    static uint8_t startIndex = 0;
    startIndex += 1; /* motion speed */

    loopConf = savedConf;
    
    if (toUpdateNextLoop) {
        toUpdate = true;
        toUpdateNextLoop = false;
    }

    time_t t = now();

    if (year() != 2021 && year() != 2022 && year() != 2023 && year() != 2024 && year() != 2025 && year() != 2026 && year() != 2027 && year() != 2028) {
        //rgb(&loopConf, 255, 0, 0);
        //loopConf.brightness = 255;
        //toUpdate = true;
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        goto end; // Hehe, fight me
    }

    if (hour(t) == 23 && minute(t) == 0 && second(t) == 0) { // 23h, set as full red, warning
        rgb(&loopConf, 255, 0, 0);
        loopConf.brightness = 255;
        toUpdate = true;
        goto end;
    } else if (hour(t) == 23 && minute(t) == 0 && second(t) == 2) { // two seconds later, dim the brightness.
        savedConf.brightness = 50;
    } else if (hour(t) == 8 && minute(t) == 0 && second(t) == 0) { // 7 am, lights off
        savedConf.brightness = 0;
    } else if (hour(t) == 17 && minute(t) == 0 && second(t) == 0) {
        if (savedConf.brightness < 100) {
            savedConf.brightness = EEPROM.read(offsetof(struct configuration, brightness));
            if (savedConf.brightness < 100) {
                savedConf.brightness = 100;
            }
        }
    }

    if (hour(t) >= 8 && minute(t) == 0 && second(t) <= 5) {
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
        modifiedBrightness = ::map(breathValue, 0, 255, 20, loopConf.brightness);
        FastLED.setBrightness(modifiedBrightness);
    }

end:
    if (toUpdate && !loopConf.rainbow) {
        leds(0, 3).fill_solid(CRGB(scale8(loopConf.red, 130), loopConf.green, loopConf.blue));
        leds(4, 129).fill_solid(CRGB(loopConf.red, loopConf.green, loopConf.blue));
        leds(130, 140).fill_solid(CRGB(scale8(loopConf.red, 130), scale8(loopConf.green, 150), loopConf.blue));
        leds(140, 141).fill_solid(CRGB(scale8(loopConf.red, 170), scale8(loopConf.green, 100), loopConf.blue));
    }

    if (toUpdate && !loopConf.breath) {
        FastLED.setBrightness(loopConf.brightness);
    }

    toUpdate = false; // DO NOT PUT AFTER HTTP HANDLING
    FastLED.show();
    ArduinoOTA.handle();
    server.handleClient();
}
