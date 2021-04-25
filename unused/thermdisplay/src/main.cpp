#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>

#define SERVER "192.168.0.252"
#define PORT 7625
#include "secrets1.h"

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

WiFiClient client;

double temperature1, temperature2, temperature3, temperature4, temperature5, temperature6, temperature7, temperatureoutside = 0;

void setup() {
    WiFi.mode(WIFI_STA);
    // SSID and passwords are stored under the secretsx.h header.
    // we need to add the include dir to .gitignore
    WiFi.begin(SSID, PASSWORD);
    // Without WiFi being up, we are useless. Restart the ESP after ~60s
    unsigned int i = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        if (i >= 100) {
            ESP.restart();
        }
        delay(200);
        i++;
    }

    ArduinoOTA.begin();
    ArduinoOTA.setRebootOnSuccess(true);

    client.setNoDelay(true);
    client.connect(SERVER, PORT);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
}



void loop() {
    ArduinoOTA.handle();

    if (client.connected() == false) {
        client.connect(SERVER, PORT);
    }

    if (client.connected()) {
        client.write("\n");
        char buffer[128];
        int i = client.readBytesUntil('\n', buffer, 128);
        if (i <= 0) {
            return;
        }

        sscanf(buffer, "%lf %lf %lf %lf %lf %lf %lf %lf ", &temperature1, &temperature2, &temperature3, &temperature4, &temperature5, &temperature6, &temperature7, &temperatureoutside);
    }

    //tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.printf("Chambre Andy :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature1);
    tft.setTextSize(1);
    tft.printf("Bureau :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature2);
    tft.setTextSize(1);
    tft.printf("Salon :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature3);
    tft.setTextSize(1);
    tft.printf("Chambre Mina :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature4);
    tft.setTextSize(1);
    tft.printf("Salle d'attente :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature7);
    tft.setTextSize(1);
    tft.printf("Couloir :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature5);
    tft.setTextSize(1);
    tft.printf("Salle de bain :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperature6);
    tft.setTextSize(1);
    tft.printf("Dehors :\n");
    tft.setTextSize(2);
    tft.printf("%.3f\n", temperatureoutside);
    delay(10000);
}