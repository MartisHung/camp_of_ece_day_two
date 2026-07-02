#include "include/Game2048.h"
#include "include/AngleTest.h"
#include "include/Globals.h"
#include "include/HardwareManager.h"
#include "include/LcdRelated.h"
#include <string.h>

// ================================================================
// Private constexpr
// ================================================================
constexpr uint8_t GRID_N = 4;
constexpr uint16_t GRID_LINE_CLR = 0x7BEFu;
constexpr uint16_t TEXT_LIGHT = ILI9341_WHITE;
constexpr uint16_t TEXT_DARK = ILI9341_BLACK;
constexpr uint16_t BG_CLR = ILI9341_BLACK;

// board[r][c] stores power-of-2 exponent (0=empty, 1=2, 2=4 ... 11=2048)
// in RAM for faster lookup
static const uint16_t TILE_COLORS[12] = {
    0x0000u, // 0  empty -> black
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
    0x07FFu  // 11 2048
};

// in RAM to avoid pgm_read_ptr, fixes garbled text
static const char *const DIR_NAMES[5] = {"IDLE", "UP", "DOWN", "LEFT", "RIGHT"};

// ================================================================
// Private state
// ================================================================
static uint8_t board[GRID_N][GRID_N]; // exponent, 0 = empty
static uint32_t score;
static uint32_t lastMoveTime;

// ================================================================
// Private prototypes
// ================================================================
static void calibrate();
static void game2048Init();
static int8_t gameLoop();

static void doMoveAndRender(Direction dir);
static uint8_t addRandomBlock();
static uint8_t doMove(Direction dir);
static uint8_t slideLine(uint8_t row[GRID_N]);
static uint8_t isGameOver();
static uint8_t isWon();

static void drawCell(uint8_t r, uint8_t c);
static void drawGridLines();
static void drawStaticBG();
static void drawScore();
static void drawDirection(Direction dir);
static void drawGameOver(bool won);

// ================================================================
// Public entry
// ================================================================
void enterGame2048() {
    calibrate();
    if (gameLoop()) game2048Init();
}

// ================================================================
// calibrate
// ================================================================
static void calibrate() {
    drawCalibationScreen();
    while (true) {
        updateHardware();
        if (isButtonPressed()) break;
    }
}

// ================================================================
// game2048Init
// ================================================================
static void game2048Init() {
    memset(board, 0, sizeof(board));
    score = 0;
    lastMoveTime = 0;

    addRandomBlock();

    addRandomBlock();

    drawStaticBG();
    drawScore();
    drawDirection(Direction::IDLE);

    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (board[r][c]) drawCell(r, c);

    drawGridLines();
}

// ================================================================
// gameLoop return
// -> 0     Button Pressed Intrrupt
// -> 1     won & exit correctly
// -> -1    Lost but exit correctly
// ================================================================
static int8_t gameLoop() {
    while (true) {
        uint32_t fs = millis();
        updateHardware();

        if (isButtonPressed()) return 0;

        Direction dir = getDirectionForGame2048(1);

        if (millis() - lastMoveTime >= MOVE_COOLDOWN) {
            if (static_cast<uint8_t>(dir) ^ static_cast<uint8_t>(Direction::IDLE)) {
                doMoveAndRender(dir);
                lastMoveTime = millis();

                if (isWon()) {
                    drawGameOver(true);
                    while (!isButtonPressed()) updateHardware();
                    return 1;
                }
                if (isGameOver()) {
                    drawGameOver(false);
                    while (!isButtonPressed()) updateHardware();
                    return -1;
                }
            } else
                drawDirection(Direction::IDLE);
        }

        FRAME_DELAY(fs);
    }
}

// ================================================================
// doMoveAndRender — snapshot, move, draw only changed cells
// ================================================================
static void doMoveAndRender(const Direction dir) {
    uint8_t prev[GRID_N][GRID_N];
    memcpy(prev, board, sizeof(board));
    const uint32_t prevScore = score;

    if (!doMove(dir)) return;

    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (board[r][c] ^ prev[r][c]) drawCell(r, c);

    // drawGridLines();
    drawDirection(dir);
    if (score ^ prevScore) drawScore();

    addRandomBlock(); // draws new tile internally
}

// ================================================================
// addRandomBlock — LCG, draws new tile directly
// 1 -> success, 0 -> board full
// ================================================================
static uint8_t addRandomBlock() {
    static uint8_t seed = 0x7B;
    static uint8_t i = 0;

    uint8_t empties[GRID_N * GRID_N];
    uint8_t cnt = 0;
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (!board[r][c]) empties[cnt++] = r * GRID_N + c;

    if (!cnt) return 0;

    seed = seed * 5 + 7;
    if (++i >= 35) {
        i = 0;
        seed = 0x7B;
    }

    const uint8_t pos = empties[seed % cnt];
    const uint8_t r = pos / GRID_N;
    const uint8_t c = pos % GRID_N;

    board[r][c] = (seed % 10) ? 1 : 2; // 90% exp1(val 2), 10% exp2(val 4)
    drawCell(r, c);
    return 1;
}

// ================================================================
// Slide / merge — operates on exponents
// 1 -> moved, 0 -> no change
// ================================================================
static uint8_t slideLine(uint8_t row[GRID_N]) {
    uint8_t moved = 0;

    // ---- pack non-zero left ----
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (!row[i]) {
            for (uint8_t j = i + 1; j < GRID_N; j++) {
                if (row[j]) {
                    row[i] = row[j];
                    row[j] = 0;
                    moved = 1;
                    break;
                }
            }
        }
    }

    // ---- merge equal adjacent ----
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (row[i] && !(row[i] ^ row[i + 1])) {
            row[i]++;
            score += (1u << row[i]);
            row[i + 1] = 0;
            moved = 1;
        }
    }

    // ---- pack again after merge ----
    for (uint8_t i = 0; i < GRID_N - 1; i++) {
        if (!row[i]) {
            for (uint8_t j = i + 1; j < GRID_N; j++) {
                if (row[j]) {
                    row[i] = row[j];
                    row[j] = 0;
                    moved = 1;
                    break;
                }
            }
        }
    }
    return moved;
}

static uint8_t doMove(const Direction dir) {
    uint8_t moved = 0;
    uint8_t tmp[GRID_N];

    if (dir == Direction::LEFT) {
        for (uint8_t r = 0; r < GRID_N; r++)
            if (slideLine(board[r])) moved = 1;

    } else if (dir == Direction::RIGHT) {
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
                moved = 1;
            }
        }
    } else if (dir == Direction::UP) {
        for (uint8_t c = 0; c < GRID_N; c++) {
            for (uint8_t r = 0; r < GRID_N; r++) tmp[r] = board[r][c];
            if (slideLine(tmp)) {
                for (uint8_t r = 0; r < GRID_N; r++) board[r][c] = tmp[r];
                moved = 1;
            }
        }
    } else if (dir == Direction::DOWN) {
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
                moved = 1;
            }
        }
    }
    return moved;
}

static uint8_t isWon() {
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (board[r][c] == 11) return 1;
    return 0;
}

static uint8_t isGameOver() {
    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++)
            if (!board[r][c]) return 0;

    for (uint8_t r = 0; r < GRID_N; r++)
        for (uint8_t c = 0; c < GRID_N; c++) {
            if (c < GRID_N - 1 && !(board[r][c] ^ board[r][c + 1])) return 0;
            if (r < GRID_N - 1 && !(board[r][c] ^ board[r + 1][c])) return 0;
        }
    return 1;
}

// ================================================================
// Draw functions
// ================================================================
static void drawCell(const uint8_t r, const uint8_t c) {
    const uint8_t pwr = board[r][c];
    const int16_t x = BOARD_OFFSET_X + c * CELL_PX + 1;
    const int16_t y = BOARD_OFFSET_Y + r * CELL_PX + 1;
    const uint8_t sz = CELL_PX - 2;
    const uint16_t bg = TILE_COLORS[pwr > 11 ? 11 : pwr];

    tft.fillRect(x, y, sz, sz, bg);
    if (!pwr) return;

    const uint16_t val = 1u << pwr;
    const uint8_t digits = (val < 10) ? 1 : (val < 100) ? 2 : (val < 1000) ? 3 : 4;
    const uint8_t tsize = (val < 100) ? 3 : (val < 1000) ? 2 : 1;
    const int16_t tx = x + (sz - digits * 6 * tsize) / 2;
    const int16_t ty = y + (sz - 8 * tsize) / 2;

    tft.setTextSize(tsize);
    tft.setTextColor((pwr >= 7) ? TEXT_DARK : TEXT_LIGHT, bg);
    tft.setCursor(tx, ty);
    tft.print(val);
}

static void drawGridLines() {
    for (uint8_t i = 0; i <= GRID_N; i++) {
        tft.drawFastHLine(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_PX, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
        tft.drawFastVLine(BOARD_OFFSET_X + i * CELL_PX, BOARD_OFFSET_Y, GRID_N * CELL_PX,
                          GRID_LINE_CLR);
    }
}

static void drawStaticBG() {
    tft.fillScreen(BG_CLR);

    // ---- Score area ----
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setTextSize(1);
    tft.setCursor(0, SCORE_Y);
    tft.print(F("SCORE:"));

    // ---- Direction label ----
    tft.setCursor(130, SCORE_Y);
    tft.print(F("Dir:"));

    // ---- Grid lines ----
    drawGridLines();
}

static void drawScore() {
    tft.fillRect(45, SCORE_Y, 80, 8, BG_CLR);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setTextSize(1);
    tft.setCursor(45, SCORE_Y);
    tft.print(score);
}

static void drawDirection(const Direction dir) {

    static Direction old = Direction::ERROR;
    if (old == dir) return;
    tft.fillRect(158, SCORE_Y, 82, 8, BG_CLR);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setTextSize(1);
    tft.setCursor(158, SCORE_Y);
    tft.print(DIR_NAMES[static_cast<uint8_t>(dir)]);
    old = dir;
}

static void drawGameOver(const bool won) {
    // ---- clear center 3x3 area ----
    constexpr uint8_t BOX = 3;
    constexpr int16_t bx = BOARD_OFFSET_X + ((GRID_N - BOX) * CELL_PX) / 2;
    constexpr int16_t by = BOARD_OFFSET_Y + ((GRID_N - BOX) * CELL_PX) / 2;
    constexpr uint16_t bsz = BOX * CELL_PX;
    constexpr uint8_t PAD = 8;

    tft.fillRect(bx, by, bsz, bsz, BG_CLR);

    // ---- result text, left aligned ----
    tft.setTextSize(2);
    tft.setTextColor(won ? 0x07FFu : ILI9341_RED, BG_CLR);
    tft.setCursor(bx + PAD, by + PAD);
    tft.print(won ? F("YOU WIN!") : F("GAME OVER"));

    // ---- hint ----
    tft.setTextSize(1);
    tft.setTextColor(TEXT_LIGHT, BG_CLR);
    tft.setCursor(bx + PAD, by + PAD + 24);
    tft.print(F("Press button to exit"));
}
