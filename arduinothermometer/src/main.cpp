#include <Arduino.h>
#include <LibPrintf.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 3
#define TIMEBETWEENREADS 1000
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define NEEDDEVICEADDRESS false

/* const DeviceAddress addresses[] = {
{0x28, 0xd6, 0xba, 0x79, 0xa2, 0, 0x3, 0xfa},
{0x28, 0x1e, 0x8d, 0x79, 0xa2, 0, 0x3, 0x11},
{0x28, 0x3e, 0x83, 0x79, 0xa2, 0, 0x3, 0x5},
{0x28, 0x14, 0xef, 0x79, 0xa2, 0, 0x3, 0x4c},
{0x28, 0xb4, 0x70, 0x79, 0xa2, 0, 0x3, 0xc8},
{0x28, 0xb1, 0x6e, 0x79, 0xa2, 0, 0x3, 0xfd},
{0x28, 0x9d, 0xd7, 0x79, 0xa2, 0, 0x3, 0x8a},
{0x28, 0xe7, 0xf7, 0x79, 0xa2, 0, 0x3, 0x25},
{0x28, 0x3f, 0x7b, 0x79, 0xa2, 0, 0x3, 0xaa}
};

const double offsets[] = {
    0.05,
    0.23,
    -0.25,
    0.45,
    -0.25,
    -0.25,
    0.32,
    0.05,
    -1.05
}; */

const DeviceAddress address = {0x28, 0xe7, 0xf7, 0x79, 0xa2, 0, 0x3, 0x25};
const double offset = 0.57;

double temperature = 0;
unsigned int temperatureCount = 0;
unsigned long lastReading = 0;
unsigned long sentDataLast = 0;

void setup() {
    sensors.begin();
    sensors.setResolution(12);
    Serial.begin(115200);
    sentDataLast = millis();      
}

void loop() {
    sensors.requestTemperatures();
    double reading = sensors.getTempC(address);
    if (reading < 0 || isnan(reading)) {
        return;
    }
    temperature += reading;
    temperatureCount++;

    if (millis() - sentDataLast >= 25000) {
        if (temperatureCount > 0 && !isnan(temperature)) {
            #if NEEDDEVICEADDRESS
                printf("%#x %#x %#x %#x %#x %#x %#x %#x: %.3f ",
                address[0], address[1], address[2], address[3],
                address[4], address[5], address[6], address[7],
                (temperature / temperatureCount) + offset);
            #else
                printf("%.3f ", (temperature / temperatureCount) + offset);
            #endif
            temperature = 0;
            temperatureCount = 0;
        }
        printf("\n");
        sentDataLast = millis();
    }
}