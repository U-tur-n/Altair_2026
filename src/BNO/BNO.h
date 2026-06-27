#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>

namespace BNO{
  
    extern float ax;
    extern float ay;
    extern float az;
    extern float q;

  void begin();
  void execute();

}