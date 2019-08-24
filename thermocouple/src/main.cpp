#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <WiFiClient.h>

#define SERVER "192.168.0.252"
#ifndef PORT
    #error PORT not defined
#endif
#define CURRENT_YEAR 2019

// the offsets serve as calibration for the probes.
#if PORT == 4201
    #define PROBEOFFSET -0.99
    #define KPROBE
    #include "secrets1.h"
#elif PORT == 4202
    #define PROBEOFFSET -0.79
    #define KPROBE
    #include "secrets2.h"
#elif PORT == 4203
    #define PROBEOFFSET -0.85
    #define KPROBE
    #include "secrets3.h"
#elif PORT == 4204
    #define PROBEOFFSET 0
    #define DS18B20
    #include "secrets4.h"
#elif PORT == 4205
    #define PROBEOFFSET 0
    #define DS18B20
    #include "secrets5.h"
#else
    #error Welp
#endif

#if defined(KPROBE)
    #include <max6675.h>
    MAX6675 thermocouple;
    #define TIMEBETWEENREADS 250
#elif defined(DS18B20)
    #include <OneWire.h>
    #include <DallasTemperature.h>
    #define ONE_WIRE_BUS D4
    #define TIMEBETWEENREADS 750
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature sensors(&oneWire);
#endif

WiFiClient client;


// signal that there is a problem
// The number of flashes are not reliable to determine origin of fail
// Mainly a cry for help directed at the user
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

unsigned long sentDataLast;
unsigned long readProbeLast;

void setup() {
    // the LED on the ESP are inverted from Arduinos. HIGH for OFF, LOW for ON
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    #if defined(KPROBE)
    thermocouple.begin(D5, D6, D7);
    #elif defined(DS18B20)
    sensors.begin();
    sensors.setResolution(12);
    sensors.requestTemperatures();
    #endif

    WiFi.mode(WIFI_STA);
    // SSID and passwords are stored under the secretsx.h header.
    // we need to add the include dir to .gitignore
    WiFi.begin(SSID, PASSWORD);
    // Without WiFi being up, we are useless. Restart the ESP after ~60s
    unsigned int i = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        handleNotConnectedWifi(&i);
    }

    ArduinoOTA.begin();
    NTP.begin("europe.pool.ntp.org", 1, true, 0);
    client.setNoDelay(true); // don't bunch up the packets
    client.setSync(true); // wait until the data is received before continue
    // This is a local server running netcat -l -k -w 5 PORT | tee log
    client.connect(SERVER, PORT);

    sentDataLast = millis();
    readProbeLast = 0; // we need an immediate reading
}

double temperature = 0;
unsigned int temperatureCount = 0;
unsigned int disconnectedCount = 0;

void loop() {
    ArduinoOTA.handle();

    // Either we have no time, or it's wrong. This need to be updated in a year.
    // Don't forget to update this!
    // We are not reporting without the proper time
    // But still need to call "end of loop" functions
    // (Also will be more obvious if there is a time problem, no output)
    if (year() != CURRENT_YEAR && year() != CURRENT_YEAR + 1) {
        flashLed(5);
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        return;
    }

    // if we lost the TCP server
    if (client.connected() == false) {
        client.connect(SERVER, PORT);
    }

    if (millis() - readProbeLast >= TIMEBETWEENREADS) {
        #if defined(KPROBE)
        double reading = thermocouple.readCelsius();
        #elif defined(DS18B20)
        double reading = sensors.getTempCByIndex(0);
        sensors.requestTemperatures();
        #endif
        readProbeLast = millis();

        if (reading < 20) {
            // Wrong readings can occur depending on physical placement of probe
            // Source of interference not found yet
            // However, since wrong readings were under 20°C, we can ignore those
            // For winter, we need to lower the threshold
            client.printf("#Incorrect reading: %.3f\n", reading);
            flashLed(10);
            return;
        }

        temperature += reading;
        temperatureCount++;
    }

    if (millis() - sentDataLast >= 10000) {
        if (client.connected() && WiFi.isConnected()) {
            client.printf("%ld %.3f\n", now(), (temperature / temperatureCount) + PROBEOFFSET);
            // We only update sentDataLast if we sent the data
            // If we were unable to send data, we should not wait another 20s
            // (see else block under here)
            sentDataLast = millis();
            disconnectedCount = 0;
        } else {
            // after 100 tries (minimum of 1s of delay due to flashLed)
            // we need to restart the ESP, just in case
            // counter is reset each successful send
            if (disconnectedCount >= 100) {
                flashLed(5);
                ESP.restart();
            }
            disconnectedCount++;
            flashLed(2);
        }

        temperatureCount = 0;
        temperature = 0;
    }
}