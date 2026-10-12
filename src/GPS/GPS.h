#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TinyGPS++.h>

namespace GPS{
    extern float latitude;
    extern float longitude;
    extern float x_dist;
    extern float y_dist;
    extern float dist;
    bool GPSbegin();
    void execute();
}