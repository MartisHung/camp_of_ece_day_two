#ifndef CATCH_GAME_H
#define CATCH_GAME_H

#include <Arduino.h>
#include <stdint.h>

// ---- Game States ----
enum class CatchState : uint8_t { CS_CALIBRATE, CS_PLAYING, CS_GAMEOVER };

// ---- Falling Item Types ----
// IT_ENEMY: red, deals 1 damage
// IT_HEALTH: green, restores 1 HP (capped at max)
// IT_COIN: yellow, adds score
enum class ItemType : uint8_t { IT_ENEMY = 0, IT_HEALTH, IT_COIN };

// ---- Layout Constants (240×320 portrait) ----
#define CG_PLAYER_W 20    // player sprite width  (px)
#define CG_PLAYER_H 16    // player sprite height (px)
#define CG_PLAYER_Y (300) // fixed y of player top edge
#define CG_ITEM_SZ 10     // falling item size (square, px)
#define CG_MAX_ITEMS 8    // max simultaneous falling items (SRAM budget)
#define CG_HP_MAX 5       // max hit points
#define CG_HP_INIT 3      // starting hit points
#define CG_COIN_SCORE 10  // points per coin
#define CG_SPAWN_MS 800U  // base spawn interval (ms)
#define CG_FALL_SPEED 2   // base pixels per frame
#define CG_HUD_Y 4        // y of HUD row (HP + Score)
#define CG_PLAY_TOP 20    // y where falling items start spawning

// ---- Entry Point ----
// Blocking loop; returns when user presses button to exit.
// Caller is the main menu loop in .ino.
void loopCatchGame();

#endif // CATCH_GAME_H
