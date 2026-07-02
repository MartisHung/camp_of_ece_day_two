#ifndef ANGLE_TEST_H
#define ANGLE_TEST_H
#include "Globals.h"
#include "HardwareManager.h"
#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t VAL_W = 120, GAPS_BETWEEN_ROWS = 20, DIRECTION_DATA_X = 100;

constexpr uint8_t IDLE_RANGE = 15, THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS = 160;

constexpr uint8_t HEIGHT_OF_RAW_X = 80, HEIGHT_OF_RAW_Y = HEIGHT_OF_RAW_X + GAPS_BETWEEN_ROWS,
                  HEIGHT_OF_RAW_Z = HEIGHT_OF_RAW_X + 2 * GAPS_BETWEEN_ROWS;

constexpr uint16_t HEIGHT_OF_SUBMENU = THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 1 * GAPS_BETWEEN_ROWS,
                   HEIGHT_OF_2048 = THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 2 * GAPS_BETWEEN_ROWS,
                   HEIGHT_OF_CATCH_GAME =
                       THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 3 * GAPS_BETWEEN_ROWS;

constexpr uint16_t ANGLE_MIN = 300, ANGLE_MAX = 450;

// 把數值改成左上為負 右下為正
#define FETCH_DATA_OF_AXIS_X analogRead(ADXL_X)
#define FETCH_DATA_OF_AXIS_Y analogRead(ADXL_Y)
#define FETCH_DATA_OF_AXIS_Z analogRead(ADXL_Z)
#define MAPPING_THE_AXIS_RELATED_DATA(rawData)                                                              \
    static_cast<int8_t>(map(rawData, ANGLE_MIN, ANGLE_MAX, -50, 50))

void loopAngleTest();
Direction getDirectionForGameCatch(uint8_t getThreeAxisSensorData);
Direction getDirectionForGame2048(uint8_t getThreeAxisSensorData);

#endif // ANGLE_TEST_H
