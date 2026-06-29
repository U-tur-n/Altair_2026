#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "BME.h"

// --- 仕様書に基づくピンアサインの設定 ---
#define BME_SCLK  D8
#define BME_MISO  D9
#define BME_MOSI D10
#define cs 3 //D2


// 同一SPIバス上のMicroSDスロットのCSピン（衝突防止用）
#define SD_CS 21

namespace BME{
// ソフトウェアSPIでBME280のインスタンスを生成
// Adafruit_BME280 bme(cs, BME_MOSI, BME_MISO, BME_SCLK);  //SPI
Adafruit_BME280 bme;  //I2C

// 打ち上げ地点の基準気圧（0m補正用変数）
float launchPressure = 1013.25; 
float lastTime = 0.0;

float altitude;

bool BMEbegin() {
  // Serial.begin(115200);
delay(1000); // シリアルモニタが開くまで待機

  // MicroSD側のCSピンをHIGHにして、SPIバスでの信号衝突を防ぐ
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  Serial.println("--- BME280 計測システム初期化 ---");

  // BME280の初期化チェック
  if (!bme.begin(0x76)) {
    Serial.println("エラー: BME280 センサが見つかりません。配線を確認してください。");
    bool result = false;
    return result; // 初期化失敗
  }

  // 起動直後の気圧を数回空読みして安定させる
  for(int i = 0; i < 5; i++) {
    bme.readPressure();
    delay(100);
  }

  // 【仕様書の要求】打ち上げ地点の気圧を取得し、ここを高度0mの基準（hPa）とする
  launchPressure = bme.readPressure() / 100.0F; 

  Serial.print("打ち上げ地点の基準気圧を設定しました: ");
  Serial.print(launchPressure);
  Serial.println(" hPa (この地点を高度 0m とします)");
  Serial.println("---------------------------------------");

  digitalWrite(cs, HIGH); // BME280のCSピンをHIGHにしてSPIバスを解放

  bool result = true; // 初期化成功
  return result;
}

void execute() {
  // --- 仕様書で定義された変数名 ---
  
  digitalWrite(cs, LOW); // BME280のCSピンをLOWにして通信開始
  // float pressure = bme.readPressure() / 100.0F;     // 気圧 [hPa]
  altitude = bme.readAltitude(launchPressure); // 高度 [m] (打ち上げ地点を0mとして算出)
  float temperature = bme.readTemperature();         // 温度 [°C] (気圧センサ内蔵)
  digitalWrite(cs, HIGH); // BME280のCSピンをHIGHにして通信終了
}

}