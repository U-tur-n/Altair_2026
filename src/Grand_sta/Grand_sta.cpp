#include <Arduino.h>
#include "../BNO/BNO.h"
#include "../GPS/GPS.h"
#include "../BME/BME.h"

namespace Grand_sta{
    float ax = BNO::ax;
    float ay = BNO::ay;
    float az = BNO::az;
    // float roll = BNO::roll;
    // float pich = BNO::pich;
    // float yaw = BNO::yaw;
}