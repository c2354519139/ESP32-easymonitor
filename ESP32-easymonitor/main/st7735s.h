#ifndef __ST7735S_H__
#define __ST7735S_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 引脚定义 ====================
#define ST7735S_SCK_GPIO   18
#define ST7735S_MOSI_GPIO  19
#define ST7735S_CS_GPIO     2
#define ST7735S_DC_GPIO    21
#define ST7735S_RST_GPIO   20
#define ST7735S_BL_GPIO    12

#define ST7735S_SPI_HOST   SPI2_HOST

// ==================== 屏幕分辨率 ====================
#define ST7735S_WIDTH   128
#define ST7735S_HEIGHT  160

// ==================== 旋转模式 ====================
typedef enum {
    ST7735S_ROTATE_0   = 0,
    ST7735S_ROTATE_90  = 1,
    ST7735S_ROTATE_180 = 2,
    ST7735S_ROTATE_270 = 3,
} st7735s_rotate_t;

// ==================== 字体大小 ====================
typedef enum {
    ST7735S_FONT_6X8  = 0,  // 6×8 小字体
    ST7735S_FONT_8X16 = 1,  // 8×16 大字体
} st7735s_font_t;

// ==================== 常用颜色 (RGB565) ====================
#define ST7735S_WHITE  0xFFFF
#define ST7735S_BLACK  0x0000
#define ST7735S_RED    0xF800
#define ST7735S_GREEN  0x07E0
#define ST7735S_BLUE   0x001F
#define ST7735S_YELLOW 0xFFE0
#define ST7735S_CYAN   0x07FF
#define ST7735S_MAGENTA 0xF81F
#define ST7735S_GRAY   0x8410

// ==================== 公开函数 ====================

esp_err_t st7735s_init(st7735s_rotate_t rotate);

void st7735s_clear(uint16_t color);

/**
 * 在指定位置显示 ASCII 字符串
 * @param font   ST7735S_FONT_6X8 或 ST7735S_FONT_8X16
 */
void st7735s_show_string(uint16_t x, uint16_t y, const char *str,
                         uint16_t color, uint16_t bg_color,
                         st7735s_font_t font);

void st7735s_show_chinese(uint16_t x, uint16_t y, uint8_t num,
                          uint16_t color, uint16_t bg_color);

void st7735s_set_backlight(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* __ST7735S_H__ */
