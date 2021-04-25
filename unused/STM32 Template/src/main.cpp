#include <Arduino.h>

void setup() {
  pinMode(PC13, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  Serial.println("hi");
  digitalWrite(PC13, HIGH);
  delay(100);
  digitalWrite(PC13, LOW);
  delay(100);
}