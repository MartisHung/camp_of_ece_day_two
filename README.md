# camp_of_ece_day_two

## 簡介

以 Arduino Nano 為核心，搭配 2.8" ILI9341 TFT 顯示器與 ADXL335 三軸加速度計，實現一個多功能互動系統，包含主菜單、2048 傾斜控制遊戲、接物遊戲（Catch Game），以及即時 3 軸角度測試頁面。

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
| MOSI (SD1) | D11 (SPI MOSI) |
| SCK      | D13 (SPI SCK) |
| LED      | 3.3V     |
| VCC      | 3.3V 或 5V（依模組規格） |
| GND      | GND      |

> **Note**: MISO (D12) 不需要接（TFT 僅寫入）。SPI 時脈設定為 4MHz（`tft.begin(3999999)`），實測為此接線下最穩定的速度。

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
| SW       | D2 (INT0 / EasyButton) |
| CLK      | D3 (INT1 / ISR)        |
| DT       | D4       |
| VCC      | 3.3V 或 5V |
| GND      | GND      |

---

## 專案檔案結構

```
camp_of_ece_day_two/
├── camp_of_ece_day_two.ino # 主程式：setup() + loop() = 主菜單
├── HardwareManager.cpp     # 硬體初始化、編碼器 ISR、按鈕、校正畫面
├── Game2048.cpp            # 2048 blocking loop + 遊戲邏輯
├── CatchGame.cpp           # 接物遊戲 blocking loop + 遊戲邏輯
├── AngleTest.cpp           # 角度測試頁 + 兩個遊戲共用的方向偵測 API
├── include/                # ← Header-only，只含宣告、常數與巨集
│   ├── Globals.h           # FRAME_MS、FRAME_DELAY、TFT_W/H、Direction enum
│   ├── HardwareManager.h   # 硬體接腳巨集、extern tft、function 宣告
│   ├── Game2048.h          # 2048 版面常數與 enterGame2048() 宣告
│   ├── CatchGame.h         # 接物遊戲常數、FallingItem struct、enterCatchGame() 宣告
│   ├── AngleTest.h         # 感測器讀取/映射巨集、版面常數、方向 API 宣告
│   └── LcdRelated.h        # 顏色巨集、共用版面高度
└── markdowns/
    └── RULES.md
```

> **Note**: Arduino IDE 只自動編譯與 `.ino` 同目錄的 `.cpp` 檔，因此所有實作檔案必須放在專案根目錄。

---

## 系統架構

### Loop 架構

```
setup()
  └─ initHardware()          ← TFT、編碼器、按鈕
  └─ enableEncoderISR()      ← 編碼器只在主菜單使用 ISR
  └─ drawMenuBG()            ← 靜態背景只畫一次

loop()  [主菜單 @ 100Hz]
  └─ updateHardware()        ← 每幀驅動按鈕 debounce
  └─ 編碼器移動遊標           ← 只重繪遊標（不清全屏）
  └─ 按鈕確認進入子頁面（進入前 disableEncoderISR()）
       ├─ loopAngleTest()    ← Blocking loop，即時顯示 3 軸數值與方向
       ├─ enterGame2048()    ← 校正畫面 → Blocking game loop
       └─ enterCatchGame()   ← 校正畫面 → Blocking game loop
```

### 方向偵測 API（AngleTest.cpp 提供）

兩個遊戲各自有專屬的方向抓取函數，回傳 `Direction` enum（IDLE/UP/DOWN/LEFT/RIGHT/ERROR）：

- `getDirectionForGame2048()` — 讀 X、Y 兩軸，取傾斜量較大的軸決定四方向；兩軸相等時回傳 `ERROR`
- `getDirectionForGameCatch()` — 只讀 X 軸，回傳左/右/IDLE

原始 ADC 值經 `map(raw, ANGLE_MIN=300, ANGLE_MAX=450, -50, 50)` 映射為 ±50，靜止死區為 `IDLE_RANGE = ±15`。

### 幀率控制

所有 loop 底部使用 `FRAME_DELAY(frameStart)` 巨集，限制最高幀率為 **100 Hz（10ms/frame，`FRAME_MS = 10`）**。角度測試頁的數值更新另有 100ms 節流。

### 繪圖策略

| 情境 | 策略 |
|------|------|
| 進入新頁面 | `fillScreen()` 後畫靜態背景（標題、標籤、邊框） |
| 遊標移動 | 只填舊遊標位置為背景色，再畫新遊標 |
| 2048 棋盤 | 僅在移動發生時重繪有變化的格子（非每幀、不重畫格線） |
| 接物遊戲 | 每幀擦掉舊位置、畫新位置；HUD 只在數值變化時更新 |
| 角度數值 | 每次更新先填舊數值區域背景色，再寫新數值 |

---

## 所需 Library

在 Arduino Library Manager 安裝：

- EasyButton
- Adafruit ILI9341

---

## ADXL335 傾斜感測原理

- Nano ADC 參考 5V，ADXL335 靜止（0g）輸出約 1.5V ≈ `307` counts
- 實測可用範圍約 `300 ~ 450` counts（`ANGLE_MIN` / `ANGLE_MAX`）
- 遊戲控制不做三角函數運算，直接將 ADC 值線性映射為 ±50 再比較死區，省去 `asin()` 的運行時開銷
