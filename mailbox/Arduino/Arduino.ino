#include <EnableInterrupt.h>

#define ESPGATE 5
#define ESPGATEBUTTON 12

bool espoverride = false;

void espgateint() {
  espoverride = !espoverride;
  //Serial.println("int");
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ESPGATEBUTTON, INPUT_PULLUP);
  pinMode(ESPGATE, OUTPUT);
  digitalWrite(ESPGATE, LOW);
  enableInterrupt(ESPGATEBUTTON, espgateint, FALLING);
}

void loop() {
  if (Serial.available() > 0) {
    byte inByte = Serial.read();
    if (inByte == 3) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.write("hi");
    }
  }

  if (espoverride) {
    digitalWrite(ESPGATE, HIGH);
  } else {
    digitalWrite(ESPGATE, LOW);
  }
}

