#include "include/AngleTest.h"
#include "Arduino.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
#include <stdint.h>
#include <stdio.h>

// 左上為負 右下為正
// int16: 存 analogRead 原始值 (0~1023)，int8 會截斷
static int16_t threeAxisData[3];
static int16_t mappedData[3];
static void drawAngleTestBackground();
static void updateThreeAxisData(const int8_t y, const char *);
static void updateDirectionData(const uint8_t y, const Direction dir);

/// 290 ~ 450
/// Mid -> 370
/// analogRead(Pin) 從小至大為 右前 -> 左後
/// 底下function改成左前為正又後為負 並把數值範圍改成 -50 ~ 50
Direction getDirectionForGame2048(uint8_t getThreeAxisSensorData) {
    if (getThreeAxisSensorData) {
        mappedData[0] = (-1) * MAPPING_THE_AXIS_RELATED_DATA(FETCH_DATA_OF_AXIS_X);
        mappedData[1] = MAPPING_THE_AXIS_RELATED_DATA(FETCH_DATA_OF_AXIS_Y);
    }
    if ((mappedData[0] > IDLE_RANGE || mappedData[0] < -IDLE_RANGE) ||
        (mappedData[1] > IDLE_RANGE || mappedData[1] < -IDLE_RANGE)) {
        if (mappedData[0] == mappedData[1]) return Direction::ERROR;

        // if -> x>y ,else -> y>x
        if (mappedData[0] * mappedData[0] > mappedData[1] * mappedData[1])
            return (mappedData[0] < 0) ? Direction::LEFT : Direction::RIGHT;
        else
            return (mappedData[1] < 0) ? Direction::UP : Direction::DOWN;

    } else
        return Direction::IDLE;
}
Direction getDirectionForGameCatch(uint8_t getThreeAxisSensorData) {
    if (getThreeAxisSensorData)
        mappedData[0] = (-1) * MAPPING_THE_AXIS_RELATED_DATA(FETCH_DATA_OF_AXIS_X);

    return (mappedData[0] < -IDLE_RANGE || mappedData[0] > IDLE_RANGE)
               ? (mappedData[0] > IDLE_RANGE) ? Direction::RIGHT : Direction::LEFT
               : Direction::IDLE;
}

void loopAngleTest() {
    drawAngleTestBackground();
    uint32_t fs = 0;
    while (1) {
        updateHardware();
        if (isButtonPressed()) return;
        if (millis() - fs < 100) continue; // 100ms cooldown

        threeAxisData[0] = FETCH_DATA_OF_AXIS_X;
        threeAxisData[1] = FETCH_DATA_OF_AXIS_Y;
        threeAxisData[2] = FETCH_DATA_OF_AXIS_Z;

        mappedData[0] = (-1) * MAPPING_THE_AXIS_RELATED_DATA(threeAxisData[0]);
        mappedData[1] = MAPPING_THE_AXIS_RELATED_DATA(threeAxisData[1]);
        mappedData[2] = MAPPING_THE_AXIS_RELATED_DATA(threeAxisData[2]);

        // 最長 "-50 470" = 7 字元 + '\0'（map 後 ±50，raw 實測不超過 470）
        char buffer[3][8];
        snprintf(buffer[0], 8, "%3d %d", mappedData[0], threeAxisData[0]);
        snprintf(buffer[1], 8, "%3d %d", mappedData[1], threeAxisData[1]);
        snprintf(buffer[2], 8, "%3d %d", mappedData[2], threeAxisData[2]);

        updateThreeAxisData(HEIGHT_OF_RAW_X, buffer[0]);
        updateThreeAxisData(HEIGHT_OF_RAW_Y, buffer[1]);
        updateThreeAxisData(HEIGHT_OF_RAW_Z, buffer[2]);

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
    tft.print(F("MAP & RAW of X:"));
    tft.setCursor(10, HEIGHT_OF_RAW_Y);
    tft.print(F("Map & RAW of Y:"));
    tft.setCursor(10, HEIGHT_OF_RAW_Z);
    tft.print(F("Map & RAW of Z:"));
    tft.drawFastHLine(0, THE_SAPERATE_LINE_FOR_DIR_AND_OTHERS, TFT_W, COLOR_GRAY);

    tft.setTextSize(2);
    tft.setCursor(10, HEIGHT_OF_SUBMENU);
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
    tft.fillRect(THE_DATA_X, y, VAL_W, 10, COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(THE_DATA_X, y);
    tft.print(str);
}
static void updateDirectionData(const uint8_t y, const Direction dir) {
    tft.fillRect(THE_DATA_X, y, VAL_W, 30, COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(THE_DATA_X, y);
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
}
