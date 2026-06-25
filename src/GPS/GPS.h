#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TinyGPS++.h>

namespace GPS{
    extern float ;
    void begin();
    void execute();
}