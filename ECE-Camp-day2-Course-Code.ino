#include "include/AngleTest.h"
#include "include/CatchGame.h"
#include "include/Game2048.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
// ================================================================
// Main Menu Configuration
// ================================================================
#define MENU_ITEMS        3 // 0=2048, 1=Angle, 2=Catch
#define MENU_TITLE_Y      10
#define MENU_ITEM_Y0      80 // y of first menu item
#define MENU_ITEM_DY      50 // vertical spacing
#define MENU_CURSOR_X     14
#define MENU_TEXT_X       30
#define TITLE_COLOR       COLOR_CYAN
#define ITEM_COLOR        COLOR_WHITE
#define CURSOR_COLOR      COLOR_GREEN
#define FUTHERINFO_COLOR  COLOR_GRAY
#define BACKGROUND_COLOR  COLOR_BLACK

// ================================================================
// Private helpers (main menu only – not shared, so stay in .ino)
// ================================================================
static int8_t menuCursor = 0;

// Draw the static background of the main menu (call once on entry)
static void drawMenuBG() {
    tft.fillScreen(BACKGROUND_COLOR);

    tft.setTextSize(2);
    tft.setTextColor(TITLE_COLOR, BACKGROUND_COLOR);
    tft.setCursor(20, MENU_TITLE_Y);
    tft.print(F("ECE  CAMP"));

    tft.setTextSize(1);
    tft.setTextColor(FUTHERINFO_COLOR, BACKGROUND_COLOR); // grey subtitle
    tft.setCursor(50, 38);
    tft.print(F("Day 2 Project"));

    // Divider line
    tft.drawFastHLine(0, 55, TFT_W, FUTHERINFO_COLOR);

    // Static item labels
    tft.setTextSize(2);
    tft.setTextColor(ITEM_COLOR, BACKGROUND_COLOR);
    tft.setCursor(MENU_TEXT_X, MENU_ITEM_Y0);
    tft.print(F("1. Angle Test"));
    tft.setCursor(MENU_TEXT_X, MENU_ITEM_Y0 + MENU_ITEM_DY);
    tft.print(F("2. 2048 Game"));
    tft.setCursor(MENU_TEXT_X, MENU_ITEM_Y0 + 2 * MENU_ITEM_DY);
    tft.print(F("3. Catch Game"));

    // Hint at bottom
    tft.setTextColor(FUTHERINFO_COLOR, BACKGROUND_COLOR);
    tft.setCursor(10, 290);
    tft.print(F("Rotate=select \n Press=enter"));
}

// Erase old cursor, draw at new position
static void drawMenuCursor(int8_t oldPos, int8_t newPos) {
    // Erase old
    tft.fillRect(MENU_CURSOR_X, MENU_ITEM_Y0 + oldPos * MENU_ITEM_DY, 8, 8, BACKGROUND_COLOR);
    // Draw new
    tft.fillRect(MENU_CURSOR_X, MENU_ITEM_Y0 + newPos * MENU_ITEM_DY, 8, 8, CURSOR_COLOR);
}

// ================================================================
// setup / loop
// ================================================================
void setup() {
    initHardware();
    enableEncoderISR(); // start encoder interrupt on main menu
    Serial.println(F("[MENU] drawMenuBG"));
    drawMenuBG();
    drawMenuCursor(0, menuCursor);
    Serial.println(F("[MENU] ready"));
}

void loop() {
    uint32_t fs = millis();
    updateHardware();

    // ---- Encoder navigation ----
    int8_t enc = getEncoderDelta();
    if (enc) {
        int8_t oldCursor = menuCursor;
        menuCursor += enc;
        // menuCursor %= MENU_ITEMS; // wrap around
        if (menuCursor < 0) menuCursor = 0;
        if (menuCursor >= MENU_ITEMS) menuCursor = MENU_ITEMS - 1;
        if (static_cast<int8_t>(menuCursor ^ oldCursor)) drawMenuCursor(oldCursor, menuCursor);
    }

    // ---- Button: enter selected page ----
    if (isButtonPressed()) {
        disableEncoderISR(); // stop ISR while in sub-page
        switch (menuCursor) {
        case 0:
            Serial.println(F("[MENU] -> loopAngleTest"));
            loopAngleTest(); // blocking; returns on exit
            Serial.println(F("[MENU] <- loopAngleTest"));
            break;
        case 1:
            Serial.println(F("[MENU] -> loopGame2048"));
            enterGame2048(); // blocking; returns on exit
            Serial.println(F("[MENU] <- loopGame2048"));
            break;
        case 2:
            Serial.println(F("[MENU] -> loopCatchGame"));
            enterCatchGame();
            Serial.println(F("[MENU] <- loopCatchGame"));
            break;
        default:
            break;
        }
        // Returned from sub-page: redraw main menu and restart ISR
        drawMenuBG();
        drawMenuCursor(0, menuCursor);
        enableEncoderISR();
    }

    FRAME_DELAY(fs);
}

/// Martis : @JunePigTseng 你寫得code 讓我好想blame你...
