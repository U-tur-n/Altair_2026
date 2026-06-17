#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <vector>
// #include "../GPS/GPS.h"
// #include "../BNO/BNO.h"
#include "../BME/BME.h"

namespace SD{
      std::vector<float> measureData;

      void begin(){
      Serial.print("Initializing SD card...");
      if (SD.begin(cs) == false) {
    Serial.println("SD card faile ro not present");
    while (1)
      ;
}
      }

      void execute(){
      Serial.print(millis());
      measureData.push_back(millis());
      Serial.print(" --> ");
      // Serial.print("acc_x = ");
      // Serial.print(BNO::acceralation_x);
      // measureData.push_back(BNO::acceralation_x);
      // Serial.print(", ");
      // Serial.print("acc_y = ");
      // Serial.print(BNO::acceralation_y);
      // measureData.push_back(BNO::acceralation_y);
      // Serial.print(", ");
      // Serial.print("acc_z = ");
      // Serial.print(BNO::acceralation_z);
      // measureData.push_back(BNO::acceralation_z);
      // Serial.print(", ");
      Serial.print("altitude = ");
      Serial.print(BME::altitude);
      measureData.push_back(BME::altitude);
      // Serial.print(", ");
      Serial.println("");

      if(measureData.size() > 10){
            for (double data : measureData) {
                  fp.print(data);
                  fp.print(",");
                  fp.println(data);
                  fp.close();
                  Serial.println("saved data");
                  while (1);
      }
      }
}