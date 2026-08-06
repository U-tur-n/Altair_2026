#include "servo.h"

namespace servo{
  const int servoPin = 5;
  int DutyCycle = 500;

  void begin() {
   pinMode(servoPin, OUTPUT);
   Serial.begin(9600);
  }

  void execute() {
    if (Serial.available() > 0) {

      char inkey = Serial.read();

      if (inkey == 'a') {

       for (DutyCycle = 500; DutyCycle <= 501; DutyCycle += 1) {//90°回転
          digitalWrite(servoPin, HIGH);
          delayMicroseconds(DutyCycle);
          digitalWrite(servoPin, LOW);
          delayMicroseconds(20000 - DutyCycle);
       }

          digitalWrite(servoPin, HIGH);//戻らないように
          delayMicroseconds(501);
          digitalWrite(servoPin, LOW);
          delayMicroseconds(20000 - 501);

      }
    }
  }

}