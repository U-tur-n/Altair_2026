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
float latitude_set;
float longitude_set;
float x_dist;
float y_dist;
float dist;
const int RX1_PIN = 44;
const int TX1_PIN = 43;
const float pi = 3.14159265358979323846;
const float rad = pi / 180;
const float Rx = 6378137.000; //長半径
const float Ry = 6356752.314245; //短半径

TinyGPSPlus gps;
// SoftwareSerial mySerial(10, 11); // RX, TX
//TinyGPSCustom magneticVariation(gps, "GPRMC", 10);

// 自分だけが使う「内部変数」を定義(ヘッダーファイルでは宣言しない)

bool GPSbegin() {  //setup()の代わり


  Serial.println("Goodnight moon!");


  // set the data rate for the SoftwareSerial port!git s
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial1.println("Hello, world?");
    while (Serial1.available() > 0) {
    char c = Serial1.read();
    //Serial.print(c);
    gps.encode(c);
    if (gps.location.isUpdated()) {
      latitude_set = gps.location.lat();
      longitude_set = gps.location.lng();
      float latitude_set_rad = latitude_set * rad;
      float longitude_set_rad = longitude_set * rad;
      // Serial.print(millis());
      // Serial.print(", ");
      // Serial.print(gps.location.lat(), 6);
      // Serial.print(", ");
      // Serial.println(gps.location.lng(), 6);
    }
  }
  return true;
}

void execute() {  // run over and over
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    //Serial.print(c);
    gps.encode(c);
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      float latitude_rad = latitude * rad;
      float longitude_rad = longitude * rad;
      float dy = (longitude - longitude_set) * rad;
      float dx = (latitude - latitude_set) * rad;
      float mu_y = ((latitude + latitude_set) / 2.0) * rad;
      float E2 = (pow(Rx, 2) - pow(Ry, 2)) / pow(Rx, 2);
      float W = sqrt(1.0 - E2 * pow(sin(mu_y), 2));
      float M = (Rx * (1.0 - E2)) / pow(W, 3);
      float N = Rx / W;
      x_dist = dx * M;
      y_dist = dy * N * cos(mu_y);
      dist = sqrt(pow(x_dist, 2) + pow(y_dist, 2));
      // Serial.print(millis());
      // Serial.print(", ");
      // Serial.print(gps.location.lat(), 6);
      // Serial.print(", ");
      // Serial.println(gps.location.lng(), 6);
    }
  }
}

//自作関数を定義する場合はここで行う

}