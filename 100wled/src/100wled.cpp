#include <Arduino.h>

void updateLoop() {
  unsigned long a2 = 0;
  for (unsigned int i = 0; i < 100; i++) {
    a2 += analogRead(A3);
  }
  a2 = a2 / 100;
  if (a2 <= 500)
    a2 = 500;
  else if (a2 >= 950)
    a2 = 1023;
  
  unsigned char result = map(a2, 500, 1023, 0, 255);
  analogWrite(0, result);
}

void setup() {
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
}

void loop() {
  updateLoop();
}

