# PROBLEM – 待確認硬體問題

> 以下問題為程式碼生成時無法確定的硬體細節，需實際量測或查閱模組規格後調整。

---

## P1. ADXL335 零偏值（`ADXL_ZERO_DEFAULT`）

**目前設定值**：`307`（對應 1.5V / 5V × 1023）

**問題**：ADXL335 的 零g 輸出電壓依供電電壓而異：
- 3.3V 供電：典型 zero-g = VCC/2 = 1.65V → ADC ≈ **338**
- 3.0V 供電：typical zero-g ≈ 1.5V → ADC ≈ **307**

**建議**：將模組平放，執行 `AngleTest` 頁面觀察靜止時的 Raw X/Y/Z 值，再將該值填入：

```cpp
// HardwareManager.h
#define ADXL_ZERO_DEFAULT   <實測值>
```

或者更改為在 `initHardware()` 時自動讀取一次作為動態基準（但這樣每次開機姿勢必須水平）。

---

## P2. ADXL335 靈敏度（`ADXL_COUNTS_PER_G`）

**目前設定值**：`61.4f`（對應 300 mV/g / 5V × 1023）

**問題**：ADXL335 靈敏度規格：
- 3.3V 供電：典型 **300 mV/g**，最小 270，最大 330
- 實際靈敏度個體差異大

**建議**：將裝置垂直立起（Z 軸對重力，讀值應為 +1g 或 -1g），量測偏移量，計算真實 counts/g：

```
counts_per_g = (raw_at_+1g - zero_raw)
```

---

## P3. ILI9341 TFT 模組供電電壓

**問題**：部分 2.8" ILI9341 模組為 **5V 相容**，部分需要 **3.3V**。若 Logic Level 不相容，長時間使用可能損壞模組。

**確認方式**：查看模組背面標記或賣家規格。若為 3.3V Logic，需加 Level Shifter（MOSI/SCK/CS/DC/RST 所有 SPI 線均需轉換）。

---

## P4. 旋轉編碼器方向

**問題**：目前程式設定順時針 = `encoderDelta++`（向下選）。實際方向取決於編碼器正反接法。

**測試方式**：開機後轉動編碼器，觀察 AngleTest 的遊標移動方向是否符合直覺。若相反，在 `HardwareManager.cpp` 中交換 `++` / `--`：

```cpp
// srcs/HardwareManager.cpp  updateHardware() 中
if (digitalRead(ENC_DT) != curClk) encoderDelta--;  // 改符號
else                                encoderDelta++;
```

---

## P5. 2048 遊戲傾斜方向對應

**問題**：gX 正值對應 `D_RIGHT`，負值對應 `D_LEFT`；gY 正值對應 `D_DOWN`，負值對應 `D_UP`。這依賴 ADXL335 的實際安裝方向。

**測試方式**：進入 AngleTest 頁面，觀察各方向傾斜時的 gX/gY 正負號，再回到 `Game2048.cpp` 的 `readTiltDir()` 調整映射：

```cpp
static Dir readTiltDir() {
    float gx = ACCEL_TO_G(ACCEL_RAW_X(), getBaseRawX());
    float gy = ACCEL_TO_G(ACCEL_RAW_Y(), getBaseRawY());
    if      (gx >  TILT_G_THRESH) return D_RIGHT;  // 依需要改方向
    else if (gx < -TILT_G_THRESH) return D_LEFT;
    else if (gy >  TILT_G_THRESH) return D_DOWN;
    else if (gy < -TILT_G_THRESH) return D_UP;
    return D_NONE;
}
```
