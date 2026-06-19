#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "BME.cpp"

namespace BME{
    extern float altitude;
    void begin();
    void execute();
}