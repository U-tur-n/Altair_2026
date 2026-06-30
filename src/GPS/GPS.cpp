//ヘッダーファイルをインクルード
#include <Arduino.h>
#include "GPS.h"
#include <TinyGPS++.h>

// ライブラリのインクルードは行わない(ヘッダーファイルですでに行っているため)

// 名前空間ここから(ヘッダーファイルと同じ名前空間を定義すること)
namespace GPS {
// 変数の定義
float latitude;   //緯度
float longitude;  //経度
const int RX1_PIN = 44;
const int TX1_PIN = 43;

TinyGPSPlus gps;
// SoftwareSerial mySerial(10, 11); // RX, TX
//TinyGPSCustom magneticVariation(gps, "GPRMC", 10);

// 自分だけが使う「内部変数」を定義(ヘッダーファイルでは宣言しない)

bool GPSbegin() {  //setup()の代わり


  Serial.println("Goodnight moon!");


  // set the data rate for the SoftwareSerial port!git s
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial1.println("Hello, world?");
  
    unsigned long lastTime = 0;
    while (millis() - lastTime < 5000) {
      while (Serial1.available() > 0) {
        char c = Serial1.read();
        //Serial.print(c);
        gps.encode(c);
        if (gps.location.isUpdated()) {
          set_latitude = gps.location.lat();
          set_longitude = gps.location.lng();
          Serial.print("Set Latitude: ");
          Serial.println(gps.location.lat(), 6);
          Serial.print("Set Longitude: ");
          Serial.println(gps.location.lng(), 6);
        }
        break;
      }
      if (millis() - lastTime >= 5000) {
        Serial.println("Cannot get GPS location. Please check the GPS module.");
        return false;
      }
    }

  
  }
  return true;

void execute() {  // run over and over
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    //Serial.print(c);
    gps.encode(c);
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
    }
    float dist_x = (longitude - set_longitude) * (6378137/sqrt(1 - 2.71828 * 2.71828 * sin((longitude + set_longitude)/2) * sin((longitude - set_longitude)/2))) * cos((latitude + set_latitude)/2);
    float dist_y = (latitude - set_latitude) * (6378137*((1 - 2.71828 * 2.71828)/(sqrt(1 - 2.71828 * 2.71828 * sin((longitude + set_longitude)/2) * sin((longitude - set_longitude)/2)))));
  }
}

//自作関数を定義する場合はここで行う

}