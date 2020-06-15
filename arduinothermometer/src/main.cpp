#include <Arduino.h>
#define REQUIRESALARMS false
#define REQUIRESNEW false
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 3
#define TIMEBETWEENREADS 1000
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    sensors.begin();
    sensors.setResolution(12);
    Serial.begin(9600);
}

void loop() {
    sensors.requestTemperatures();
    for (unsigned int i = 0; i < sensors.getDS18Count(); i++) {
        float temp = sensors.getTempCByIndex(i);
        Serial.print(temp);
        Serial.print(" ");
    }
    Serial.println();
    delay(1000);
}