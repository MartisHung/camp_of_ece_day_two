#ifndef CATCH_GAME_H
#define CATCH_GAME_H

#include <Arduino.h>
#include <stdint.h>

// ---- Falling Item Types ----
// IT_ENEMY: red, deals 1 damage
// IT_HEALTH: green, restores 1 HP (capped at max)
// IT_COIN: yellow, adds score
enum class ItemType : uint8_t { IT_ENEMY = 0, IT_HEALTH, IT_COIN };

struct FallingItem {
    int16_t x;
    int16_t y;
    ItemType type;
    uint8_t active;
};

// ---- Layout Constants (240×320 portrait) ----
constexpr uint8_t CG_PLAYER_W = 20;    // player sprite width  (px)
constexpr uint8_t CG_PLAYER_H = 16;    // player sprite height (px)
constexpr int16_t CG_PLAYER_Y = 300;   // fixed y of player top edge (signed)
constexpr uint8_t CG_ITEM_SZ = 10;     // falling item size (square, px)
constexpr uint8_t CG_MAX_ITEMS = 8;    // max simultaneous falling items (SRAM budget)
constexpr uint8_t CG_HP_MAX = 5;       // max hit points
constexpr uint8_t CG_HP_INIT = 3;      // starting hit points
constexpr uint8_t CG_COIN_SCORE = 10;  // points per coin
constexpr uint16_t CG_SPAWN_MS = 800U; // base spawn interval (ms)
constexpr uint8_t CG_FALL_SPEED = 2;   // base pixels per frame
constexpr uint8_t CG_HUD_Y = 4;        // y of HUD row (HP + Score)
constexpr uint8_t CG_PLAY_TOP = 20;    // y where falling items start spawning
constexpr uint8_t CG_MOVE_SPEED = 4;
// ---- Entry Point ----
// Blocking loop; returns when user presses button to exit.
// Caller is the main menu loop in .ino.
void enterCatchGame();
#endif // CATCH_GAME_H
