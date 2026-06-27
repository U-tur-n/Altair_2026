#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "BME/BME.h"
#include "BNO/BNO.h"
#include "GPS/GPS.h"

#define tact 1 // D0
#define SD_CS 21
#define BME_CS 3

unsigned long lastTime = 0;

float altitude = BME::altitude;
float latitude = GPS::latitude;
float longitude = GPS::longitude;
float ax;
float ay;
float az;
float q;

std::vector<float> measureData;
File fp;
char fileName[] = "/data0.csv";

// put function declarations here:
void save(bool);

void setup() {
  // put your setup code here, to run once:
  //ピン設定
  pinMode(SD_CS, OUTPUT);
  pinMode(BME_CS, OUTPUT);
  pinMode(tact, INPUT_PULLUP);

  
SPI.begin();
  //SPI衝突回避用
  digitalWrite(SD_CS, HIGH);
  digitalWrite(BME_CS, HIGH);

  

  Serial.begin(115200);
  while(!Serial);
  delay(100);

  //SD初期化
  
  Serial.print("Initializing SD card...");
    if (SD.begin(SD_CS) == false) {
    Serial.println("SD card faile ro not present");
    while (1)
      ;
    }
    
  Serial.println("OK");


  
  // Serial.println("receive any key...");
  // while (Serial.available() == 0)
  //   ;
  //   ;
  

  BME::begin();
  BNO::begin();
  GPS::begin();

  //ボーレートを115200
  Serial.end();
  Serial.begin(115200);

    //タクトスイッチが押されたら計測開始
  Serial.print("Opening the file...");
  fp = SD.open(fileName, FILE_WRITE);
  if (fp == false) {
    Serial.println("cannot open the file");
    while (1)
      ;
  }
  Serial.println("OK");
  fp.println("time, altitude, latitude, longitude, ax, ay, az, q");
  Serial.println("press the tact switch...");
  while(digitalRead(tact) == HIGH)
    ;
}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - lastTime >= 100){
    measureData.push_back(millis());
    Serial.print(millis());
    Serial.print(", ");

    BME::execute();
    GPS::execute();
    BNO::execute();
    altitude = BME::altitude;
    latitude = GPS::latitude;
    longitude = GPS::longitude;
    ax = BNO::ax;
    ay = BNO::ay;
    az = BNO::az;
    // q = BNO::q;

    measureData.push_back(altitude);
    measureData.push_back(latitude);
    measureData.push_back(longitude);
    measureData.push_back(ax);
    measureData.push_back(ay); 
    measureData.push_back(az);
    // measureData.push_back(q);

    Serial.print(altitude);
    Serial.print(", ");
    Serial.print(latitude);
    Serial.print(", ");
    Serial.print(longitude);
    Serial.print(", ");
    Serial.print(ax);
    Serial.print(", ");
    Serial.print(ay);
    Serial.print(", ");
    Serial.println(az);
    // Serial.print(", ");
    // Serial.println(q);
    if(digitalRead(tact) == LOW){
      save(true);
    }
    if(measureData.size() >= 2100){
      save(false);
    }
  lastTime = millis();
  }
}

// put function definitions here:
void save(bool end) {
  int i = 0;
  digitalWrite(SD_CS, LOW);
    for (double data : measureData)
    {
      fp.print(data);
      i++;
      if (i % 7 == 0) { // 7個のデータごとに改行
        fp.println();
      } else {
        fp.print(",");
      }
      // fp.print(",");
      // fp.println(data); // q
      // Serial.println("receive any key...");
      // if (Serial.available() != 0){
      //   while(1)
      //     ;
      // }
    }
  if (end == false) { // データ保存のみ
    fp.flush();
    Serial.println("saved data");
    measureData.clear();
    digitalWrite(SD_CS, HIGH);
  } else {
    fp.close(); // ファイルを閉じる(スイッチが押された場合)
    Serial.println("saved data and closed file");
    measureData.clear();
    while (1)
      ;
  }
}
