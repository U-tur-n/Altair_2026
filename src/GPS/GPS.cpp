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

void begin() {  //setup()の代わり
  // Open serial communications and wait for port to open:
  // Serial.begin(57600);z
  // while (!Serial) {
  //   ;  // wait for serial port to connect. Needed for native USB port only
  // }

  Serial.println("Goodnight moon!");


  // set the data rate for the SoftwareSerial port!git s
  Serial1.begin(9600, SERIAL_8N1, RX1_PIN, TX1_PIN);
  Serial1.println("Hello, world?");
}

void execute() {  // run over and over
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    //Serial.print(c);
    gps.encode(c);
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
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