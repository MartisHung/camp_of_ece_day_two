// 這裡只寫我們程式規範部份
# 規則

1. 函數回傳狀態 `correct -> 0 error -> 非0之數` 通常為 -1 如果回傳非-1 則需要寫註解解釋回傳的數字所代表的意思
    為減少記憶體用量回傳數值盡可能使用 `int8_t` 或者`enum`作為我們的回傳形式 若使用enum則註解寫在enum上
2. 檔案架構最上層只留：`README.md` `.ino` `.clang-format`  剩下 ~/{include/`headers相關`} markdown類型放`~/markdowns/` , src放 ~/*
3. 適當使用struct enum union,enum部份避免記憶體用量太大可以使用 `enum class TypeName: uint8_t{...};`,然後定義請統一放在 .h 內 
4. 統一使用一個.clang-format 格式化避免格式混雜 套用方法在下方
套用方法：
```sh
clang-format --version
clang-format -i include/*.h *.cpp *.ino
```
沒有clang-format 的話因為我們OS都是arch所以
```sh
sudo pacman -Syu
sudo pacman -S clang
```
5. 有狀況錯誤需要卡死的話都使用`for(;;);`來卡死
6. 變數為enum時 賦值時 請使用這個格式
```cpp
AppState currentAppState = AppState::**Your Status**
``` 
7. 作boolean `!=` 時改使用 `^` 搭配C++原生`static_cast<type>(foo)` 時間消耗1 CLK 較且在組語XOR速度較 `==` `!=` 速度快 如果型態本身就可以作 `^` 那就直接運算即可

8. the burn command in platformio -> ```pio run -e nano_new -t upload --upload-port $port_name``` 

9. 高度 / 數值相關的 常數 `marco`變數 改用`constexpr`取代`define`
