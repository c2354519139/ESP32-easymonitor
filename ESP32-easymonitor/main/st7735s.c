#include "st7735s.h"
#include "driver/spi_master.h"
#include "font8x16.h"
#include "esp_log.h"

// ==================== 6×8 小字体（ASCII 32-126, 每字符 6 字节列数据） ====================
static const uint8_t font6x8[95][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, // '$'
    {0x23,0x13,0x08,0x64,0x62,0x00}, // '%'
    {0x36,0x49,0x55,0x22,0x50,0x00}, // '&'
    {0x00,0x05,0x03,0x00,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, // '*'
    {0x08,0x08,0x3E,0x08,0x08,0x00}, // '+'
    {0x00,0x50,0x30,0x00,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08,0x00}, // '-'
    {0x00,0x60,0x60,0x00,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02,0x00}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, // '0'
    {0x00,0x42,0x7F,0x40,0x00,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46,0x00}, // '2'
    {0x21,0x41,0x45,0x4B,0x31,0x00}, // '3'
    {0x18,0x14,0x12,0x7F,0x10,0x00}, // '4'
    {0x27,0x45,0x45,0x45,0x39,0x00}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, // '6'
    {0x01,0x71,0x09,0x05,0x03,0x00}, // '7'
    {0x36,0x49,0x49,0x49,0x36,0x00}, // '8'
    {0x06,0x49,0x49,0x29,0x1E,0x00}, // '9'
    {0x00,0x36,0x36,0x00,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00,0x00}, // ';'
    {0x00,0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14,0x00}, // '='
    {0x41,0x22,0x14,0x08,0x00,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06,0x00}, // '?'
    {0x32,0x49,0x79,0x41,0x3E,0x00}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36,0x00}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22,0x00}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41,0x00}, // 'E'
    {0x7F,0x09,0x09,0x01,0x01,0x00}, // 'F'
    {0x3E,0x41,0x41,0x51,0x32,0x00}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01,0x00}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41,0x00}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40,0x00}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06,0x00}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46,0x00}, // 'R'
    {0x46,0x49,0x49,0x49,0x31,0x00}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01,0x00}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, // 'V'
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, // 'W'
    {0x63,0x14,0x08,0x14,0x63,0x00}, // 'X'
    {0x03,0x04,0x78,0x04,0x03,0x00}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43,0x00}, // 'Z'
    {0x00,0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20,0x00}, // '\'
    {0x41,0x41,0x7F,0x00,0x00,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04,0x00}, // '^'
    {0x40,0x40,0x40,0x40,0x40,0x00}, // '_'
    {0x00,0x01,0x02,0x04,0x00,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78,0x00}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38,0x00}, // 'b'
    {0x38,0x44,0x44,0x44,0x20,0x00}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F,0x00}, // 'd'
    {0x38,0x54,0x54,0x54,0x18,0x00}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02,0x00}, // 'f'
    {0x08,0x14,0x54,0x54,0x3C,0x00}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78,0x00}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00,0x00}, // 'j'
    {0x00,0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78,0x00}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78,0x00}, // 'n'
    {0x38,0x44,0x44,0x44,0x38,0x00}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08,0x00}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C,0x00}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08,0x00}, // 'r'
    {0x48,0x54,0x54,0x54,0x20,0x00}, // 's'
    {0x04,0x3F,0x44,0x40,0x20,0x00}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, // 'w'
    {0x44,0x28,0x10,0x28,0x44,0x00}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44,0x00}, // 'z'
    {0x00,0x08,0x36,0x41,0x00,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00,0x00}, // '}'
    {0x08,0x04,0x08,0x10,0x08,0x00}, // '~'
};

static const char *TAG = "ST7735S";
static spi_device_handle_t spi_dev = NULL;
static st7735s_rotate_t screen_rotate = ST7735S_ROTATE_0;
static uint16_t screen_width  = ST7735S_WIDTH;
static uint16_t screen_height = ST7735S_HEIGHT;

// ==================== SPI 底层 ====================

static void st7735s_send_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .tx_buffer = &cmd,
        .length = 8,
    };
    gpio_set_level(ST7735S_DC_GPIO, 0);
    spi_device_polling_transmit(spi_dev, &t);
}

static void st7735s_send_data_byte(uint8_t data)
{
    spi_transaction_t t = {
        .tx_buffer = &data,
        .length = 8,
    };
    gpio_set_level(ST7735S_DC_GPIO, 1);
    spi_device_polling_transmit(spi_dev, &t);
}

// 批量发送数据（DC=1），keep_cs=true 时保持 CS 不释放
static void st7735s_send_data_burst(const uint8_t *data, size_t len, bool keep_cs)
{
    if (len == 0) return;
    spi_transaction_t t = {
        .tx_buffer = data,
        .length = len * 8,
        .flags = keep_cs ? SPI_TRANS_CS_KEEP_ACTIVE : 0,
    };
    gpio_set_level(ST7735S_DC_GPIO, 1);
    spi_device_polling_transmit(spi_dev, &t);
}

// ==================== 窗口设置 ====================

static void st7735s_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t caset[4], raset[4];

    switch (screen_rotate) {
    case ST7735S_ROTATE_0:
        caset[0] = x1 >> 8;  caset[1] = x1 & 0xFF;
        caset[2] = x2 >> 8;  caset[3] = x2 & 0xFF;
        raset[0] = y1 >> 8;  raset[1] = y1 & 0xFF;
        raset[2] = y2 >> 8;  raset[3] = y2 & 0xFF;
        break;
    case ST7735S_ROTATE_90:
    case ST7735S_ROTATE_270:
        caset[0] = y1 >> 8;  caset[1] = y1 & 0xFF;
        caset[2] = y2 >> 8;  caset[3] = y2 & 0xFF;
        raset[0] = (screen_width - x2 - 1) >> 8;
        raset[1] = (screen_width - x2 - 1) & 0xFF;
        raset[2] = (screen_width - x1 - 1) >> 8;
        raset[3] = (screen_width - x1 - 1) & 0xFF;
        break;
    case ST7735S_ROTATE_180:
        caset[0] = (screen_width - x2 - 1) >> 8;
        caset[1] = (screen_width - x2 - 1) & 0xFF;
        caset[2] = (screen_width - x1 - 1) >> 8;
        caset[3] = (screen_width - x1 - 1) & 0xFF;
        raset[0] = (screen_height - y2 - 1) >> 8;
        raset[1] = (screen_height - y2 - 1) & 0xFF;
        raset[2] = (screen_height - y1 - 1) >> 8;
        raset[3] = (screen_height - y1 - 1) & 0xFF;
        break;
    }

    st7735s_send_cmd(0x2A);
    st7735s_send_data_burst(caset, 4, false);
    st7735s_send_cmd(0x2B);
    st7735s_send_data_burst(raset, 4, false);
    st7735s_send_cmd(0x2C);  // RAMWR
}

// ==================== 初始化 ====================

esp_err_t st7735s_init(st7735s_rotate_t rotate)
{
    // GPIO 初始化
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ST7735S_DC_GPIO) |
                        (1ULL << ST7735S_RST_GPIO) |
                        (1ULL << ST7735S_BL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // 硬件复位
    gpio_set_level(ST7735S_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(ST7735S_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // SPI 初始化
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = ST7735S_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = ST7735S_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ST7735S_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 8 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = ST7735S_CS_GPIO,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(ST7735S_SPI_HOST, &dev_cfg, &spi_dev));

    // ========== ST7735S 最简初始化序列 ==========

    st7735s_send_cmd(0x11);      // SLPOUT: 退出睡眠
    vTaskDelay(pdMS_TO_TICKS(120));

    st7735s_send_cmd(0x3A);      // COLMOD: 像素格式
    st7735s_send_data_byte(0x05); // 16-bit/pixel (与 Adafruit 库一致)

    st7735s_send_cmd(0x36);      // MADCTL: 显示方向
    switch (rotate) {
    case ST7735S_ROTATE_0:   st7735s_send_data_byte(0x40); break;  // MX=1
    case ST7735S_ROTATE_90:  st7735s_send_data_byte(0x60); break;
    case ST7735S_ROTATE_180: st7735s_send_data_byte(0xC0); break;
    case ST7735S_ROTATE_270: st7735s_send_data_byte(0xA0); break;
    }

    st7735s_send_cmd(0x29);      // DISPON: 开启显示
    vTaskDelay(pdMS_TO_TICKS(20));

    // 保存状态
    screen_rotate = rotate;
    if (rotate == ST7735S_ROTATE_90 || rotate == ST7735S_ROTATE_270) {
        screen_width  = ST7735S_HEIGHT;
        screen_height = ST7735S_WIDTH;
    } else {
        screen_width  = ST7735S_WIDTH;
        screen_height = ST7735S_HEIGHT;
    }

    ESP_LOGI(TAG, "init ok, %dx%d rot=%d", screen_width, screen_height, rotate);
    return ESP_OK;
}

// ==================== 清屏 ====================

void st7735s_clear(uint16_t color)
{
    // 逐行清屏：每行一个 set_window + 一次 SPI 事务，与字符串渲染模式一致
    uint8_t color_h = (color >> 8) & 0xFF;
    uint8_t color_l = color & 0xFF;

    uint8_t row_buf[ST7735S_HEIGHT * 2];  // 最大宽 160 像素 × 2 字节 = 320 字节/行

    for (int y = 0; y < screen_height; y++) {
        st7735s_set_window(0, y, screen_width - 1, y);
        for (int x = 0; x < screen_width; x++) {
            row_buf[x * 2]     = color_h;
            row_buf[x * 2 + 1] = color_l;
        }
        st7735s_send_data_burst(row_buf, screen_width * 2, false);
    }
}

// ==================== 字符串显示 ====================

static void _draw_char_portrait(uint16_t x, uint16_t y, uint8_t c,
                                uint8_t fh, uint8_t fl, uint8_t bh, uint8_t bl)
{
    if (c < 32 || c > 126) c = 32;
    int idx = c - 32;

    st7735s_set_window(x, y, x + 7, y + 15);

    uint8_t buf[256];
    for (int row = 0; row < 16; row++) {
        uint8_t d = asc[idx * 16 + row];
        for (int col = 0; col < 8; col++) {
            int pos = (row * 8 + col) * 2;
            if (d & 0x80) {
                buf[pos]     = fh;
                buf[pos + 1] = fl;
            } else {
                buf[pos]     = bh;
                buf[pos + 1] = bl;
            }
            d <<= 1;
        }
    }
    st7735s_send_data_burst(buf, 256, false);
}

static void _draw_char_landscape(uint16_t x, uint16_t y, uint8_t c,
                                 uint8_t fh, uint8_t fl, uint8_t bh, uint8_t bl)
{
    if (c < 32 || c > 126) c = 32;
    int idx = c - 32;

    st7735s_set_window(x, y, x + 7, y + 15);

    bool rev_rows = (screen_rotate == ST7735S_ROTATE_90);
    bool rev_cols = true;

    uint8_t buf[256];
    for (int jc = 0; jc < 8; jc++) {
        int col = rev_cols ? (7 - jc) : jc;
        for (int ic = 0; ic < 16; ic++) {
            int row = rev_rows ? (15 - ic) : ic;
            int pos = (jc * 16 + ic) * 2;
            if (asc[idx * 16 + row] & (0x80 >> col)) {
                buf[pos]     = fh;
                buf[pos + 1] = fl;
            } else {
                buf[pos]     = bh;
                buf[pos + 1] = bl;
            }
        }
    }
    st7735s_send_data_burst(buf, 256, false);
}

void st7735s_show_string(uint16_t x, uint16_t y, const char *str,
                         uint16_t color, uint16_t bg_color,
                         st7735s_font_t font)
{
    if (x >= screen_width || y >= screen_height || !str) return;

    uint8_t fh = (color >> 8) & 0xFF;
    uint8_t fl = color & 0xFF;
    uint8_t bh = (bg_color >> 8) & 0xFF;
    uint8_t bl = bg_color & 0xFF;

    if (font == ST7735S_FONT_6X8) {
        while (*str) {
            uint8_t c = *str++;
            if (c < 32 || c > 126) c = 32;
            const uint8_t *col_data = font6x8[c - 32];

            st7735s_set_window(x, y, x + 5, y + 7);

            uint8_t buf[96];
            for (int col = 0; col < 6; col++) {
                uint8_t d = col_data[col];
                for (int row = 0; row < 8; row++) {
                    int pos = (row * 6 + col) * 2;
                    if (d & 0x80) {
                        buf[pos]     = fh;
                        buf[pos + 1] = fl;
                    } else {
                        buf[pos]     = bh;
                        buf[pos + 1] = bl;
                    }
                    d <<= 1;
                }
            }
            st7735s_send_data_burst(buf, 96, false);
            x += 6;
        }
    } else {
        bool landscape = (screen_rotate == ST7735S_ROTATE_90 ||
                          screen_rotate == ST7735S_ROTATE_270);
        while (*str) {
            if (landscape)
                _draw_char_landscape(x, y, *str, fh, fl, bh, bl);
            else
                _draw_char_portrait(x, y, *str, fh, fl, bh, bl);
            x += 8;
            str++;
        }
    }
}

// ==================== 汉字显示（16×16） ====================

void st7735s_show_chinese(uint16_t x, uint16_t y, uint8_t num,
                          uint16_t color, uint16_t bg_color)
{
    if (x >= screen_width || y >= screen_height) return;

    uint8_t fh = (color >> 8) & 0xFF;
    uint8_t fl = color & 0xFF;
    uint8_t bh = (bg_color >> 8) & 0xFF;
    uint8_t bl = bg_color & 0xFF;

    st7735s_set_window(x, y, x + 15, y + 15);

    bool landscape = (screen_rotate == ST7735S_ROTATE_90 ||
                      screen_rotate == ST7735S_ROTATE_270);

    uint8_t buf[512];

    if (landscape) {
        bool rev_rows = (screen_rotate == ST7735S_ROTATE_90);
        bool rev_cols = true;
        for (int jc = 0; jc < 16; jc++) {
            int col = rev_cols ? (15 - jc) : jc;
            for (int ic = 0; ic < 16; ic++) {
                int row = rev_rows ? (15 - ic) : ic;
                uint8_t byte;
                if (col < 8)
                    byte = chinese_font[num * 32 + row * 2];
                else
                    byte = chinese_font[num * 32 + row * 2 + 1];
                int pos = (jc * 16 + ic) * 2;
                if (byte & (0x80 >> (col & 7))) {
                    buf[pos]     = fh;
                    buf[pos + 1] = fl;
                } else {
                    buf[pos]     = bh;
                    buf[pos + 1] = bl;
                }
            }
        }
    } else {
        for (int row = 0; row < 16; row++) {
            uint8_t d1 = chinese_font[num * 32 + row * 2];
            uint8_t d2 = chinese_font[num * 32 + row * 2 + 1];
            for (int col = 0; col < 8; col++) {
                int pos = (row * 16 + col) * 2;
                if (d1 & 0x80) {
                    buf[pos]     = fh;
                    buf[pos + 1] = fl;
                } else {
                    buf[pos]     = bh;
                    buf[pos + 1] = bl;
                }
                d1 <<= 1;
            }
            for (int col = 0; col < 8; col++) {
                int pos = (row * 16 + col + 8) * 2;
                if (d2 & 0x80) {
                    buf[pos]     = fh;
                    buf[pos + 1] = fl;
                } else {
                    buf[pos]     = bh;
                    buf[pos + 1] = bl;
                }
                d2 <<= 1;
            }
        }
    }
    st7735s_send_data_burst(buf, 512, false);
}

// ==================== LVGL 刷新接口 ====================

/*
 * 面板 y=0 在底部，LVGL y=0 在顶部，逐行翻转发送。
 */
void st7735s_flush(int x1, int x2, int y1, int y2, void *data)
{
    int row_bytes = (x2 - x1) * 2;
    const uint8_t *p = (const uint8_t *)data;
    for (int lvgl_y = y1; lvgl_y < y2; lvgl_y++) {
        int phys_y = 159 - lvgl_y;
        st7735s_set_window(x1, phys_y, x2 - 1, phys_y);
        st7735s_send_data_burst(p, row_bytes, false);
        p += row_bytes;
    }
}

// ==================== 背光控制 ====================

void st7735s_set_backlight(bool enable)
{
    gpio_set_level(ST7735S_BL_GPIO, enable ? 1 : 0);
}
