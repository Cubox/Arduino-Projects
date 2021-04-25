#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A6, OUTPUT);
  pinMode(A7, OUTPUT);
  digitalWrite(A6, HIGH);
  pinMode(A5, OUTPUT);
  pinMode(A4, OUTPUT);
  digitalWrite(A5, HIGH);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(A7, HIGH);
  digitalWrite(A4, LOW);
  delay(5);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(A7, LOW);
  digitalWrite(A4, HIGH);
  delay(5);
}