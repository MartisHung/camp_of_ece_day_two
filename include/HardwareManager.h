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
#define TFT_SD1 11
#define TFT_SCK 13
#define TFT_LED 3V3

// ---- ADXL335 Analog Pins ----
#define ADXL_X A5
#define ADXL_Y A6
#define ADXL_Z A7

// ---- Rotary Encoder Pins ----
#define ENC_SW 2
#define ENC_CLK 3
#define ENC_DT 4

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
int8_t isButtonPressed(); // True once per physical press; debounced via EasyButton

// ---- ADXL Baseline Calibration ----
void captureBaseline();
int16_t getBaseRawX();
int16_t getBaseRawY();
int16_t getBaseRawZ();

#endif // HARDWARE_MANAGER_H
