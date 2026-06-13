#include "include/AngleTest.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include <math.h>

// ================================================================
// Private macros
// ================================================================
#define AT_BG       ILI9341_BLACK
#define AT_LABEL    ILI9341_CYAN
#define AT_VALUE    ILI9341_WHITE
#define AT_WARN     ILI9341_YELLOW

// Layout: labels at fixed x=10, values at x=VAL_X
// Each row height = 18 px (textSize 1 = 8px + 10px gap)
#define VAL_X       55      // x where numeric value starts
#define VAL_W       120     // width to erase before redrawing value
#define ROW_H       18

#define ROW_RAW_X   50      // y: Raw X
#define ROW_RAW_Y   (ROW_RAW_X + ROW_H)
#define ROW_RAW_Z   (ROW_RAW_Y + ROW_H)
#define ROW_G_X     (ROW_RAW_Z + ROW_H + 6)
#define ROW_G_Y     (ROW_G_X + ROW_H)
#define ROW_G_Z     (ROW_G_Y + ROW_H)
#define ROW_PITCH   (ROW_G_Z + ROW_H + 6)
#define ROW_ROLL    (ROW_PITCH + ROW_H)

// ================================================================
// Static helpers
// ================================================================

// Draw all static labels (called once)
static void drawATStaticBG() {
    tft.fillScreen(AT_BG);

    tft.setTextSize(2);
    tft.setTextColor(AT_LABEL, AT_BG);
    tft.setCursor(30, 10);
    tft.print(F("ANGLE TEST"));

    tft.setTextSize(1);
    tft.setTextColor(AT_LABEL, AT_BG);

    tft.setCursor(10, ROW_RAW_X); tft.print(F("Raw X:"));
    tft.setCursor(10, ROW_RAW_Y); tft.print(F("Raw Y:"));
    tft.setCursor(10, ROW_RAW_Z); tft.print(F("Raw Z:"));

    tft.setCursor(10, ROW_G_X);   tft.print(F("gX:"));
    tft.setCursor(10, ROW_G_Y);   tft.print(F("gY:"));
    tft.setCursor(10, ROW_G_Z);   tft.print(F("gZ:"));

    tft.setCursor(10, ROW_PITCH); tft.print(F("Pitch:"));
    tft.setCursor(10, ROW_ROLL);  tft.print(F("Roll:"));

    tft.setTextColor(AT_WARN, AT_BG);
    tft.setCursor(10, 290);
    tft.print(F("Press button to exit"));
}

// Erase + redraw a single numeric row
static void printValue(int16_t y, float val, uint8_t decimals) {
    tft.fillRect(VAL_X, y, VAL_W, 8, AT_BG);
    tft.setTextColor(AT_VALUE, AT_BG);
    tft.setTextSize(1);
    tft.setCursor(VAL_X, y);
    tft.print(val, decimals);
}

static void printValueInt(int16_t y, int16_t val) {
    tft.fillRect(VAL_X, y, VAL_W, 8, AT_BG);
    tft.setTextColor(AT_VALUE, AT_BG);
    tft.setTextSize(1);
    tft.setCursor(VAL_X, y);
    tft.print(val);
}

// ================================================================
// loopAngleTest – public entry (blocking)
// ================================================================
void loopAngleTest() {
    drawATStaticBG();

    while (true) {
        uint32_t fs = millis();
        updateHardware();

        if (isButtonPressed()) return;  // exit to main menu

        // ---- Read raw ADC ----
        int16_t rawX = (int16_t)analogRead(ACCEL_X_PIN);
        int16_t rawY = (int16_t)analogRead(ACCEL_Y_PIN);
        int16_t rawZ = (int16_t)analogRead(ACCEL_Z_PIN);

        // ---- Convert to g ----
        float gx = ((float)(rawX - ADXL_ZERO_DEFAULT)) / ADXL_COUNTS_PER_G;
        float gy = ((float)(rawY - ADXL_ZERO_DEFAULT)) / ADXL_COUNTS_PER_G;
        float gz = ((float)(rawZ - ADXL_ZERO_DEFAULT)) / ADXL_COUNTS_PER_G;

        // ---- Compute pitch & roll (degrees) ----
        // Pitch: rotation about Y-axis = atan2(gx, sqrt(gy²+gz²))
        // Roll:  rotation about X-axis = atan2(gy, sqrt(gx²+gz²))
        float pitch = atan2(gx, sqrt(gy * gy + gz * gz)) * (180.0f / M_PI);
        float roll  = atan2(gy, sqrt(gx * gx + gz * gz)) * (180.0f / M_PI);

        // ---- Update dynamic values only ----
        printValueInt(ROW_RAW_X, rawX);
        printValueInt(ROW_RAW_Y, rawY);
        printValueInt(ROW_RAW_Z, rawZ);

        printValue(ROW_G_X, gx, 3);
        printValue(ROW_G_Y, gy, 3);
        printValue(ROW_G_Z, gz, 3);

        printValue(ROW_PITCH, pitch, 1);
        printValue(ROW_ROLL,  roll,  1);

        FRAME_DELAY(fs);
    }
}
