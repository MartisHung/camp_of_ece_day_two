# camp_of_ece_day_two 

## 簡介

以 Arduino Nano 為核心，搭配 2.8" ILI9341 TFT 顯示器與 ADXL335 三軸加速度計，實現一個多功能互動系統，包含主菜單、2048 傾斜控制遊戲，以及即時 3 軸角度測試頁面。

---

## 硬體規格

| 元件 | 型號 / 規格 |
|------|------------|
| MCU | Arduino Nano (ATmega328P) |
| 顯示器 | 2.8" TFT SPI 240×320 ILI9341 v1.2 |
| 加速度計 | ADXL335（三軸類比輸出） |
| 旋轉編碼器 | 附按鈕旋轉編碼器 |

---

## 接線表

### TFT 顯示器（ILI9341 SPI）

| TFT 接腳 | Nano 接腳 |
|----------|----------|
| CS       | D5       |
| RST      | D6       |
| DC       | D7       |
| MOSI     | D11 (SPI MOSI) |
| SCK      | D13 (SPI SCK) |
| VCC      | 3.3V 或 5V（依模組規格） |
| GND      | GND      |

> **Note**: MISO (D12) 不需要接（TFT 僅寫入）

### ADXL335 加速度計

| ADXL 接腳 | Nano 接腳 |
|----------|----------|
| X out    | A5       |
| Y out    | A6       |
| Z out    | A7       |
| VCC      | 3.3V     |
| GND      | GND      |

> **Note**: ADXL335 供電 3.3V，輸出約 1.5V（靜止），Nano ADC 參考 5V。

### 旋轉編碼器

| 編碼器接腳 | Nano 接腳 |
|----------|----------|
| SW       | D2       |
| CLK      | D3       |
| DT       | D4       |
| VCC      | 3.3V 或 5V |
| GND      | GND      |

---

## 專案檔案結構

```
ECE-Camp-day2-Course-Code/
├── ECE-Camp-day2-Course-Code.ino   # 主程式：setup() + loop() = 主菜單
├── HardwareManager.cpp     # 硬體初始化、編碼器/按鈕輪詢、ADXL 基線
├── Game2048.cpp            # 2048 blocking loop + 遊戲邏輯
├── AngleTest.cpp           # 角度測試 blocking loop
├── include/                # ← Header-only，只含宣告與公用巨集
│   ├── Globals.h           # 公用巨集（FRAME_MS、TFT_W/H、TILT_G_THRESH）
│   ├── HardwareManager.h   # 硬體接腳巨集、extern 物件、function 宣告
│   ├── Game2048.h          # 2048 遊戲公用巨集與 loopGame2048() 宣告
│   └── AngleTest.h         # 角度測試 loopAngleTest() 宣告
└── markdowns/
    ├── RULES.md
    ├── TODO.md
    └── PROBLEM.md
```

> **Note**: Arduino IDE 只自動編譯與 `.ino` 同目錄的 `.cpp` 檔，因此所有實作檔案必須放在專案根目錄。

---

## 系統架構

### Loop 架構

```
setup()
  └─ initHardware()
  └─ drawMenuBG()          ← 靜態背景只畫一次

loop()  [主菜單 @ 50Hz]
  └─ updateHardware()      ← 每幀更新編碼器 + 按鈕
  └─ 編碼器移動遊標         ← 只重繪遊標（不清全屏）
  └─ 按鈕確認進入子頁面
       ├─ loopGame2048()   ← Blocking loop（有自己的 while(true)）
       └─ loopAngleTest()  ← Blocking loop（有自己的 while(true)）
```

### 幀率控制

所有 loop 底部使用 `FRAME_DELAY(frameStart)` 巨集，限制最高幀率為 **50 Hz（20ms/frame）**。

### 繪圖策略

| 情境 | 策略 |
|------|------|
| 進入新頁面 | `fillScreen()` 後畫靜態背景（標題、標籤、邊框） |
| 遊標移動 | 只填舊遊標位置為背景色，再畫新遊標 |
| 2048 棋盤 | 僅在移動發生時重繪（非每幀）|
| 角度數值 | 每幀填舊數值區域背景色，再寫新數值 |

---

## 所需 Library

在 Arduino Library Manager 安裝：

- EasyButton
- Adafruit ILI9341

---

## ADXL335 傾斜感測原理

- 靜止輸出：約 `307` counts（1.5V / 5V × 1023）
- 靈敏度：約 `61.4` counts/g（300 mV/g / 5V × 1023）
- 遊戲控制：比較 `sin(15°) ≈ 0.259` vs g 值，避免 `asin()` 運行時開銷
- 角度顯示（AngleTest）：`pitch = atan2(gx, √(gy²+gz²))` × 180/π
