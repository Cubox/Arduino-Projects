#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <WiFiClient.h>
#include "secrets.h"

Adafruit_ADS1115 ads;

// Nicked this from somewhere, don't know don't ask
const double multiplier = 0.0625;
// This depends on the model of clamp being used
const double FACTOR = 30;

WiFiClient client;

void flashLed(unsigned char times) {
    for (unsigned char i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
    }
}

void handleNotConnectedWifi(unsigned int *i) {
    if (*i >= 100) {
        ESP.restart();
    }
    flashLed(2);
    delay(200);
    (*i)++;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Wire.begin(D4, D5); // Change if you wired things differently.
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD); // Hint: secrets.h
    // Without WiFi being up, we are useless. Kill ourselves until it works.
    unsigned int i = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        handleNotConnectedWifi(&i);
    }

    ArduinoOTA.begin();
    NTP.begin("europe.pool.ntp.org", 1, true, 0);
    // Since the max voltage the ADC will see is 1.42, we can double it
    // To gain more precision. If you change this, maybe update hardcoded values?
    ads.setGain(GAIN_TWO);
    ads.begin();
    client.setNoDelay(true);
    client.setSync(true);
    // This is a local server running netcat -l -k -w 5 4200 | tee log
    client.connect("192.168.0.252", 4200);
}

// Defined later
double getCurrent();

// A full loop should happen once per second, plus a liiiiittle bit each time
// Might be more if we are trying to reconnect to the server, or if NTP needs 
void loop() { 
    ArduinoOTA.handle();

    // Either we have no time, or it's wrong. This need to be updated in a year.
    // Don't forget to update this!
    // We are not reporting power use without the proper time
    // But still need to call "end of loop" functions
    // (Also will be more obvious if there is a time problem, no output)
    if (year() != 2019 && year() != 2020) {
        flashLed(5);
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        return;
    }

    if (!client.connected()) {
        client.connect("192.168.0.252", 4200);
    }
    time_t t = now();
    double currentRMS = getCurrent();
    double power = 230 * currentRMS; // Calibrated for my house
    
    if (client.connected()) {
        // If you need a specific output format, this is where you set it up
        client.printf("%ld %f %f\n", t, currentRMS, power);
    // If despite the previous connect attempt, we are still not connected, well, fuck.
    } else { 
        client.connect("192.168.0.252", 4200);
    }
}

// Got this from some website, not made by me
double getCurrent() {
    double voltage;
    double current;
    double sum = 0;
    long t = millis();
    int counter = 0;

    while (millis() - t < 1000) {
        voltage = ads.readADC_Differential_0_1() * multiplier;
        current = voltage * FACTOR;
        current /= 1000.0;

        sum += sq(current);
        counter++;
    }

    current = sqrt(sum / counter);
    return(current);
}