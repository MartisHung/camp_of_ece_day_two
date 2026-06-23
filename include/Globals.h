#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdint.h>

// ---- Frame Rate ----
#define FRAME_MS 10U // 100 Hz
// ---- Gameplay Tuning ----
#define MOVE_COOLDOWN 400U

// Caps loop to FRAME_MS; 'start' must be a uint32_t holding millis() at frame begin
#define FRAME_DELAY(start)                                                                         \
    do {                                                                                           \
        uint32_t _el = millis() - (start);                                                         \
        if (_el < FRAME_MS) delay(FRAME_MS - _el);                                                 \
    } while (0)
// the const of the colors & Menu attribute

//  ---- Screen dimensions (ILI9341 portrait) ----
#define TFT_W 240
#define TFT_H 320

// ---- Tilt control (ADXL335) ----
// Threshold expressed in degrees; compared as sin(angle) vs g to avoid asin() at runtime.
// sin(15°) ≈ 0.2588
#define TILT_THRESHOLD_DEG 30
#define TILT_G_THRESH 0.5f

// ERROR 是有兩個方向得大小是相同得
enum class Direction : uint8_t { IDLE = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4, ERROR = 5};
#endif // GLOBALS_H
