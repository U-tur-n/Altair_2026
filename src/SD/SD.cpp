#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <vector>

namespace SD{
      std::vector<float> measureData;

      Serial.print(millis());
      measureData.push_back(millis());
      Serial.print(" --> ");
      Serial.print("deg = ");
      Serial.println(degResult);
      Serial.print(millis());
      Serial.print(" --> ");
      Serial.print("speed = ");
      Serial.println(speed);
      measureData.push_back(speed);
      Serial.println("");
      times_Display = millis();
}