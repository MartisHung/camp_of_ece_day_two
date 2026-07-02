#include "include/CatchGame.h"
#include "include/AngleTest.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
#include <Arduino.h>

constexpr uint16_t BG = ILI9341_BLACK;
constexpr uint16_t PLAYER_CLR = 0x07FFu;
constexpr uint16_t ENEMY_CLR = 0xF800u;
constexpr uint16_t HEALTH_CLR = 0x07E0u;
constexpr uint16_t COIN_CLR = 0xFFE0u;
constexpr uint16_t TEXT_CLR = ILI9341_WHITE;
constexpr uint16_t HP_ICON_CLR = 0xF800u;
constexpr uint8_t SCORE_X = 140;
constexpr uint8_t HP_X = 10;

static FallingItem items[CG_MAX_ITEMS];
static int16_t playerX;
static int8_t playerHP;
static uint16_t score;
static uint32_t lastSpawnTime;
static uint16_t spawnInterval;
static uint8_t fallSpeed;

static void calibrate();
static void catchGameInit();
static void gameLoop();

static void drawStaticBG();
static void drawHUDStatic();
static void updateHPDisplay(int8_t oldHP, int8_t newHP);
static void drawScore();
static void drawPlayer(int16_t oldX, int16_t newX);
static void drawItem(const FallingItem &it);
static void eraseItem(const FallingItem &it);
static void spawnItem();
static void updateItems();
static uint8_t checkCollision(const FallingItem &it);
static void handleCollision(FallingItem &it);
static void updateDifficulty();
static void drawGameOverScreen();

void enterCatchGame() {
    calibrate();
    catchGameInit();
    gameLoop();
}

static void calibrate() {
    drawCalibationScreen();
    while (true) {
        updateHardware();
        if (isButtonPressed()) break;
    }
}

static void catchGameInit() {
    playerX = (TFT_W - CG_PLAYER_W) / 2;
    playerHP = CG_HP_INIT;
    score = 0;
    lastSpawnTime = millis();
    spawnInterval = CG_SPAWN_MS;
    fallSpeed = CG_FALL_SPEED;

    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) items[i].active = 0;

    drawStaticBG();
    drawHUDStatic();
    drawPlayer(-1, playerX);
}

static void gameLoop() {
    while (true) {
        uint32_t fs = millis();
        updateHardware();

        if (isButtonPressed()) return;

        // ---- movement ----
        const Direction dir = getDirectionForGameCatch(1);
        const int16_t oldPX = playerX;

        if (dir == Direction::LEFT)
            playerX -= CG_MOVE_SPEED;
        else if (dir == Direction::RIGHT)
            playerX += CG_MOVE_SPEED;

        if (playerX < 0) playerX = 0;
        if (playerX > TFT_W - CG_PLAYER_W) playerX = TFT_W - CG_PLAYER_W;

        if (playerX ^ oldPX) drawPlayer(oldPX, playerX);

        // ---- spawn + update ----
        if (millis() - lastSpawnTime >= spawnInterval + 20) {
            spawnItem();
            lastSpawnTime = millis();
        }
        updateItems();

        // ---- game over check ----
        if (playerHP <= 0) {
            drawGameOverScreen();
            while (!isButtonPressed()) updateHardware();
            return;
        }

        FRAME_DELAY(fs);
    }
}

static void drawStaticBG() {
    tft.fillScreen(BG);
    tft.drawFastHLine(0, CG_PLAY_TOP - 2, TFT_W, 0x7BEFu);
}

// Labels drawn once at init; per-frame updates only touch the changed cells.
static void drawHUDStatic() {
    tft.setTextSize(1);
    tft.setTextColor(HP_ICON_CLR, BG);
    tft.setCursor(HP_X, CG_HUD_Y);
    tft.print(F("HP:"));
    updateHPDisplay(0, playerHP);

    tft.setTextColor(COIN_CLR, BG);
    tft.setCursor(SCORE_X, CG_HUD_Y);
    tft.print(F("SCORE:"));
    drawScore();
}

// Redraws only the HP cells between oldHP and newHP.
static void updateHPDisplay(const int8_t oldHP, const int8_t newHP) {
    if (oldHP == newHP) return;
    const int8_t lo = min(oldHP, newHP);
    const int8_t hi = max(oldHP, newHP);
    const uint16_t clr = (newHP > oldHP) ? HP_ICON_CLR : BG;
    for (int8_t i = lo; i < hi; i++) tft.fillRect(HP_X + 22 + i * 12, CG_HUD_Y, 8, 8, clr);
}

// Score only grows, so printing with a bg color overwrites cleanly; no fillRect needed.
static void drawScore() {
    tft.setTextSize(1);
    tft.setTextColor(COIN_CLR, BG);
    tft.setCursor(SCORE_X + 40, CG_HUD_Y);
    tft.print(score);
}

static void drawPlayer(const int16_t oldX, const int16_t newX) {
    if (oldX >= 0) tft.fillRect(oldX, CG_PLAYER_Y, CG_PLAYER_W, CG_PLAYER_H, BG);
    tft.fillRect(newX, CG_PLAYER_Y, CG_PLAYER_W, CG_PLAYER_H, PLAYER_CLR);
    tft.fillRect(newX + 5, CG_PLAYER_Y + 3, 3, 3, BG);
    tft.fillRect(newX + 12, CG_PLAYER_Y + 3, 3, 3, BG);
}

static void drawItem(const FallingItem &it) {
    uint16_t clr;
    switch (it.type) {
    case ItemType::IT_ENEMY:
        clr = ENEMY_CLR;
        break;
    case ItemType::IT_HEALTH:
        clr = HEALTH_CLR;
        break;
    case ItemType::IT_COIN:
        clr = COIN_CLR;
        break;
    default:
        clr = TEXT_CLR;
        break;
    }
    tft.fillRect(it.x, it.y, CG_ITEM_SZ, CG_ITEM_SZ, clr);

    switch (it.type) {
    case ItemType::IT_ENEMY:
        tft.drawLine(it.x + 2, it.y + 2, it.x + CG_ITEM_SZ - 3, it.y + CG_ITEM_SZ - 3, BG);
        tft.drawLine(it.x + CG_ITEM_SZ - 3, it.y + 2, it.x + 2, it.y + CG_ITEM_SZ - 3, BG);
        break;
    case ItemType::IT_HEALTH:
        tft.drawFastHLine(it.x + 2, it.y + CG_ITEM_SZ / 2, CG_ITEM_SZ - 4, BG);
        tft.drawFastVLine(it.x + CG_ITEM_SZ / 2, it.y + 2, CG_ITEM_SZ - 4, BG);
        break;
    case ItemType::IT_COIN:
        tft.drawCircle(it.x + CG_ITEM_SZ / 2, it.y + CG_ITEM_SZ / 2, 3, BG);
        break;
    }
}

static void eraseItem(const FallingItem &it) {
    tft.fillRect(it.x, it.y, CG_ITEM_SZ, CG_ITEM_SZ, BG);
}

static void spawnItem() {
    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) {
        if (items[i].active ^ 1) {
            items[i].active = 1;
            items[i].x = (int16_t)random(0, TFT_W - CG_ITEM_SZ);
            items[i].y = CG_PLAY_TOP;

            const uint8_t roll = (uint8_t)random(100);
            if (roll < 50)
                items[i].type = ItemType::IT_ENEMY;
            else if (roll < 65)
                items[i].type = ItemType::IT_HEALTH;
            else
                items[i].type = ItemType::IT_COIN;
            return;
        }
    }
}

static void updateItems() {
    const int8_t oldHP = playerHP;
    const uint16_t oldScore = score;

    for (uint8_t i = 0; i < CG_MAX_ITEMS; i++) {
        if (items[i].active ^ 1) continue;

        eraseItem(items[i]);
        items[i].y += fallSpeed;

        if (items[i].y > TFT_H) {
            items[i].active = 0;
            continue;
        }
        if (checkCollision(items[i])) {
            handleCollision(items[i]);
            items[i].active = 0;
            continue;
        }
        drawItem(items[i]);
    }

    if (playerHP != oldHP) updateHPDisplay(oldHP, playerHP);
    if (score != oldScore) {
        drawScore();
        updateDifficulty();
    }
}

static uint8_t checkCollision(const FallingItem &it) {
    if (it.x + CG_ITEM_SZ <= playerX) return 0;
    if (it.x >= playerX + CG_PLAYER_W) return 0;
    if (it.y + CG_ITEM_SZ <= CG_PLAYER_Y) return 0;
    if (it.y >= CG_PLAYER_Y + CG_PLAYER_H) return 0;
    return 1;
}

static void handleCollision(FallingItem &it) {
    switch (it.type) {
    case ItemType::IT_ENEMY:
        playerHP--;
        break;
    case ItemType::IT_HEALTH:
        if (playerHP < CG_HP_MAX) playerHP++;
        break;
    case ItemType::IT_COIN:
        score += CG_COIN_SCORE;
        break;
    }
}

static void updateDifficulty() {
    const uint8_t level = (uint8_t)(score / 50);
    fallSpeed = CG_FALL_SPEED + level;
    if (fallSpeed > 8) fallSpeed = 8;

    spawnInterval = CG_SPAWN_MS - (uint16_t)level * 50;
    if (spawnInterval < 300) spawnInterval = 300;
}

static void drawGameOverScreen() {
    tft.fillRect(20, 120, 200, 80, BG);

    tft.setTextSize(2);
    tft.setTextColor(ENEMY_CLR, BG);
    tft.setCursor(40, 130);
    tft.print(F("GAME OVER"));

    tft.setTextSize(1);
    tft.setTextColor(COIN_CLR, BG);
    tft.setCursor(60, 160);
    tft.print(F("Score: "));
    tft.print(score);

    tft.setTextColor(TEXT_CLR, BG);
    tft.setCursor(30, 185);
    tft.print(F("Press button to exit"));
}
