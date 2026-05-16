#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "st7735s.h"

#define TAG     "lv_port"

#define LCD_WIDTH   128
#define LCD_HEIGHT  160

static lv_disp_drv_t disp_drv;

static void disp_flush(struct _lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    st7735s_flush(area->x1, area->x2 + 1, area->y1, area->y2 + 1, color_p);
    lv_disp_flush_ready(&disp_drv);
}

static void lv_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;
    const size_t disp_buf_size = LCD_WIDTH * (LCD_HEIGHT / 7);

    lv_color_t *buf1 = heap_caps_malloc(disp_buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(disp_buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "disp buf malloc fail");
        return;
    }
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, disp_buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_flush;
    lv_disp_drv_register(&disp_drv);
}

static void lv_tick_cb(void *arg)
{
    uint32_t tick_interval = *((uint32_t *)arg);
    lv_tick_inc(tick_interval);
}

static void lv_tick_init(void)
{
    static uint32_t tick_interval = 5;
    const esp_timer_create_args_t arg = {
        .arg = &tick_interval,
        .callback = lv_tick_cb,
        .name = "",
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = true,
    };

    esp_timer_handle_t timer_handle;
    esp_timer_create(&arg, &timer_handle);
    esp_timer_start_periodic(timer_handle, tick_interval * 1000);
}

esp_err_t lv_port_init(void)
{
    lv_init();
    lv_disp_init();
    lv_tick_init();
    ESP_LOGI(TAG, "init ok, %dx%d", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;
}
