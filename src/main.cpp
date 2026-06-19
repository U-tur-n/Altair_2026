#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "BME/BME.h"

#define tact D0
#define cs_SD 21

unsigned long lastTime = 0;

std::vector<float> measureData;
FILE fp;
char fileName[] = "data.csv";

// put function declarations here:
void data();

void setup() {
  // put your setup code here, to run once:
  //ピン設定
  pinMode(cs_SD, OUTPUT);
  pinMode(tact, INPUT_PULLUP);

  //SD初期化
  Serial.print("Initializing SD card...");
    if (SD.begin(cs_SD) == false) {
    Serial.println("SD card faile ro not present");
    }
    while (1)
      ;

  BME::begin();

  //ボーレートを115200
  Serial.end();
  Serial.begin(115200);

  //タクトスイッチが押されたら計測開始
  Serial.print("Opening the file...");
  fp = SD.open(fileName, "FILE_WRITE");
  if (fp == false) {
    Serial.println("cannot open the file");
    while (1)
      ;
  }
  
  Serial.println("Ready...");
  while (tact)
    ;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - lastTime >= 100){
  BME::execute();
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
    while (1);
  }
}

// put function definitions here: