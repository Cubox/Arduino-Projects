#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NTPClientLib.h>
#include <WiFiClient.h>
#include <lwip/etharp.h>

#define SERVER "192.168.0.252"
#ifndef PORT
    #error PORT not defined
#endif
#define CURRENT_YEAR 2020

#define CALIBRATING false
#define UNKNOWNDEVICEADDRESS false
#define NULLDEVICEADDRESS {0, 0, 0, 0, 0, 0, 0, 0}
#if CALIBRATING
    #include "secrets1.h"
#endif

#define GLOBALOFFSET 0.25

// the offsets serve as calibration for the probes.
#if PORT == 4201 // Andy
    #include "secrets1.h"
    #define DEVICEADDRESS {0x28, 0xa9, 0x8d, 0x79, 0xa2, 0x16, 0x3, 0xd5}
    #define PROBEOFFSET GLOBALOFFSET // Always zero, since this is the reference
#elif PORT == 4202 // Bureau
    #include "secrets2.h"
    #define DEVICEADDRESS {0x28, 0xdf, 0xa4, 0x79, 0xa2, 0, 0x3, 0xb2}
    #define PROBEOFFSET 0.35+GLOBALOFFSET
#elif PORT == 4203 // Salon
    #include "secrets3.h"
    #define DEVICEADDRESS {0x28, 0x26, 0x90, 0x79, 0xa2, 0x16, 0x3, 0x8b}
    #define PROBEOFFSET 0.6+GLOBALOFFSET
#elif PORT == 4204 // Mina
    #include "secrets4.h"
    #define DEVICEADDRESS {0x28, 0xe5, 0x8b, 0x79, 0xa2, 0x16, 0x3, 0x41}
    #define PROBEOFFSET 0.4+GLOBALOFFSET
#elif PORT == 4205 // Couloir
    #include "secrets5.h"
    #define DEVICEADDRESS {0x28, 0x20, 0x51, 0x79, 0xa2, 0x16, 0x3, 0xd6}
    #define PROBEOFFSET 0.83+GLOBALOFFSET
#elif PORT == 4206 // Salle de bain
    #include "secrets6.h"
    #define DEVICEADDRESS {0x28, 0xc1, 0xc0, 0x79, 0xa2, 0x16, 0x3, 0xb2}
    #define PROBEOFFSET 0.51+GLOBALOFFSET
#elif PORT == 4207 // Salle d'attente
    #include "secrets7.h"
    #define DEVICEADDRESS {0x28, 0xb4, 0x70, 0x79, 0xa2, 0, 0x3, 0xc8}
    #define PROBEOFFSET -0.29+GLOBALOFFSET
#else
    #error Welp
#endif

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress deviceAddress = DEVICEADDRESS;

WiFiClient client;

#if UNKNOWNDEVICEADDRESS
    bool isnullDeviceAddress(DeviceAddress address) {
        return (
        address[0] == 0 && address[1] == 0 &&
        address[2] == 0 && address[3] == 0 &&
        address[4] == 0 && address[5] == 0 &&
        address[6] == 0 && address[7] == 0);
    }
#endif

// signal that there is a problem
// The number of flashes are not reliable to determine origin of fail
// Mainly a cry for help directed at the user
void flashLed(unsigned char times) {
    for (unsigned char i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
    }
}

unsigned long sentDataLast;
double temperature;
unsigned int temperatureCount, disconnectedCount, ntpFailed;

void setup() {
    // the LED on the ESP are inverted from Arduinos. HIGH for OFF, LOW for ON
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    sensors.begin();
    sensors.setResolution(12);
    sensors.requestTemperaturesByAddress(deviceAddress);

    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    WiFi.mode(WIFI_STA);
    // WiFi.setSleepMode(WIFI_NONE_SLEEP);
    // SSID and passwords are stored under the secretsx.h header.
    // we need to add the include dir to .gitignore
    WiFi.begin(SSID, PASSWORD);
    // Without WiFi being up, we are useless. Restart the ESP after ~60s
    unsigned int i = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        if (i >= 10) {
            ESP.restart();
        }
        flashLed(2);
        delay(200);
        i++;
    }

    ArduinoOTA.begin();
    ArduinoOTA.setRebootOnSuccess(true);
    
    NTP.begin("europe.pool.ntp.org", 1, true, 0);

    client.setNoDelay(true); // don't bunch up the packets
    client.setSync(true); // wait until the data is received before continue
    client.connect(SERVER, PORT);
    
    #if UNKNOWNDEVICEADDRESS
        if (isnullDeviceAddress(deviceAddress)) {
            sensors.getAddress(deviceAddress, 0);
            client.printf("#Device address: %#x %#x %#x %#x %#x %#x %#x %#x\n",
            deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3],
            deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
        }
    #endif

    sentDataLast = millis();

    temperature = 0;
    temperatureCount = 0;
    disconnectedCount = 0;
    ntpFailed = 0;
}

void loop() {
    ArduinoOTA.handle();

    // Either we have no time, or it's wrong. This need to be updated in a year.
    // Don't forget to update this!
    // We are not reporting without the proper time
    // But still need to call "end of loop" functions
    // (Also will be more obvious if there is a time problem, no output)
    if (year() != CURRENT_YEAR && year() != CURRENT_YEAR + 1) {
        if (ntpFailed >= 10) {
            ESP.restart();
        }
        ntpFailed++;
        flashLed(1);
        NTP.begin("europe.pool.ntp.org", 1, true, 0);
        return;
    }
    ntpFailed = 0;

    // if we lost the TCP server
    if (client.connected() == false) {
        client.connect(SERVER, PORT);
    }

    if (sensors.isConversionComplete()) {
        double reading = sensors.getTempC(deviceAddress);

        if (reading < -100 || isnan(reading)) {
            client.printf("#%lu %.3f\n", now(), reading);
            flashLed(3);
            return;
        }

        temperature += reading;
        temperatureCount++;

        sensors.requestTemperaturesByAddress(deviceAddress);
    }

    if (millis() - sentDataLast >= 20000 && temperatureCount > 0 && !isnan(temperature)) {
        if (client.connected() && WiFi.isConnected() && now() > 0) {
            size_t len = client.printf("%lu %.3f\n", now(), (temperature / temperatureCount) + PROBEOFFSET);
            // We only update sentDataLast if we sent the data
            if (len > 0) {
                sentDataLast = millis();
                disconnectedCount = 0;
            } else {
                if (disconnectedCount >= 10) {
                    ESP.restart();
                }
                disconnectedCount++;
                flashLed(2);
                client.connect(SERVER, PORT);
            }
        } else {
            // after 100 tries
            // we need to restart the ESP, just in case
            // counter is reset each successful send
            if (disconnectedCount >= 10) {
                ESP.restart();
            }
            disconnectedCount++;
            flashLed(2);
            client.connect(SERVER, PORT);
        }

        temperatureCount = 0;
        temperature = 0;
    }
}