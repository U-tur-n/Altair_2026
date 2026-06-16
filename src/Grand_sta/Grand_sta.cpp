#include <Arduino.h>
#include "../BNO/BNO.h"
#include "../GPS/GPS.h"
#include "../BME/BME.h"

namespace Grand_sta{
    float acclaration_x = BNO::acclaration_x;
    float acclaration_y = BNO::acclaration_y;
    float acclaration_z = BNO::acclaration_z;
    float roll = BNO::roll;
    float pich = BNO::pich;
    float yaw = BNO::yaw;
}