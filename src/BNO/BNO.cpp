#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include "BNO.h"

namespace BNO {

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

float ax;
float ay;
float az;
float q;


void begin(){
  // Serial.begin(115200);
  Wire.begin();
  Wire.setClock(50000);   // 通信速度を50kHzに下げて安定させる
  Wire.setTimeOut(20000); // センサーの演算待ちによるタイムアウトを防ぐ

  if (!bno.begin()) {
    Serial.println("no BNO055 detected");
    while (1);
  }
  Serial.println("BNO055 OK");
  bno.setExtCrystalUse(true);
}

void execute(){
   imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  ax = a.x();
  ay = a.y();
  az = a.z();

  // Serial.print("Accel: ");
  // Serial.print(ax); Serial.print(", ");
  // Serial.print(ay); Serial.print(", ");
  // Serial.println(az);

  // クォータニオン 
  imu::Quaternion q = bno.getQuat();
  
}
}