#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <EasyButton.h>
#include <SPI.h>
#include <stdint.h>

// ---- TFT SPI Pins (ILI9341 2.8" 240x320) ----
#define TFT_CS 5
#define TFT_RST 6
#define TFT_DC 7

// ---- ADXL335 Analog Pins ----
#define ADXL_X A5
#define ADXL_Y A6
#define ADXL_Z A7

// ---- Rotary Encoder Pins ----
#define ENC_SW 2
#define ENC_CLK 3
#define ENC_DT 4

// ---- ADXL335 ADC Constants (Nano 5V ref, ADXL at 3.3V) ----
// Zero-g: ~1.5 V → (1.5/5)*1023 ≈ 307 counts  (typical; varies per unit)
// Sensitivity: ~300 mV/g → (0.3/5)*1023 ≈ 61.4 counts/g
#define ADXL_ZERO_DEFAULT 307
#define ADXL_COUNTS_PER_G 61.4f

// ---- Fast ADC Macros ----
#define ACCEL_RAW_X() analogRead(ADXL_X)
#define ACCEL_RAW_Y() analogRead(ADXL_Y)
#define ACCEL_RAW_Z() analogRead(ADXL_Z)
// Convert raw ADC delta from baseline → g value (float)
#define ACCEL_TO_G(raw, base) (((float)((int16_t)(raw) - (int16_t)(base))) / ADXL_COUNTS_PER_G)

// ---- Public Display Object ----
extern Adafruit_ILI9341 tft;

// ---- Hardware Lifecycle ----
void initHardware();
void updateHardware(); // Poll button (EasyButton); call once per frame

// ---- Encoder ISR Control ----
// Enable on main menu entry; disable before entering sub-pages.
// CLK is on D3 (INT1); ISR fires on RISING edge.
void enableEncoderISR();
void disableEncoderISR();

// ---- Input Accessors ----
int8_t getEncoderDelta(); // Cumulative delta since last call; auto-resets to 0
bool isButtonPressed();   // True once per physical press; debounced via EasyButton

// ---- ADXL Baseline Calibration ----
void captureBaseline();
int16_t getBaseRawX();
int16_t getBaseRawY();
int16_t getBaseRawZ();

#endif // HARDWARE_MANAGER_H
