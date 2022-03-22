#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HttpClient.h>
#include <Wire.h>
#include "secrets.h"

#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include <GxEPD2_3C.h> // including both doesn't use more code or ram
#include "GxEPD2_display_selection_new_style.h"
#include <Fonts/FreeMonoBold18pt7b.h>

const char *ssid = SSID;
const char *password = PASSWORD;

const char Text[] = "1111 PPM";

uint16_t x;
uint16_t y;

const IPAddress raven(192, 186, 0, 4);

void setup(void) {
    Serial.begin(9600);
    Serial.println();
    display.init();
    display.setRotation(3);
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(Text, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center the bounding box by transposition of the origin:
    x = ((display.width() - tbw) / 2) - tbx;
    y = ((display.height() - tbh) / 2) - tby;
    WiFi.setPhyMode(WIFI_PHY_MODE_11N);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(100);
    }
    WiFi.hostname("Co2Screen");
}

void loop(void) {
    if (WiFi.status() == WL_CONNECTED) {
        if (Ping.ping("192.168.0.42") && Ping.ping("192.168.0.5")) {
            WiFiClientSecure client;
            HTTPClient http;
            client.setInsecure();
            http.begin(client, "https://cubox.dev/files/co2");
            if (http.GET() == 200) {
                char buf[32];
                strcpy(buf, http.getString().c_str());
                char *newline = strchr(buf, '\n');
                if (newline != NULL) {
                    *newline = 0;
                }
                if (strlen(buf) == 0) {
                    strcpy(buf, "ERROR");
                } else {
                    strcat(buf, " PPM");
                }
                Serial.println(buf);
                display.setFullWindow();
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                    display.setCursor(x, y);
                    display.print(buf);
                } while (display.nextPage());
                http.end();
            }
        }
    }
    delay(1000 * 60 * 5);
}
