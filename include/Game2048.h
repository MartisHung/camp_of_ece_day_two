#ifndef GAME2048_H
#define GAME2048_H

#include <Arduino.h>
#include <stdint.h>

// ---- Board Layout (240×320 display, portrait) ----
constexpr uint8_t CELL_PX = 60;        // pixel size of each cell (4 × 60 = 240 = full width)
constexpr uint8_t BOARD_OFFSET_X = 0;  // board starts at left edge
constexpr uint8_t BOARD_OFFSET_Y = 40; // leave 40 px at top for score/title
constexpr uint8_t SCORE_Y = 8;         // y-position of score label row

// ---- Entry Point ----
// Blocking loop; returns when user presses button to exit.
// Caller is the main menu loop in .ino.
void enterGame2048();

#endif // GAME2048_H
