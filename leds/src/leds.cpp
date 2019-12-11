#include <Arduino.h>
#include <FastLED.h>
#include <EEPROM.h>

#define RIGHTLED 35
#define RIGHTPIN 6
#define LEFTPIN 8
#define LEFTLED 35

CRGB right[RIGHTLED];
CRGB left[LEFTLED];

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

enum modes {
  OFF,
  ON,
  RAINBOW,
  BORANGE,
  BBLUE,
  TEST1
};

enum modes mode = OFF;
char globalBrightness = 150;
bool toUpdate = false;

void setup() {
  Serial.begin(9600);

  mode = EEPROM.read(0);
  globalBrightness = EEPROM.read(1);
  
  FastLED.addLeds<WS2812B, RIGHTPIN, GRB>(right, RIGHTLED);
  FastLED.addLeds<WS2812B, LEFTPIN, GRB>(left, LEFTLED);
  FastLED.setBrightness(globalBrightness);
  FastLED.setCorrection(TypicalLEDStrip);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  toUpdate = true;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex, CRGB *strip, uint8_t len)
{
    uint8_t brightness = 255;
    
    for(int i = 0; i < len; i++) {
        strip[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

void serialEvent() {
  if (Serial.available()) {
    char buf[128];
    char *buf2;
    memset(buf, 0, 128);
    //Serial.setTimeout(10);
    Serial.readBytesUntil('\n', buf, 128);
    if (strcmp(buf, "ON") == 0) {
      mode = ON;
    } else if (strcmp(buf, "OFF") == 0) {
      mode = OFF;
    } else if (strcmp(buf, "R") == 0) {
      mode = RAINBOW;
    } else if (strcmp(buf, "BO") == 0) {
      mode = BORANGE;
    } else if (strcmp(buf, "BB") == 0) {
      mode = BBLUE;
    } else if (strcmp(buf, "T") == 0) {
      mode = TEST1;
    } else if ((buf2 = strstr(buf, "Br: ")) != NULL) {
      buf2 += 4;
      int received = atoi(buf2);
      if (received <= 0 || received > 255) {
        return;
      }
      globalBrightness = received;
    } else {
      return;
    }
    EEPROM.update(0, mode);
    EEPROM.update(1, globalBrightness);
    toUpdate = true;
  }
}

void loop() {
  float breath;
  float bright;
  if (toUpdate) {
    switch (mode) {
      case OFF: FastLED.setBrightness(0); toUpdate = false; break; // off
      case ON: // normal use (supposed to be 255,152,54 without correction)
          FastLED.setBrightness(globalBrightness);
          fill_solid(right, RIGHTLED, CRGB(255,223,69));
          fill_solid(left, LEFTLED, CRGB(255,223,69));
          toUpdate = false;
          break;
      case RAINBOW: // rainbow
          FastLED.setBrightness(255);
          static uint8_t startIndex = 0;
          startIndex = startIndex + 1; /* motion speed */

          FillLEDsFromPaletteColors(startIndex, right, RIGHTLED);
          FillLEDsFromPaletteColors(startIndex, left, LEFTLED);
          break;
       case BORANGE: // orange breathing
          FastLED.setBrightness(globalBrightness);
          fill_solid(right, RIGHTLED, CRGB(255,30,0));
          fill_solid(left, LEFTLED, CRGB(255,30,0));
          breath = (exp(sin(millis()/2000.0*PI))-0.36787944)*108.0;
          bright = map(breath, 0, 255, 0, 255); // min breath, max breath, min brightness, max brightness
          FastLED.setBrightness(bright);
          break;
       case BBLUE: // blue breathing
          FastLED.setBrightness(globalBrightness);
          fill_solid(right, RIGHTLED, CRGB::Blue);
          fill_solid(left, LEFTLED, CRGB::Blue);
          breath = (exp(sin(millis()/2000.0*PI))-0.36787944)*108.0;
          bright = map(breath, 0, 255, 0, 255); // min breath, max breath, min brightness, max brightness
          FastLED.setBrightness(bright);
          break;
       case TEST1: // testing
          FastLED.setBrightness(globalBrightness);
          fill_solid(right, RIGHTLED, CRGB::Blue);
          fill_solid(left, LEFTLED, CRGB::Blue);
          breath = (exp(sin(millis()/5000.0*PI))-0.36787944)*308.0;
          bright = map(breath, 0, 255, 0, 20); // min breath, max breath, min brightness, max brightness
          FastLED.setBrightness(bright);
          break;
    }
  }
  FastLED.show();
  //delay(1000);
}

