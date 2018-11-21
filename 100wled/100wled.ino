#include <SoftPwmTask.h>
#include <SoftTimer.h>

SoftPwmTask pwm(0);

bool truePwm = false;

void updateLoop(Task *me) {
  unsigned int a2 = analogRead(A2);
  if (a2 <= 2)
    a2 = 0;
  else if (a2 >= 253)
    a2 = 255;
  pwm.analogWrite(a2);
  unsigned int a3 = analogRead(A3);
  if (a3 <= 10) {
    SoftTimer.remove(&pwm);
    truePwm = true;
    pinMode(0, OUTPUT);
    analogWrite(0, a2);
    return;
  }
  pwm.setFrequency(map(a3, 0, 255, 0.1, 10000));
  if (!truePwm) {
    SoftTimer.add(&pwm);
    truePwm = false;
  }
}

Task updateV(100, updateLoop);

void setup() {
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  SoftTimer.add(&updateV);
}

//void loop() {
//  digitalWrite(0, analogRead(A2));
//}

