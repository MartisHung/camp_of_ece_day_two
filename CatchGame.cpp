#include "include/CatchGame.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
#include <math.h>

// ================================================================
// Private macros
// ================================================================
#define CG_BG ILI9341_BLACK
#define CG_PLAYER_CLR 0x07FFu // cyan
#define CG_ENEMY_CLR 0xF800u  // red
#define CG_HEALTH_CLR 0x07E0u // green
#define CG_COIN_CLR 0xFFE0u   // yellow
#define CG_TEXT_CLR ILI9341_WHITE
#define CG_HP_ICON_CLR 0xF800u // red hearts
#define CG_SCORE_X 140         // x for score display
#define CG_HP_X 10             // x for HP display

// Player movement sensitivity (g threshold for left/right)
#define CG_MOVE_THRESH 0.25f
// Player speed in pixels per frame per unit of g above threshold
#define CG_MOVE_SPEED 4

// ================================================================
// Private types
// ================================================================
struct FallingItem {
    int16_t x;
    int16_t y;
    ItemType type;
    bool active;
};

// ================================================================
// Private state
// ================================================================
static FallingItem items[CG_MAX_ITEMS];
static int16_t playerX; // player left edge x
static int8_t playerHP;
static uint16_t cgScore;
static CatchState cgState;
static uint32_t lastSpawnTime;
static uint16_t spawnInterval; // ms between spawns (decreases with score)
static uint8_t fallSpeed;      // px per frame (increases with score)

// ================================================================
// Private function prototypes
// ================================================================
static void resetCatchGame();
static void drawCGStaticBG();
static void drawHUD();
static void drawPlayer(int16_t oldX, int16_t newX);
static void drawItem(const FallingItem &it);
static void eraseItem(const FallingItem &it);
static void spawnItem();
static void updateItems();
static bool checkCollision(const FallingItem &it);
static void handleCollision(FallingItem &it);
static void updateDifficulty();
static void drawCalibScreen();
static void drawGameOverScreen();
static float readPlayerTilt();

// ================================================================
// loopCatchGame – public entry (blocking)
// ================================================================
void loopCatchGame() {
    // ---- Calibration phase ----
    cgState = CatchState::CS_CALIBRATE;
    drawCalibationScreen();

    while (cgState == CatchState::CS_CALIBRATE) {
        uint32_t fs = millis();
        updateHardware();
        if (isButtonPressed()) {
            captureBaseline();
            cgState = CatchState::CS_PLAYING;
        }
        FRAME_DELAY(fs);
    }

    // ---- Game init ----
    resetCatchGame();
    drawCGStaticBG();
    drawHUD();
    drawPlayer(-1, playerX); // -1 = no old position to erase

    // ---- Main game loop ----
    while (true) {
        uint32_t fs = millis();
        updateHardware();

        // Button press exits to main menu at any time
        if (isButtonPressed()) return;

        if (cgState == CatchState::CS_PLAYING) {
            // ---- Player movement via tilt ----
            float gx = readPlayerTilt();
            int16_t oldPX = playerX;

            if (gx > CG_MOVE_THRESH) {
                // Tilted right → move right
                playerX += CG_MOVE_SPEED;
            } else if (gx < -CG_MOVE_THRESH) {
                // Tilted left → move left
                playerX -= CG_MOVE_SPEED;
            }

            // Clamp to screen bounds
            if (playerX < 0) playerX = 0;
            if (playerX > TFT_W - CG_PLAYER_W) playerX = TFT_W - CG_PLAYER_W;

            // Redraw player only if moved
            if (static_cast<int16_t>(playerX ^ oldPX)) drawPlayer(oldPX, playerX);

            // ---- Spawn new items ----
            if (millis() - lastSpawnTime >= spawnInterval) {
                spawnItem();
                lastSpawnTime = millis();
            }

            // ---- Update falling items ----
            updateItems();

            // ---- Check game over ----
            if (playerHP <= 0) {
                cgState = CatchState::CS_GAMEOVER;
                drawGameOverScreen();
            }
        }
        // CS_GAMEOVER: just wait for button (handled above)

        FRAME_DELAY(fs);
    }
}

// ================================================================
// Static helpers
// ================================================================

static void resetCatchGame() {
    playerX = (TFT_W - CG_PLAYER_W) / 2;
    playerHP = CG_HP_INIT;
    cgScore = 0;
    lastSpawnTime = millis();
    spawnInterval = CG_SPAWN_MS;
    fallSpeed = CG_FALL_SPEED;

    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) items[i].active = false;
}

static void drawCGStaticBG() {
    tft.fillScreen(CG_BG);

    // HUD separator line
    tft.drawFastHLine(0, CG_PLAY_TOP - 2, TFT_W, 0x7BEFu);
}

static void drawHUD() {
    // Erase HUD area
    tft.fillRect(0, 0, TFT_W, CG_PLAY_TOP - 3, CG_BG);

    // HP: draw hearts
    tft.setTextSize(1);
    tft.setTextColor(CG_HP_ICON_CLR, CG_BG);
    tft.setCursor(CG_HP_X, CG_HUD_Y);
    tft.print(F("HP:"));
    for (int8_t i = 0; i < playerHP; i++) {
        // Small filled heart (approximated with a character)
        tft.fillRect(CG_HP_X + 22 + i * 12, CG_HUD_Y, 8, 8, CG_HP_ICON_CLR);
    }
    // Erase extra hearts if HP decreased
    for (int8_t i = playerHP; i < CG_HP_MAX; i++) {
        tft.fillRect(CG_HP_X + 22 + i * 12, CG_HUD_Y, 8, 8, CG_BG);
    }

    // Score
    tft.setTextColor(CG_COIN_CLR, CG_BG);
    tft.setCursor(CG_SCORE_X, CG_HUD_Y);
    tft.print(F("SCORE:"));
    tft.fillRect(CG_SCORE_X + 40, CG_HUD_Y, 60, 8, CG_BG);
    tft.setCursor(CG_SCORE_X + 40, CG_HUD_Y);
    tft.print(cgScore);
}

// ---- Player drawing ----
static void drawPlayer(int16_t oldX, int16_t newX) {
    // Erase old position (skip if oldX == -1, meaning first draw)
    if (oldX >= 0) {
        tft.fillRect(oldX, CG_PLAYER_Y, CG_PLAYER_W, CG_PLAYER_H, CG_BG);
    }
    // Draw new player: body
    tft.fillRect(newX, CG_PLAYER_Y, CG_PLAYER_W, CG_PLAYER_H, CG_PLAYER_CLR);
    // Eyes (2 dark dots near top)
    tft.fillRect(newX + 5, CG_PLAYER_Y + 3, 3, 3, CG_BG);
    tft.fillRect(newX + 12, CG_PLAYER_Y + 3, 3, 3, CG_BG);
}

// ---- Item drawing ----
static void drawItem(const FallingItem &it) {
    uint16_t clr;
    switch (it.type) {
    case ItemType::IT_ENEMY:
        clr = CG_ENEMY_CLR;
        break;
    case ItemType::IT_HEALTH:
        clr = CG_HEALTH_CLR;
        break;
    case ItemType::IT_COIN:
        clr = CG_COIN_CLR;
        break;
    default:
        clr = CG_TEXT_CLR;
        break;
    }
    tft.fillRect(it.x, it.y, CG_ITEM_SZ, CG_ITEM_SZ, clr);

    // Draw icon indicator inside the item
    switch (it.type) {
    case ItemType::IT_ENEMY:
        // X mark
        tft.drawLine(it.x + 2, it.y + 2, it.x + CG_ITEM_SZ - 3, it.y + CG_ITEM_SZ - 3, CG_BG);
        tft.drawLine(it.x + CG_ITEM_SZ - 3, it.y + 2, it.x + 2, it.y + CG_ITEM_SZ - 3, CG_BG);
        break;
    case ItemType::IT_HEALTH:
        // + mark
        tft.drawFastHLine(it.x + 2, it.y + CG_ITEM_SZ / 2, CG_ITEM_SZ - 4, CG_BG);
        tft.drawFastVLine(it.x + CG_ITEM_SZ / 2, it.y + 2, CG_ITEM_SZ - 4, CG_BG);
        break;
    case ItemType::IT_COIN:
        // $ circle
        tft.drawCircle(it.x + CG_ITEM_SZ / 2, it.y + CG_ITEM_SZ / 2, 3, CG_BG);
        break;
    }
}

static void eraseItem(const FallingItem &it) {
    tft.fillRect(it.x, it.y, CG_ITEM_SZ, CG_ITEM_SZ, CG_BG);
}

// ---- Spawn a new item in a free slot ----
static void spawnItem() {
    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) {
        if (static_cast<uint8_t>(items[i].active) ^ 1) {
            items[i].active = true;
            items[i].x = (int16_t)random(0, TFT_W - CG_ITEM_SZ);
            items[i].y = CG_PLAY_TOP;

            // Weighted random: 50% enemy, 15% health, 35% coin
            uint8_t roll = (uint8_t)random(100);
            if (roll < 50)
                items[i].type = ItemType::IT_ENEMY;
            else if (roll < 65)
                items[i].type = ItemType::IT_HEALTH;
            else
                items[i].type = ItemType::IT_COIN;

            return; // only spawn one per call
        }
    }
    // All slots full – skip this spawn cycle
}

// ---- Update all falling items (move + collision) ----
static void updateItems() {
    bool hudDirty = false;

    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) {
        if (static_cast<uint8_t>(items[i].active) ^ 1) continue;

        // Erase at old position
        eraseItem(items[i]);

        // Move down
        items[i].y += fallSpeed;

        // Off-screen: deactivate
        if (items[i].y > TFT_H) {
            items[i].active = false;
            continue;
        }

        // Collision with player?
        if (checkCollision(items[i])) {
            handleCollision(items[i]);
            items[i].active = false;
            hudDirty = true;
            continue;
        }

        // Draw at new position
        drawItem(items[i]);
    }

    if (hudDirty) {
        drawHUD();
        updateDifficulty();
    }
}

// ---- Collision: AABB overlap ----
static bool checkCollision(const FallingItem &it) {
    // Player rect: (playerX, CG_PLAYER_Y, CG_PLAYER_W, CG_PLAYER_H)
    // Item rect:   (it.x, it.y, CG_ITEM_SZ, CG_ITEM_SZ)
    if (it.x + CG_ITEM_SZ <= playerX) return false;
    if (it.x >= playerX + CG_PLAYER_W) return false;
    if (it.y + CG_ITEM_SZ <= CG_PLAYER_Y) return false;
    if (it.y >= CG_PLAYER_Y + CG_PLAYER_H) return false;
    return true;
}

// ---- Apply item effect ----
static void handleCollision(FallingItem &it) {
    switch (it.type) {
    case ItemType::IT_ENEMY:
        playerHP--;
        break;
    case ItemType::IT_HEALTH:
        if (playerHP < CG_HP_MAX) playerHP++;
        break;
    case ItemType::IT_COIN:
        cgScore += CG_COIN_SCORE;
        break;
    }
}

// ---- Difficulty scaling ----
static void updateDifficulty() {
    // Every 50 points: speed +1, spawn interval -50ms (min 300ms)
    uint8_t level = (uint8_t)(cgScore / 50);
    fallSpeed = CG_FALL_SPEED + level;
    if (fallSpeed > 8) fallSpeed = 8; // cap

    spawnInterval = CG_SPAWN_MS - (uint16_t)level * 50;
    if (spawnInterval < 300) spawnInterval = 300; // floor
}

// ---- Read accelerometer X-axis tilt as g value ----
static float readPlayerTilt() {
    return ACCEL_TO_G(ACCEL_RAW_X(), getBaseRawX());
}

/*// ---- Calibration screen ----
static void drawCalibScreen() {
    tft.fillScreen(CG_BG);
    tft.setTextColor(CG_TEXT_CLR, CG_BG);
    tft.setTextSize(2);
    tft.setCursor(20, 60);
    tft.print(F("CALIBRATION"));
    tft.setTextSize(1);
    tft.setCursor(10, 110);
    tft.print(F("Hold device flat"));
    tft.setCursor(10, 125);
    tft.print(F("in your play position,"));
    tft.setCursor(10, 145);
    tft.print(F("then press button."));
}*/

// ---- Game over screen ----
static void drawGameOverScreen() {
    // Erase play area center
    tft.fillRect(20, 120, 200, 80, CG_BG);

    tft.setTextSize(2);
    tft.setTextColor(CG_ENEMY_CLR, CG_BG);
    tft.setCursor(40, 130);
    tft.print(F("GAME OVER"));

    tft.setTextSize(1);
    tft.setTextColor(CG_COIN_CLR, CG_BG);
    tft.setCursor(60, 160);
    tft.print(F("Score: "));
    tft.print(cgScore);

    tft.setTextColor(CG_TEXT_CLR, CG_BG);
    tft.setCursor(30, 185);
    tft.print(F("Press button to exit"));
}
