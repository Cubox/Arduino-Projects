#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HttpClient.h>
#include "secrets.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const char *ssid = SSID;
const char *password = PASSWORD;

U8G2_SH1107_64X128_F_HW_I2C u8g2(U8G2_R1);

void setup(void) {
    randomSeed(analogRead(0));
    Serial.begin(9600);
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
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
            strcat(buf, " PPM");
            Serial.println(buf);
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_crox5hb_tr);	// choose a suitable font
            u8g2.drawStr(0+random(10), 30+random(20), buf);	// write something to the internal memory
            u8g2.setContrast(0);
            u8g2.sendBuffer();
            http.end();
        }
    }
    delay(1000 * 60);
}
