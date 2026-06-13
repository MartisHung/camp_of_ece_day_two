# TODO

## 本次改動（2026-05-09）

### 硬體遷移
- [x] 顯示器：ST7735S 128×160 → ILI9341 2.8" 240×320 SPI
  - CS=D5, RST=D6, DC=D7
  - Library 改為 `Adafruit_ILI9341`
- [x] 加速度計：MPU6050（I2C） → ADXL335（類比）
  - X=A5, Y=A6, Z=A7；移除 Wire / Adafruit_MPU6050 / Adafruit_Sensor
- [x] MCU：UNO → Nano（接腳相容，無額外更動）
- [x] 移除 DS3231 RTC（RTClib 一併刪除）

### 架構重構
- [x] 主菜單直接寫在 `loop()`，不再使用 state machine switch
- [x] 子頁面（2048、角度測試）改為 **blocking loop**（`while(true)` + `return` 退出）
- [x] 幀率用 `FRAME_DELAY()` 巨集限制 50 Hz，避免過度刷屏
- [x] 靜態背景只在進入頁面時畫一次；動態元素（遊標、數值）局部更新
- [x] `.h` 只保留公用巨集、`extern` 宣告、function prototype
- [x] `.cpp` 保留私有巨集、`static` 變數、function 實作

### 頁面調整
- [x] 移除 StateClock（無 RTC）
- [x] 移除 StateSettings（暫無設定需求）
- [x] 移除 StateGameMenu（合併入主 loop）
- [x] 移除 StateCalibrate（校正併入 loopGame2048 前置流程）
- [x] 新增 AngleTest 頁面（ADXL335 Raw/g/Pitch/Roll 即時顯示）
- [x] 2048 棋盤佈局調整為 240×240，每格 60×60

### 文件
- [x] 新增 README.md（接線、架構、Library）
- [x] 更新 TODO.md
- [x] 新增 PROBLEM.md（硬體不確定項）

---

## 後續待辦

### 高優先
- [ ] 實際硬體驗證 ADXL335 零偏值（`ADXL_ZERO_DEFAULT` 可能需校正）
- [ ] 確認 ILI9341 模組接腳（部分模組 CS/DC/RST 順序不同）
- [ ] 編譯測試（arduino-cli 或 Arduino IDE，目標板：Arduino Nano）

### 中優先
- [ ] 2048 win 條件觸發時顯示 "YOU WIN!" 畫面（目前 `drawGameOver(false)` 無觸發 win）
  - 在 `doMove()` 後檢查 board 是否有 2048 格
- [ ] 旋轉編碼器方向校正（順時針/逆時針對應 +/- 視實際元件而定）
- [ ] 2048 高分紀錄（EEPROM 儲存）

### 低優先 / 未來擴充
- [ ] 新增第二款遊戲（記憶體確認後）
- [ ] 主菜單可視化美化（icon、動畫）
- [ ] AngleTest 增加校正功能（按下基線歸零）
