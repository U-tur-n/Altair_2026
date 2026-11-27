//ヘッダーファイルをインクルード
#include <Arduino.h>
#include "GPS.h"
#include <TinyGPS++.h> 
#include <math.h>
#include <sys/time.h>
#include <time.h>

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

bool set = false; //初期位置検出フラグ

TinyGPSPlus gps;
// SoftwareSerial mySerial(10, 11); // RX, TX
//TinyGPSCustom magneticVariation(gps, "GPRMC", 10);

// 自分だけが使う「内部変数」を定義(ヘッダーファイルでは宣言しない)

bool GPSbegin() {  //setup()の代わり
  setenv("TZ", "UTC0", 1);
  tzset();

  Serial.println("Goodnight moon!");


  // set the data rate for the SoftwareSerial port!git s
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial1.println("Hello, world?");
  return true;
}

static void syncClockFromGps() {
  static bool clockSynced = false;
  if (clockSynced || !gps.date.isUpdated() || !gps.time.isUpdated() ||
      !gps.date.isValid() || !gps.time.isValid()) {
    return;
  }

  // RMCの日時はUTC。取得できた時点でESP32の時計を合わせる。
  struct tm utc = {};
  utc.tm_year = gps.date.year() - 1900;
  utc.tm_mon = gps.date.month() - 1;
  utc.tm_mday = gps.date.day();
  utc.tm_hour = gps.time.hour();
  utc.tm_min = gps.time.minute();
  utc.tm_sec = gps.time.second();
  if (utc.tm_year < 100 || utc.tm_mon < 0 || utc.tm_mon > 11 ||
      utc.tm_mday < 1 || utc.tm_mday > 31 || utc.tm_hour > 23 ||
      utc.tm_min > 59 || utc.tm_sec > 60) {
    return;
  }

  const time_t seconds = mktime(&utc);
  const struct timeval now = {seconds, 0};
  clockSynced = seconds >= 0 && settimeofday(&now, nullptr) == 0;
}

void execute() {  // run over and over
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    //Serial.print(c);
    if (gps.encode(c)) {
      syncClockFromGps();
    }
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      if (latitude != 0.0 && longitude != 0.0 && set == false) {
        latitude_set = latitude;
        longitude_set = longitude;
        set = true;
      }
      float latitude_rad = latitude * rad;
      float longitude_rad = longitude * rad;
      float dy = (longitude - longitude_set) * rad;
      float dx = (latitude - latitude_set) * rad;
      float mu_y = ((latitude + latitude_set) / 2.0) * rad;
      float E2 = (Rx*Rx - Ry*Ry) / Rx*Rx;
      float W = sqrt(1.0 - E2 * sin(mu_y)*sin(mu_y));
      float M = (Rx * (1.0 - E2)) / W*W*W;
      float N = Rx / W;
      x_dist = dx * M;
      y_dist = dy * N * cos(mu_y);
      dist = sqrt(x_dist*x_dist + y_dist*y_dist);
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
