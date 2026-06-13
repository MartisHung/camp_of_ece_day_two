#include "include/Game2048.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include <math.h>

// ================================================================
// Private macros
// ================================================================
#define GRID_N 4

// ILI9341 RGB565 tile colours indexed by power-of-2 (index = log2(value))
// index 0 = empty cell
#define TILE_CLR_EMPTY 0x39E7u // dark grey
// colours for values 2(1)..2048(11)
static const uint16_t TILE_COLORS[12] PROGMEM = {
    0x39E7u, // 0  empty
    0xEF5Du, // 1  2
    0xF5BBu, // 2  4
    0xFB6Du, // 3  8
    0xFD40u, // 4  16
    0xFC00u, // 5  32
    0xF800u, // 6  64
    0xFFE0u, // 7  128
    0xFEA0u, // 8  256
    0xFDA0u, // 9  512
    0xFF00u, // 10 1024
    0x07FFu  // 11 2048  cyan/win
};

#define GRID_LINE_CLR 0x7BEFu // light grey grid lines
#define TEXT_LIGHT ILI9341_WHITE
#define TEXT_DARK ILI9341_BLACK
#define BG_CLR ILI9341_BLACK

// Pixel origin of cell (r, c)
#define CELL_X(c) (BOARD_OFFSET_X + (c) * CELL_PX)
#define CELL_Y(r) (BOARD_OFFSET_Y + (r) * CELL_PX)

// log2 for uint16_t (only powers of 2 expected)
#define LOG2_U16(v) (__builtin_ctz(v))

// ================================================================
// Private types
// ================================================================
// GState and Dir enums are defined in Game2048.h (Rule 3)

// ================================================================
// Private state
// ================================================================
static uint16_t board[GRID_N][GRID_N];
static uint32_t score;
static GState gState = GState::GS_CALIBRATE;
static uint32_t lastMoveTime;
static bool tiltGate; // prevents repeated moves on sustained tilt

// ================================================================
// Private function prototypes
// ================================================================
static void resetBoard();
static void addRandomTile();
static void drawStaticBG();
static void drawScore();
static void drawCell(uint8_t r, uint8_t c);
static void drawBoard();
static void drawGameOver(bool won);
static void drawCalibScreen();
static bool slideLine(uint16_t row[GRID_N]);
static bool doMove(Dir d);
static bool isGameOver();
static Dir readTiltDir();

// ================================================================
// loopGame2048 – public entry (blocking)
// ================================================================
void loopGame2048() {
    // ---- Calibration phase ----
    gState = GState::GS_CALIBRATE;
    drawCalibScreen();

    while (gState == GState::GS_CALIBRATE) {
        uint32_t fs = millis();
        updateHardware();
        if (isButtonPressed()) {
            captureBaseline();
            gState = GState::GS_PLAYING;
        }
        FRAME_DELAY(fs);
    }

    // ---- Game init ----
    resetBoard();
    score = 0;
    lastMoveTime = 0;
    tiltGate = false;

    drawStaticBG();
    drawScore();
    drawBoard();

    // ---- Main game loop ----
    while (true) {
        uint32_t fs = millis();
        updateHardware();

        // Any button press exits back to main menu
        if (isButtonPressed()) return;

        if (gState == GState::GS_PLAYING) {
            if (millis() - lastMoveTime >= MOVE_COOLDOWN) {
                Dir d = readTiltDir();

                if (static_cast<uint8_t>(d) ^ static_cast<uint8_t>(Dir::D_NONE)) {
                    if (static_cast<uint8_t>(tiltGate) ^ 1) {
                        bool moved = doMove(d);
                        if (moved) {
                            addRandomTile();
                            drawBoard();
                            drawScore();
                            if (isGameOver()) {
                                gState = GState::GS_GAMEOVER;
                                drawGameOver(false);
                            }
                        }
                        tiltGate = true;
                        lastMoveTime = millis();
                    }
                } else {
                    tiltGate = false; // device returned to flat
                }
            }
        }
        // GS_GAMEOVER: just wait for button (handled above)

        FRAME_DELAY(fs);
    }
}

// ================================================================
// Static helpers
// ================================================================

static void resetBoard() {
    memset(board, 0, sizeof(board));
    addRandomTile();
    addRandomTile();
}

static void addRandomTile() {
    uint8_t empties[GRID_N * GRID_N];
    uint8_t cnt = 0;
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (board[r][c] == 0) empties[cnt++] = r * GRID_N + c;

    if (cnt == 0) return;
    uint8_t idx = (uint8_t)random(cnt);
    uint8_t r = empties[idx] / GRID_N;
    uint8_t c = empties[idx] % GRID_N;
    board[r][c] = (random(10) < 9) ? 2 : 4;
}

// ---- Draw static background (once on game start) ----
static void drawStaticBG() {
    tft.fillScreen(BG_CLR);

    // Score label (static text)
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setTextSize(1);
    tft.setCursor(0, SCORE_Y);
    tft.print(F("SCORE:"));

    // Grid lines (horizontal + vertical)
    for (uint8_t i = 0; i <= GRID_N; i++) {
        tft.drawFastHLine(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_PX, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
        tft.drawFastVLine(BOARD_OFFSET_X + i * CELL_PX, BOARD_OFFSET_Y, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
    }
}

// ---- Draw/update score value only ----
static void drawScore() {
    // Erase old value area (fixed width region after "SCORE:")
    tft.fillRect(45, SCORE_Y, 90, 8, BG_CLR);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setTextSize(3);
    tft.setCursor(45, SCORE_Y);
    tft.print(score);
}

// ---- Draw a single cell ----
static void drawCell(uint8_t r, uint8_t c) {
    uint16_t val = board[r][c];
    int16_t x = CELL_X(c) + 1; // +1: inside grid line
    int16_t y = CELL_Y(r) + 1;
    uint8_t sz = CELL_PX - 2;

    // Background fill
    uint8_t pidx = (val == 0) ? 0 : (uint8_t)LOG2_U16(val);
    if (pidx > 11) pidx = 11;
    uint16_t bg = pgm_read_word(&TILE_COLORS[pidx]);
    tft.fillRect(x, y, sz, sz, bg);

    if (val == 0) return;

    // Choose text size by number of digits
    uint8_t tsize;
    if (val < 10)
        tsize = 3;
    else if (val < 100)
        tsize = 3;
    else if (val < 1000)
        tsize = 2;
    else
        tsize = 1;

    // Character dimensions: 6×8 per char at size 1
    uint8_t digits = (val < 10) ? 1 : (val < 100) ? 2 : (val < 1000) ? 3 : 4;
    uint8_t charW = 6 * tsize;
    uint8_t charH = 8 * tsize;
    uint8_t textW = digits * charW;
    int16_t tx = x + (sz - textW) / 2;
    int16_t ty = y + (sz - charH) / 2;

    tft.setTextSize(tsize);
    tft.setTextColor((pidx >= 7) ? TEXT_DARK : TEXT_LIGHT, bg);
    tft.setCursor(tx, ty);
    tft.print(val);
}

// ---- Redraw all 16 cells ----
static void drawBoard() {
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++) drawCell(r, c);
    // Restore grid lines (cells overwrite them)
    for (uint8_t i = 0; i <= GRID_N; i++) {
        tft.drawFastHLine(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_PX, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
        tft.drawFastVLine(BOARD_OFFSET_X + i * CELL_PX, BOARD_OFFSET_Y, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
    }
}

static void drawGameOver(bool won) {
    tft.fillRect(0, GAMEOVER_Y, TFT_W, 50, BG_CLR);
    tft.setTextSize(2);
    tft.setTextColor(won ? 0x07FFu : ILI9341_RED, BG_CLR);
    tft.setCursor(30, GAMEOVER_Y);
    tft.print(won ? F("YOU WIN!") : F("GAME OVER"));
    tft.setTextSize(1);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setCursor(20, GAMEOVER_Y + 22);
    tft.print(F("Press button to exit"));
}

static void drawCalibScreen() {
    tft.fillScreen(BG_CLR);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
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
}

// ================================================================
// Slide / merge logic
// ================================================================

// Slide and merge one row leftward; returns true if anything changed
static bool slideLine(uint16_t row[GRID_N]) {
    bool moved = false;

    // Pack non-zero values left
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (row[i] == 0) {
            for (uint8_t j = i + 1; j < GRID_N; j++) {
                if (row[j] != 0) {
                    row[i] = row[j];
                    row[j] = 0;
                    moved = true;
                    break;
                }
            }
        }
    }
    // Merge adjacent equal tiles
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (row[i] != 0 && row[i] == row[i + 1]) {
            row[i] <<= 1; // ×2
            score += row[i];
            row[i + 1] = 0;
            moved = true;
        }
    }
    // Pack again after merge
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (row[i] == 0) {
            for (uint8_t j = i + 1; j < GRID_N; j++) {
                if (row[j] != 0) {
                    row[i] = row[j];
                    row[j] = 0;
                    moved = true;
                    break;
                }
            }
        }
    }
    return moved;
}

static bool doMove(Dir d) {
    bool moved = false;
    uint16_t tmp[GRID_N];

    if (d == Dir::D_LEFT) {
        for (uint8_t r = 0; r < GRID_N; r++)
            if (slideLine(board[r])) moved = true;
    } else if (d == Dir::D_RIGHT) {
        for (uint8_t r = 0; r < GRID_N; r++) {
            tmp[0] = board[r][3];
            tmp[1] = board[r][2];
            tmp[2] = board[r][1];
            tmp[3] = board[r][0];
            if (slideLine(tmp)) {
                board[r][3] = tmp[0];
                board[r][2] = tmp[1];
                board[r][1] = tmp[2];
                board[r][0] = tmp[3];
                moved = true;
            }
        }
    } else if (d == Dir::D_UP) {
        for (uint8_t c = 0; c < GRID_N; c++) {
            for (uint8_t r = 0; r < GRID_N; r++) tmp[r] = board[r][c];
            if (slideLine(tmp)) {
                for (uint8_t r = 0; r < GRID_N; r++) board[r][c] = tmp[r];
                moved = true;
            }
        }
    } else if (d == Dir::D_DOWN) {
        for (uint8_t c = 0; c < GRID_N; c++) {
            tmp[0] = board[3][c];
            tmp[1] = board[2][c];
            tmp[2] = board[1][c];
            tmp[3] = board[0][c];
            if (slideLine(tmp)) {
                board[3][c] = tmp[0];
                board[2][c] = tmp[1];
                board[1][c] = tmp[2];
                board[0][c] = tmp[3];
                moved = true;
            }
        }
    }
    return moved;
}

static bool isGameOver() {
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (board[r][c] == 0) return false;

    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++) {
            if (c < GRID_N - 1 && board[r][c] == board[r][c + 1]) return false;
            if (r < GRID_N - 1 && board[r][c] == board[r + 1][c]) return false;
        }
    return true;
}

// ================================================================
// Tilt detection using ADXL335
// ================================================================
// Compare g component against sin(TILT_THRESHOLD_DEG) to avoid asin() at runtime.
// Threshold: TILT_G_THRESH = sin(15°) ≈ 0.2588 (defined in Globals.h)
static Dir readTiltDir() {
    float gx = ACCEL_TO_G(ACCEL_RAW_X(), getBaseRawX());
    float gy = ACCEL_TO_G(ACCEL_RAW_Y(), getBaseRawY());

    if (gx > TILT_G_THRESH)
        return Dir::D_LEFT;
    else if (gx < -TILT_G_THRESH)
        return Dir::D_RIGHT;
    else if (gy > TILT_G_THRESH)
        return Dir::D_DOWN;
    else if (gy < -TILT_G_THRESH)
        return Dir::D_UP;
    return Dir::D_NONE;
}
