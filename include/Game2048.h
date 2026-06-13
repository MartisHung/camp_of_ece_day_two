#ifndef GAME2048_H
#define GAME2048_H

#include <Arduino.h>
#include <stdint.h>

// ---- Game States ----
enum class GState : uint8_t { GS_CALIBRATE, GS_PLAYING, GS_GAMEOVER };

// ---- Tilt Directions ----
enum class Dir : uint8_t { D_NONE = 0, D_UP, D_DOWN, D_LEFT, D_RIGHT };

// ---- Board Layout (240×320 display, portrait) ----
#define CELL_PX         60      // pixel size of each cell (4 × 60 = 240 = full width)
#define BOARD_OFFSET_X  0       // board starts at left edge
#define BOARD_OFFSET_Y  40      // leave 40 px at top for score/title
#define SCORE_Y         8       // y-position of score label row
#define GAMEOVER_Y      268     // y-position of game-over message row

// ---- Gameplay Tuning ----
#define MOVE_COOLDOWN   400U    // ms between accepted tilt moves

// ---- Entry Point ----
// Blocking loop; returns when user presses button to exit.
// Caller is the main menu loop in .ino.
void loopGame2048();

#endif // GAME2048_H
