#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "BME/BME.h"
#include "BNO/BNO.h"
#include "GPS/GPS.h"

#define tact 1 // D0
#define cs_SD 21

unsigned long lastTime = 0;

std::vector<float> measureData;
File fp;
char fileName[] = "/data.csv";

// put function declarations here:
void data();

void setup() {
  // put your setup code here, to run once:
  //ピン設定
  pinMode(cs_SD, OUTPUT);
  pinMode(tact, INPUT_PULLUP);

  
  Serial.begin(115200);
  //SD初期化
  Serial.print("Initializing SD card...");
    if (SD.begin(cs_SD) == false) {
    Serial.println("SD card faile ro not present");
    while (1)
      ;
    }

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
  
  Serial.println("receive any key...");
  while (Serial.available() == 0)
    ;
    ;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - lastTime >= 100){
    measureData.push_back(millis());
    Serial.print(millis());
    Serial.print(", ");
    BME::execute();
    float altitude = BME::altitude;
    measureData.push_back(altitude);
    float latitude = GPS::latitude;
    float longitude = GPS::longitude;
    measureData.push_back(latitude);
    measureData.push_back(longitude);
    float ax = BNO::ax;
    float ay = BNO::ay;
    float az = BNO::az;
    float q = BNO::q;
    measureData.push_back(ax);
    measureData.push_back(ay); 
    measureData.push_back(az);
    measureData.push_back(q);

    Serial.print(altitude);
    Serial.println("");
    data();
  }
}

void data() {
  for (double data : measureData) {
    fp.print(data);
    fp.print(",");
    fp.println(data);
    fp.close();
    Serial.println("saved data");
    // Serial.println("receive any key...");
    if (Serial.available() != 0){
      while(1)
        ;
    }
  }
}

// put function definitions here: