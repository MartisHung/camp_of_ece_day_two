#ifndef LCD_RELATED
#define LCD_RELATED
// ---------------Colour----------------
#include <stdint.h>
#define COLOR_GRAY 0x7BEFu
#define COLOR_CYAN ILI9341_CYAN
#define COLOR_WHITE ILI9341_WHITE
#define COLOR_BLACK ILI9341_BLACK
#define COLOR_GREEN ILI9341_GREEN
#define COLOR_YELLOW ILI9341_YELLOW

//  ---------------Height----------------
constexpr uint8_t HIGHT_OF_THE_HORIZONTIAL_SAPERATE_LINE_FOR_TITLE_AND_DATAS = 55;

void drawCalibationScreen();
#endif // !LCD_RELATED
