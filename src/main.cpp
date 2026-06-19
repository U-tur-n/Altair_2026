#include <Arduino.h>
#include "BME/BME.h"

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  BME::begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  BME::execute();
}

// put function definitions here: