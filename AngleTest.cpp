#include "include/AngleTest.h"
#include "Arduino.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
#include <stdint.h>
#include <stdio.h>

constexpr uint8_t VAL_X = 55, VAL_W = 120, GAPS_BETWEEN_ROWS = 20, DIRECTION_DATA_X = 100;
constexpr uint8_t IDLE_RANGE = 15, THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS = 160;
constexpr uint8_t HEIGHT_OF_RAW_X = 80, HEIGHT_OF_RAW_Y = HEIGHT_OF_RAW_X + GAPS_BETWEEN_ROWS,
                  HEIGHT_OF_RAW_Z = HEIGHT_OF_RAW_X + 2 * GAPS_BETWEEN_ROWS;
constexpr uint8_t HEIGHT_OF_SUBMENU = THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 1 * GAPS_BETWEEN_ROWS,
                  HEIGHT_OF_2048 = THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 2 * GAPS_BETWEEN_ROWS,
                  HEIGHT_OF_CATCH_GAME =
                      THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS + 3 * GAPS_BETWEEN_ROWS;

// 左上為負 右下為正
static int8_t threeAxisData[3];
static void drawAngleTestBackground(), updateThreeAxisData(const int8_t y, const char *),
    updateDirectionData(const uint8_t y, const Direction dir);

// 290 ~ 450
// Mid -> 370
// 右前 -> 左後
Direction getDirectionForGame2048(uint8_t getThreeAxisSensorData) {
    if (getThreeAxisSensorData) {
        *(threeAxisData + 0) =
            static_cast<int8_t>((-1) * map(analogRead(ADXL_X), 290, 450, -50, 50));
        *(threeAxisData + 1) = static_cast<int8_t>(map(analogRead(ADXL_Y), 290, 450, -50, 50));
    }
    if ((*(threeAxisData + 0) > IDLE_RANGE || *(threeAxisData + 0) < -IDLE_RANGE) ||
        (*(threeAxisData + 1) > IDLE_RANGE || *(threeAxisData + 1) < -IDLE_RANGE)) {
        if (*(threeAxisData + 0) == *(threeAxisData + 1)) return Direction::ERROR;

        // if -> x>y ,else y->x
        if (*(threeAxisData + 0) * *(threeAxisData + 0) >
            *(threeAxisData + 1) * *(threeAxisData + 1))
            return (*(threeAxisData + 0) < 0) ? Direction::LEFT : Direction::RIGHT;
        else
            return (*(threeAxisData + 1) < 0) ? Direction::UP : Direction::DOWN;

    } else
        return Direction::IDLE;
}
Direction getDirectionForGameCatch(uint8_t getThreeAxisSensorData) {
    if (getThreeAxisSensorData)
        *(threeAxisData + 0) =
            static_cast<int8_t>((-1) * map(analogRead(ADXL_X), 290, 450, -50, 50));

    return (*(threeAxisData + 0) < -IDLE_RANGE || *(threeAxisData + 0) > IDLE_RANGE)
               ? (*(threeAxisData + 0) > IDLE_RANGE) ? Direction::RIGHT : Direction::LEFT
               : Direction::IDLE;
}

void loopAngleTest() {
    drawAngleTestBackground();
    uint32_t fs;
    while (1) {
        updateHardware();
        if (isButtonPressed()) return;
        if (millis() - fs < 100) continue;

        // 把數值改成左上為負 右下為正
        *(threeAxisData + 0) =
            static_cast<int8_t>((-1) * map(analogRead(ADXL_X), 290, 450, -50, 50));
        *(threeAxisData + 1) = static_cast<int8_t>(map(analogRead(ADXL_Y), 290, 450, -50, 50));
        *(threeAxisData + 2) = static_cast<int8_t>(map(analogRead(ADXL_Z), 290, 450, -50, 50));

        char buffer[3][4];
        snprintf(*(buffer + 0), 4, "%d", *(threeAxisData + 0));
        snprintf(*(buffer + 1), 4, "%d", *(threeAxisData + 1));
        snprintf(*(buffer + 2), 4, "%d", *(threeAxisData + 2));
        updateThreeAxisData(HEIGHT_OF_RAW_X, *(buffer + 0));
        updateThreeAxisData(HEIGHT_OF_RAW_Y, *(buffer + 1));
        updateThreeAxisData(HEIGHT_OF_RAW_Z, *(buffer + 2));

        updateDirectionData(HEIGHT_OF_2048, getDirectionForGame2048(0));
        updateDirectionData(HEIGHT_OF_CATCH_GAME, getDirectionForGameCatch(0));
        fs = millis();
    }
}

static void drawAngleTestBackground() {
    tft.fillScreen(COLOR_BLACK);

    tft.setTextSize(2);
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setCursor(30, 10);
    tft.print(F("ANGLE TEST"));
    tft.drawFastHLine(0, HIGHT_OF_THE_HORIZONTIAL_SAPERATE_LINE_FOR_TITLE_AND_DATAS, TFT_W,
                      COLOR_GRAY);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);

    tft.setCursor(10, HEIGHT_OF_RAW_X);
    tft.print(F("Raw X:"));
    tft.setCursor(10, HEIGHT_OF_RAW_Y);
    tft.print(F("Raw Y:"));
    tft.setCursor(10, HEIGHT_OF_RAW_Z);
    tft.print(F("Raw Z:"));

    tft.drawFastHLine(0, THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS, TFT_W, COLOR_GRAY);
    
    tft.setTextSize(2);
    tft.setCursor(10,HEIGHT_OF_SUBMENU);
    tft.setTextSize(1);
    tft.print(F("The Direction Of Games:"));
    tft.setCursor(10, HEIGHT_OF_2048);
    tft.print(F("2048  ->"));
    tft.setCursor(10, HEIGHT_OF_CATCH_GAME);
    tft.print(F("Catch ->"));

    tft.setTextColor(COLOR_YELLOW, COLOR_BLACK);
    tft.setCursor(10, 290);
    tft.print(F("Press button to exit"));
}

// y -> the one which is changing (HEIGHT_OF_RAW_{X,Y,Z}這邊我高度都改差不多了)
// *str -> 輸入得字串 int to str 再上層函數就該弄完
static void updateThreeAxisData(const int8_t y, const char *str) {
    tft.fillRect(VAL_X, y, VAL_W, 10, COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(VAL_X, y);
    tft.print(str);
}
static void updateDirectionData(const uint8_t y, const Direction dir) {
    static Direction o_dir;
    tft.fillRect(DIRECTION_DATA_X, y, VAL_W, 30, COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(DIRECTION_DATA_X, y);
    switch (dir) {
    case Direction::IDLE:
        tft.print(F("IDLE"));
        break;
    case Direction::UP:
        tft.print(F("UP"));
        break;
    case Direction::DOWN:
        tft.print(F("DOWN"));
        break;
    case Direction::LEFT:
        tft.print(F("LEFT"));
        break;
    case Direction::RIGHT:
        tft.print(F("RIGHT"));
        break;
    case Direction::ERROR:
        tft.print(F("ERROR"));
        break;
    }
    o_dir = dir;
}
