#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <WiFiClient.h>
#include "secrets.h"

#define DEBUGGING true

#if DEBUGGING
#include <RemoteDebug.h>
RemoteDebug Debug;
#endif

Adafruit_ADS1115 ads;

// Nicked this from somewhere, don't know don't ask
const double multiplier = 0.0625;
// This depends on the model of clamp being used
const double FACTOR = 50;

WiFiClient client;

void setup() {
    Wire.begin(D4, D5); // Change if you wired things differently.
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD); // Hint: secrets.h
    // Without WiFi being up, we are useless. Kill ourselves until it works.
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(1000);
        ESP.restart();
    }

    ArduinoOTA.begin();
    #if DEBUGGING
    Debug.begin("linky.cubox");
    #endif
    NTP.begin("europe.pool.ntp.org", 1, true, 0);
    // Since the max voltage the ADC will see is 1.42, we can double it
    // To gain more precision. If you change this, maybe update hardcoded values?
    ads.setGain(GAIN_TWO);
    ads.begin();
    client.setNoDelay(true);
    // This is a local server running netcat -l -k -w 5 4200 | tee log
    client.connect("192.168.0.252", 4200);
}

// Defined later
double getCurrent();

// A full loop should happen once per second, plus a liiiiittle bit each time
// Might be more if we are trying to reconnect to the server, or if NTP needs 
void loop() { 
    double currentRMS;
    double power;
    time_t t; // Need to declare here of the goto won't work

    // Either we have no time, or it's wrong. This need to be updated in a year.
    // Don't forget to update this!
    // We are not reporting power use without the proper time
    // But still need to call "end of loop" functions
    // (Also will be more obvious if there is a problem, no output)
    if (year() != 2019 && year() != 2020) { 
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        goto end; // Hehe, fight me
    }

    if (!client.connected()) {
        #if DEBUGGING
        rdebugAln("We are disconnected. Reconnecting");
        #endif
        client.connect("192.168.0.252", 4200);
    }
    t = now();
    currentRMS = getCurrent();
    power = 224 * currentRMS; // Calibrated for my house

    #if DEBUGGING
    rdebugA("%s: Current is: %fA, which makes power: %fW\n", NTP.getTimeDateString().c_str(), currentRMS, power);
    #endif
    
    if (client.connected()) {
        // If you need a specific output format, this is where you set it up
        client.printf("%ld %f %f\n", t, currentRMS, power);
    // If despite the previous connect attempt, we are still not connected, well, fuck.
    } else { 
        #if DEBUGGING
        rdebugAln("Despite trying to reconnect, no luck");
        #endif
    }

end:
    ArduinoOTA.handle();
    #if DEBUGGING
    Debug.handle();
    #endif
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