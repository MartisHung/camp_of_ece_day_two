#ifndef ANGLE_TEST_H
#define ANGLE_TEST_H
#include "Globals.h"
#include <stdint.h>

void loopAngleTest();
Direction getDirectionForGameCatch(uint8_t getThreeAxisSensorData);
Direction getDirectionForGame2048(uint8_t getThreeAxisSensorData);
#endif // ANGLE_TEST_H
