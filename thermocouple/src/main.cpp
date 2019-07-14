#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <WiFiClient.h>
#include <max6675.h>
#include "secrets.h"

#define SERVER "192.168.0.252"
#ifndef PORT
    #error PORT not defined
#endif
#define CURRENT_YEAR 2019

#if PORT == 4201
    #define OFFSET -1.1
#elif PORT == 4202
    #define OFFSET -0.9
#else
    #error Welp
#endif

WiFiClient client;
MAX6675 thermocouple;

unsigned long timeLoop;

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD); // Hint: secrets.h
    // Without WiFi being up, we are useless. Kill ourselves until it works.
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(100);
        ESP.restart();
    }

    ArduinoOTA.begin();
    NTP.begin("europe.pool.ntp.org", 1, true, 0);
    client.setNoDelay(true);
    // This is a local server running netcat -l -k -w 5 4201 | tee log
    client.connect(SERVER, PORT);

    delay(200); // Let the max6675 chip start
    timeLoop = millis();
    thermocouple.begin(D5, D6, D7);
}

double temperature = 0;
double i = 0;

void loop() {
    // Either we have no time, or it's wrong. This need to be updated in a year.
    // Don't forget to update this!
    // We are not reporting without the proper time
    // But still need to call "end of loop" functions
    // (Also will be more obvious if there is a time problem, no output)
    if (year() != CURRENT_YEAR && year() != CURRENT_YEAR + 1) { 
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        goto end; // Hehe, fight me
    }

    if (!client.connected()) {
        client.connect(SERVER, PORT);
    }

    delay(250);
    temperature += thermocouple.readCelsius();
    i++;

    if (millis() - timeLoop >= 10000) {
        if (client.connected()) {
            // If you need a specific output format, this is where you set it up
            client.printf("%ld %.2f\n", now(), (temperature / i) + OFFSET);
            // If despite the previous connect attempt, we are still not connected, well, fuck.
        }
        i = 0;
        temperature = 0;
        timeLoop = millis();
    }

end:
    ArduinoOTA.handle();
}