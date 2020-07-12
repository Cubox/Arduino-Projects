#include <Arduino.h>
#include <LibPrintf.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define NEEDDEVICEADDRESS false

const DeviceAddress address = {0x28, 0xe7, 0xf7, 0x79, 0xa2, 0, 0x3, 0x25};
const double offset = 0.57;

double temperature = 0;
unsigned int temperatureCount = 0;
unsigned long lastReading = 0;
unsigned long sentDataLast = 0;

void setup() {
    sensors.begin();
    sensors.setResolution(12);
    sensors.requestTemperaturesByAddress(address);
    Serial.begin(115200);
    sentDataLast = millis();      
}

void loop() {
    if (sensors.isConversionComplete()) {
        double reading = sensors.getTempC(address);
        if (reading < 0 || isnan(reading)) {
            printf("#%.3f\n", reading);
            return;
        }
        temperature += reading;
        temperatureCount++;
        sensors.requestTemperaturesByAddress(address);
    }

    if (millis() - sentDataLast >= 30000) {
        if (temperatureCount > 0 && !isnan(temperature)) {
            #if NEEDDEVICEADDRESS
                printf("%#x %#x %#x %#x %#x %#x %#x %#x: %.3f\n",
                address[0], address[1], address[2], address[3],
                address[4], address[5], address[6], address[7],
                (temperature / temperatureCount) + offset);
            #else
                printf("%.3f\n", (temperature / temperatureCount) + offset);
            #endif
            temperature = 0;
            temperatureCount = 0;
            sentDataLast = millis();
        }
    }
}