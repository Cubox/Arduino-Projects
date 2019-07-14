#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <WiFiClient.h>
#include <max6675.h>

#define SERVER "192.168.0.252"
#ifndef PORT
    #error PORT not defined
#endif
#define CURRENT_YEAR 2019

#if PORT == 4201
    #define OFFSET -1.05
    #include "secrets1.h"
#elif PORT == 4202
    #define OFFSET -0.9
    #include "secrets2.h"
#else
    #error Welp
#endif

WiFiClient client;
MAX6675 thermocouple;

unsigned long timeLoop;

void flashLed(unsigned char times) {
    for (unsigned char i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(25);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(25);
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD); // Hint: secrets.h
    // Without WiFi being up, we are useless. Kill ourselves until it works.
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        flashLed(2);
        delay(100);
    }

    ArduinoOTA.begin();
    NTP.begin("europe.pool.ntp.org", 1, true, 0);
    client.setNoDelay(true);
    // This is a local server running netcat -l -k -w 5 4201 | tee log
    client.connect(SERVER, PORT);

    delay(200); // Let the max6675 chip start
    thermocouple.begin(D5, D6, D7);
    timeLoop = millis();
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
        flashLed(5);
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        goto end; // Hehe, fight me
    }

    if (!client.connected()) {
        client.connect(SERVER, PORT);
    }

    delay(250);
    temperature += thermocouple.readCelsius();
    i++;

    if (millis() - timeLoop >= 20000) {
        if (client.connected()) {
            // If you need a specific output format, this is where you set it up
            client.printf("%ld %.2f\n", now(), (temperature / i) + OFFSET);
            i = 0;
            temperature = 0;
            timeLoop = millis();
            // If despite the previous connect attempt, we are still not connected, we try to connect again
        } else {
            client.connect(SERVER, PORT);
        }
    } 

end:
    ArduinoOTA.handle();
}