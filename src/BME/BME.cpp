#include "BME.h"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// --- 仕様書に基づくピンアサインの設定 ---
#define BME_SCLK  D8
#define BME_MISO  D9
#define BME_MOSI D10
#define BME_CS    D2

// 同一SPIバス上のMicroSDスロットのCSピン（衝突防止用）
#define SD_CS     21

namespace BME{
// ソフトウェアSPIでBME280のインスタンスを生成
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCLK);

// 打ち上げ地点の基準気圧（0m補正用変数）
float launchPressure = 1013.25; 
float lastTime = 0.0;

void begin() {
  Serial.begin(115200);
delay(1000); // シリアルモニタが開くまで待機

  // MicroSD側のCSピンをHIGHにして、SPIバスでの信号衝突を防ぐ
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  Serial.println("--- BME280 計測システム初期化 ---");

  // BME280の初期化チェック
  if (!bme.begin()) {
    Serial.println("エラー: BME280 センサが見つかりません。配線を確認してください。");
    while (1);
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
}

void execute() {
  // --- 仕様書で定義された変数名 ---
  
  if (millis() - lastTime >= 1000){
  // float pressure = bme.readPressure() / 100.0F;     // 気圧 [hPa]
  float altitude = bme.readAltitude(launchPressure); // 高度 [m] (打ち上げ地点を0mとして算出)
  float temperature = bme.readTemperature();         // 温度 [°C] (気圧センサ内蔵)

  // シリアルモニタへの出力（デバッグ用）
  // Serial.print("気圧 [pressure]: ");
  // Serial.print(pressure);
  // Serial.print(" hPa | ");

  Serial.print("高度 [altitude]: ");
  Serial.print(altitude);
  Serial.print(" m | ");

  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.println(" °C");

  lastTime = millis();
  }
}

}