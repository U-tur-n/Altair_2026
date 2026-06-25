#include <Arduino.h>
#include <Adafruit_bno055.h>

namespace BNO {

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

void begin(){
  Serial.begin(115200);

  if (!bno.begin()) {
    Serial.println("no BNO055 detected");
    while (1);
  }
  bno.setExtCrystalUse(true);
}

void execute(){
   imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  float ax = a.x();
  float ay = a.y();
  float az = a.z();

  Serial.print("Accel: ");
  Serial.print(ax); Serial.print(", ");
  Serial.print(ay); Serial.print(", ");
  Serial.println(az);

  // クォータニオン 
  imu::Quaternion q = bno.getQuat();