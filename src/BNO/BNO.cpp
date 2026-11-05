#include <Arduino.h>
#include "BNO.h"

namespace BNO {

const uint8_t BNO_I2C_ADDR = 0x28; //0x29だとだめだった
Adafruit_BNO055 bno = Adafruit_BNO055(-1, BNO_I2C_ADDR, &Wire);

void BNObegin(){
  Serial.begin(115200);
  delay(100); 

  if (!bno.begin()) {
    Serial.println("no BNO055");
    while (1);
  }
  //bno.setExtCrystalUse(true);
  Serial.println("BNO055 OK");

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x3D);           // OPR_MODE
  Wire.write(0x00);           // CONFIGMODE
  Wire.endTransmission(true);
  delay(25);                 

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x07);           // PAGE_ID 
  Wire.write(0x01);           // ページ1
   Wire.endTransmission(true);
  delay(10);

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x08);           // ACC_CONFIG 
  Wire.write(0b00001101);     // 16Gに設定
  Wire.endTransmission(true);
  delay(10);

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x08);           // ACC_CONFIG
  Wire.endTransmission(false); 
  Wire.requestFrom((uint8_t)BNO_I2C_ADDR, (size_t)1, true);  
  if (Wire.available()) {
    byte regVal = Wire.read(); 
    
    Serial.print("読み取り値 : 0x");
    Serial.println(regVal, HEX);

    uint8_t rangeBits = regVal & 0x03; 

    Serial.print("加速度範囲：");
    Serial.println(rangeBits, BIN);
    }

  else {
    Serial.println("register read error");
  }

  delay(100);

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x07);           // PAGE_ID
  Wire.write(0x00);           // ページ0
  Wire.endTransmission();
  delay(10);

  Wire.beginTransmission(BNO_I2C_ADDR);
  Wire.write(0x3D);           // OPR_MODE
  Wire.write(0x0C);           // NDOFモード
  Wire.endTransmission();
  delay(25); 
}

void execute(){
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  float ax = a.x();
  float ay = a.y();
  float az = a.z();

  // Serial.print("Accel: ");
  // Serial.print(ax); Serial.print(", ");
  // Serial.print(ay); Serial.print(", ");
  // Serial.print(az);

  imu::Quaternion q = bno.getQuat();

  float w = q.w();
  float x = q.x();
  float y = q.y();
  float z = q.z();

  float yaw   = atan2(2.0f * (w*z + x*y), 1.0f - 2.0f*(y*y + z*z));
  float sinp  = 2.0f * (w*y - z*x);
  if (sinp > 1) sinp = 1;
  if (sinp < -1) sinp = -1;
  float pitch = asin(sinp);
  float roll  = atan2(2.0f * (w*x + y*z), 1.0f - 2.0f*(x*x + y*y));

  roll  *= 180.0f / PI;
  pitch *= 180.0f / PI;
  yaw   *= 180.0f / PI;

  // Serial.print("Euler: ");
  // Serial.print(yaw); Serial.print(", ");
  // Serial.print(pitch); Serial.print(", ");
  // Serial.println(roll);
  delay(70);

  Serial.print("Q: ");
  Serial.print(w, 4); Serial.print(", ");
  Serial.print(x, 4); Serial.print(", ");
  Serial.print(y, 4); Serial.print(", ");
  Serial.print(z, 4);

  Serial.print("   Euler: ");
  Serial.print(yaw, 2); Serial.print(", ");
  Serial.print(pitch, 2); Serial.print(", ");
  Serial.println(roll, 2);
 }
}
