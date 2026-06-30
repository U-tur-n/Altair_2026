#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TinyGPS++.h>

namespace GPS{
    extern float latitude;
    extern float longitude;
    bool GPSbegin();
    void execute();
}