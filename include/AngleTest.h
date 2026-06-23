#ifndef ANGLE_TEST_H
#define ANGLE_TEST_H
#include "Globals.h"
#include <stdint.h>

#define ANGLE_MIN 290
#define ANGLE_MAX 450



void loopAngleTest();
Direction getDirectionForGameCatch(uint8_t getThreeAxisSensorData),
    getDirectionForGame2048(uint8_t getThreeAxisSensorData);
#endif // ANGLE_TEST_H
