#include "include/HardwareManager.h"
#include "include/LcdRelated.h"

// ---- Display Object ----
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, 11, 13, TFT_RST, 12);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ---- Private: Encoder (ISR-driven) ----
// volatile: shared between ISR and main context
static volatile int8_t encoderDelta = 0;

// ISR – fires on RISING edge of CLK (D3 / INT1)
static void encoderISR() {
    // DT state at rising CLK edge determines direction
    if (digitalRead(ENC_DT) == LOW)
        encoderDelta--;
    else
        encoderDelta++;
}

// ---- Private: Button (EasyButton handles debounce internally) ----
// EasyButton default: INPUT_PULLUP, active LOW, 35 ms debounce
static EasyButton encButton(ENC_SW);

// ================================================================
void initHardware() {
    Serial.begin(115200);
    Serial.println(F("[HW] initHardware start"));

    // ---- ILI9341 TFT ----
    Serial.println(F("[HW] tft.begin()..."));
    tft.begin(3999999);
    Serial.println(F("[HW] tft.begin() done"));

    // Read power-mode register: ILI9341 returns 0x9C when healthy.
    // 0x00 or 0xFF = SPI wiring issue (check CS/DC/RST/MOSI/SCK).
    uint8_t pmode = tft.readcommand8(0x0A);
    Serial.print(F("[HW] ILI9341 power-mode (expect 0x9C): 0x"));
    Serial.println(pmode, HEX);
    if (pmode != 0x9C) {
        Serial.println(F("[HW] WARNING: display not responding! Check SPI wiring."));
    }

    tft.setRotation(0); // Portrait 240x320
    Serial.println(F("[HW] fillScreen..."));
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // bg colour prevents ghost text
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.println(F("HW Init OK"));
    Serial.println(F("[HW] display init done"));

    // ---- Rotary Encoder ----
    // CLK (D3) = INT1 → ISR; DT (D4) = GPIO input; SW (D2) = EasyButton
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);
    encButton.begin(); // EasyButton sets ENC_SW as INPUT_PULLUP internally
    Serial.println(F("[HW] encoder + button init done"));

    Serial.println(F("[HW] initHardware complete"));
}

// ================================================================
void updateHardware() {
    // Encoder is ISR-driven; nothing to poll here.
    // Button debounce state machine must be driven every loop.
    encButton.read();
}

// ================================================================
void enableEncoderISR() {
    noInterrupts();
    encoderDelta = 0; // discard stale delta before re-enabling
    interrupts();
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, RISING);
    Serial.println(F("[ENC] ISR enabled"));
}

void disableEncoderISR() {
    detachInterrupt(digitalPinToInterrupt(ENC_CLK));
    Serial.println(F("[ENC] ISR disabled"));
}

// ================================================================
int8_t getEncoderDelta() {
    // Atomic read+clear (ISR may fire between read and clear)
    noInterrupts();
    int8_t d = encoderDelta;
    encoderDelta = 0;
    interrupts();
    return d;
}

bool isButtonPressed() {
    return encButton.wasPressed(); // true once per press; EasyButton resets it
}

// the define is at "LcdRelated.h"
void drawCalibationScreen(){
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
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
